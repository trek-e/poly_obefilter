#include "plugin.hpp"
#include "SVFilter.hpp"

struct HydraQuartetVCF : Module {
	enum ParamId {
		CUTOFF_PARAM,
		CUTOFF_ATTEN_PARAM,
		RESONANCE_PARAM,
		RESONANCE_ATTEN_PARAM,
		DRIVE_PARAM,
		DRIVE_ATTEN_PARAM,
		FILTER_TYPE_PARAM,    // 0 = 12dB SEM, 1 = 24dB OB-X
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_INPUT,
		CUTOFF_CV_INPUT,
		RESONANCE_CV_INPUT,
		DRIVE_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LP_OUTPUT,
		HP_OUTPUT,
		BP_OUTPUT,
		NOTCH_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SVFilter filters[PORT_MAX_CHANNELS];  // 16 filter instances for polyphony
	rack::dsp::TExponentialFilter<float> driveSmoothers[PORT_MAX_CHANNELS];
	bool driveSmootherInitialized = false;

	SVFilter filters24dB_stage2[PORT_MAX_CHANNELS];  // Stage 2 for 24dB cascade

	// Click-free mode switching state
	int prevFilterType = 0;
	int crossfadeCounter = 0;
	static constexpr int CROSSFADE_SAMPLES = 128;  // ~2.7ms at 48kHz
	float xfLP[PORT_MAX_CHANNELS] = {};
	float xfHP[PORT_MAX_CHANNELS] = {};
	float xfBP[PORT_MAX_CHANNELS] = {};
	float xfNotch[PORT_MAX_CHANNELS] = {};

	HydraQuartetVCF() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff");
		configParam(CUTOFF_ATTEN_PARAM, -1.f, 1.f, 0.f, "Cutoff CV");
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance");
		configParam(RESONANCE_ATTEN_PARAM, -1.f, 1.f, 0.f, "Resonance CV");
		configParam(DRIVE_PARAM, 0.f, 1.f, 0.f, "Drive");
		configParam(DRIVE_ATTEN_PARAM, -1.f, 1.f, 0.f, "Drive CV");
		configSwitch(FILTER_TYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type", {"12dB SEM", "24dB OB-X"});

		configInput(AUDIO_INPUT, "Audio");
		configInput(CUTOFF_CV_INPUT, "Cutoff CV");
		configInput(RESONANCE_CV_INPUT, "Resonance CV");
		configInput(DRIVE_CV_INPUT, "Drive CV");

		configOutput(LP_OUTPUT, "Lowpass");
		configOutput(HP_OUTPUT, "Highpass");
		configOutput(BP_OUTPUT, "Bandpass");
		configOutput(NOTCH_OUTPUT, "Notch");

		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			driveSmoothers[i].setTau(0.001f);  // 1ms tau matches cutoff/resonance
		}
	}

	void process(const ProcessArgs& args) override {
		// Get channel count from audio input (polyphonic: 1-16 channels)
		int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());

		// Read filter type switch
		int filterType = (int)(params[FILTER_TYPE_PARAM].getValue() + 0.5f);

		// Detect mode change before the per-voice loop
		bool modeJustChanged = (filterType != prevFilterType);

		// Read global parameters (knob positions)
		float cutoffParam = params[CUTOFF_PARAM].getValue();
		float baseCutoffHz = 20.f * std::pow(1000.f, cutoffParam);  // 20Hz-20kHz log
		float cvAmount = params[CUTOFF_ATTEN_PARAM].getValue();
		float resonanceParam = params[RESONANCE_PARAM].getValue();
		float driveParam = params[DRIVE_PARAM].getValue();
		float driveCvAmount = params[DRIVE_ATTEN_PARAM].getValue();

