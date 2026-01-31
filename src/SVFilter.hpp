#pragma once
#include "rack.hpp"
#include <cmath>

struct SVFilterOutputs {
	float lowpass;
	float highpass;
	float bandpass;
	float notch;
};

struct SVFilter {
	// State variables (integrator outputs)
	float ic1eq = 0.f;
	float ic2eq = 0.f;

	// Coefficients
	float g = 0.f;
	float k = 0.f;
	float a1 = 0.f;
	float a2 = 0.f;
	float a3 = 0.f;

	// Parameter smoothing
	rack::dsp::TExponentialFilter<float> cutoffSmoother;
	rack::dsp::TExponentialFilter<float> resonanceSmoother;

	bool initialized = false;

	SVFilter() {
		// Initialize smoothers with ~1ms time constant
		cutoffSmoother.setTau(0.001f);
		resonanceSmoother.setTau(0.001f);
	}

	void setParams(float cutoffHz, float resonance, float sampleRate) {
		// Initialize smoothers on first call to avoid ramp-up delay
		if (!initialized) {
			cutoffSmoother.out = cutoffHz;
			resonanceSmoother.out = resonance;
			initialized = true;
		}

		// Smooth parameters to avoid zipper noise
		float smoothedCutoff = cutoffSmoother.process(1.f / sampleRate, cutoffHz);
		float smoothedResonance = resonanceSmoother.process(1.f / sampleRate, resonance);

		// Clamp cutoff to valid range and normalize
		// Minimum 0.001 to prevent g=0 which would cause zero output
		float cutoffNorm = rack::clamp(smoothedCutoff / sampleRate, 0.001f, 0.49f);

		// Frequency warping for trapezoidal integration
		g = std::tan(M_PI * cutoffNorm);

		// Map resonance 0-1 to Q range (0.5 to 20)
		float Q = 0.5f + smoothedResonance * 19.5f;
		k = 1.f / Q;

		// Pre-compute coefficients
		a1 = 1.f / (1.f + g * (g + k));
		a2 = g * a1;
		a3 = g * a2;
	}

	SVFilterOutputs process(float input) {
		// Clamp input to VCV Rack standard range
		input = rack::clamp(input, -12.f, 12.f);

		// Trapezoidal SVF equations with zero-delay feedback
		float v3 = input - ic2eq;
		float v1_raw = a1 * ic1eq + a2 * v3;

		// Soft saturation in feedback path for Oberheim character
		float v1 = std::tanh(v1_raw * 2.f) * 0.5f;

		float v2 = ic2eq + a2 * ic1eq + a3 * v3;

		// Update integrator states
		ic1eq = 2.f * v1 - ic1eq;
		ic2eq = 2.f * v2 - ic2eq;

		// Check for NaN/infinity and reset if needed
		if (!std::isfinite(v2)) {
			reset();
			return {0.f, 0.f, 0.f, 0.f};
		}

		// Output taps using CLEAN (unsaturated) v1_raw for authentic SVF response
		float bp = v1_raw;                    // Bandpass (pre-saturation for clean response)
		float lp = v2;                        // Lowpass
		float hp = v3 - g * v1_raw;           // Highpass (Cytomic direct form)
		float notch = lp + hp;                // Notch (LP + HP sum)

		return {lp, hp, bp, notch};
	}

	void reset() {
		ic1eq = 0.f;
		ic2eq = 0.f;
	}
};