		// Initialize smoothers on first call
		if (!driveSmootherInitialized) {
			for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
				driveSmoothers[i].out = driveParam;
			}
			driveSmootherInitialized = true;
		}

		// Process each voice independently
		for (int c = 0; c < channels; c++) {
			// Read per-voice audio
			float input = inputs[AUDIO_INPUT].getPolyVoltage(c);

			// Analog noise floor: tiny dither enables self-oscillation from zero state
			// ~1µV RMS, completely inaudible, mimics thermal noise in analog circuits
			input += 1e-6f * ((float)((c * 1013 + args.frame * 2731) & 0xFFFF) / 32768.f - 1.f);

			// Calculate per-voice cutoff (CV is polyphonic)
			float cutoffHz = baseCutoffHz;
			if (inputs[CUTOFF_CV_INPUT].isConnected()) {
				float cutoffCV = inputs[CUTOFF_CV_INPUT].getPolyVoltage(c);
				cutoffHz = baseCutoffHz * std::pow(2.f, cutoffCV * cvAmount);
			}
			cutoffHz = rack::clamp(cutoffHz, 20.f, 20000.f);

			// Calculate per-voice resonance (CV is polyphonic)
			float resonance = resonanceParam;
			if (inputs[RESONANCE_CV_INPUT].isConnected()) {
				float resCV = inputs[RESONANCE_CV_INPUT].getPolyVoltage(c);
				float resCvAmount = params[RESONANCE_ATTEN_PARAM].getValue();
				resonance = rack::clamp(resonanceParam + resCV * resCvAmount * 0.1f, 0.f, 1.f);
			}

			// Calculate per-voice drive (CV is polyphonic, same scaling as resonance: 10%/V)
			float drive = driveParam;
			if (inputs[DRIVE_CV_INPUT].isConnected()) {
				float driveCV = inputs[DRIVE_CV_INPUT].getPolyVoltage(c);
				drive = rack::clamp(driveParam + driveCV * driveCvAmount * 0.1f, 0.f, 1.f);
			}

			// Smooth drive parameter
			float smoothedDrive = driveSmoothers[c].process(1.f / args.sampleRate, drive);

			// Capture crossfade-from values at mode change (before overwriting outputs)
			if (modeJustChanged && c == 0) {
				crossfadeCounter = CROSSFADE_SAMPLES;
			}
			if (modeJustChanged) {
				xfLP[c] = outputs[LP_OUTPUT].getVoltage(c);
				xfHP[c] = outputs[HP_OUTPUT].getVoltage(c);
				xfBP[c] = outputs[BP_OUTPUT].getVoltage(c);
				xfNotch[c] = outputs[NOTCH_OUTPUT].getVoltage(c);
			}

			float outLP, outHP, outBP, outNotch;

			if (filterType == 0) {
				// 12dB SEM mode: existing processing, unchanged from v0.50b
				filters[c].setParams(cutoffHz, resonance, args.sampleRate);
				SVFilterOutputs out = filters[c].process(input);

				// Apply drive with output-specific scaling
				// LP/BP: full drive (low-frequency content saturates naturally)
				// HP: reduced drive (high-frequency less affected)
				// Notch: medium drive
				outLP = blendedSaturation(out.lowpass, smoothedDrive * 1.0f);
				outHP = blendedSaturation(out.highpass, smoothedDrive * 0.5f);
				outBP = blendedSaturation(out.bandpass, smoothedDrive * 1.0f);
				outNotch = blendedSaturation(out.notch, smoothedDrive * 0.7f);
			} else {
				// 24dB OB-X mode: cascaded SVF topology
				// Boosted resonance for bright/edgy OB-X character (1.15x Stage 1, 0.65x Stage 2)
				float resonance24 = rack::clamp(resonance * 1.15f, 0.f, 0.95f);

				// Stage 1: boosted resonance for aggressive OB-X peaks
				filters[c].setParams(cutoffHz, resonance24, args.sampleRate);
				SVFilterOutputs s1 = filters[c].process(input);

				// Inter-stage safety: check for NaN/infinity and clamp
				float interStage;
				if (!std::isfinite(s1.lowpass)) {
					filters[c].reset();
					filters24dB_stage2[c].reset();
					interStage = 0.f;
				} else {
					// Mild tanh saturation for CEM3320 diode-pair nonlinearity character
					// Level-dependent odd harmonics; unity gain for small signals
					interStage = rack::clamp(std::tanh(s1.lowpass * 0.12f) / 0.12f, -12.f, 12.f);
				}

				// Stage 2: split resonance (0.65x) for stability with OB-X edge
				float q2 = resonance24 * 0.65f;
				filters24dB_stage2[c].setParams(cutoffHz, q2, args.sampleRate);
				SVFilterOutputs s2 = filters24dB_stage2[c].process(interStage);

				// Output mapping for 24dB OB-X: LP-only (authentic OB-Xa)
				// HP/BP/Notch silent — crossfade handles click-free transitions
				outLP    = blendedSaturation(s2.lowpass, smoothedDrive * 1.3f);
				outHP    = 0.0f;
				outBP    = 0.0f;
				outNotch = 0.0f;
			}

			// Apply crossfade for click-free mode switching
			if (crossfadeCounter > 0) {
				float t = 1.f - (float)crossfadeCounter / (float)CROSSFADE_SAMPLES;
				outLP = xfLP[c] * (1.f - t) + outLP * t;
				outHP = xfHP[c] * (1.f - t) + outHP * t;
				outBP = xfBP[c] * (1.f - t) + outBP * t;
				outNotch = xfNotch[c] * (1.f - t) + outNotch * t;
			}

			// Write outputs for this channel
			outputs[LP_OUTPUT].setVoltage(outLP, c);
			outputs[HP_OUTPUT].setVoltage(outHP, c);
			outputs[BP_OUTPUT].setVoltage(outBP, c);
			outputs[NOTCH_OUTPUT].setVoltage(outNotch, c);
		}

		// Decrement crossfade counter and update prevFilterType after all voices
		if (crossfadeCounter > 0) crossfadeCounter--;
		prevFilterType = filterType;

		// Set output channel counts (all outputs match input channel count)
		outputs[LP_OUTPUT].setChannels(channels);
		outputs[HP_OUTPUT].setChannels(channels);
		outputs[BP_OUTPUT].setChannels(channels);
		outputs[NOTCH_OUTPUT].setChannels(channels);
	}
};

struct HydraQuartetVCFWidget : ModuleWidget {
	HydraQuartetVCFWidget(HydraQuartetVCF* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/HydraQuartetVCF.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Parameters (red circles in SVG)
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.56, 40.0)), module, HydraQuartetVCF::CUTOFF_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(35.56, 62.0)), module, HydraQuartetVCF::CUTOFF_ATTEN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.56, 88.0)), module, HydraQuartetVCF::RESONANCE_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(35.56, 98.0)), module, HydraQuartetVCF::RESONANCE_ATTEN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(55.0, 40.0)), module, HydraQuartetVCF::DRIVE_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(55.0, 62.0)), module, HydraQuartetVCF::DRIVE_ATTEN_PARAM));

		// Filter type switch (between title divider and drive knob)
		addParam(createParamCentered<CKSS>(mm2px(Vec(55.0, 28.0)), module, HydraQuartetVCF::FILTER_TYPE_PARAM));

		// Inputs (green circles in SVG)
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.0, 35.0)), module, HydraQuartetVCF::AUDIO_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.0, 50.0)), module, HydraQuartetVCF::CUTOFF_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.0, 88.0)), module, HydraQuartetVCF::RESONANCE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(55.0, 50.0)), module, HydraQuartetVCF::DRIVE_CV_INPUT));

		// Outputs (blue circles in SVG)
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.0, 107.0)), module, HydraQuartetVCF::LP_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 107.0)), module, HydraQuartetVCF::HP_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.0, 107.0)), module, HydraQuartetVCF::BP_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.0, 118.0)), module, HydraQuartetVCF::NOTCH_OUTPUT));
	}
};

Model* modelHydraQuartetVCF = createModel<HydraQuartetVCF, HydraQuartetVCFWidget>("HydraQuartetVCF");
