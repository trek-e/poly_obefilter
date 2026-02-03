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

	HydraQuartetVCF() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff");
		configParam(CUTOFF_ATTEN_PARAM, -1.f, 1.f, 0.f, "Cutoff CV");
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance");
		configParam(RESONANCE_ATTEN_PARAM, -1.f, 1.f, 0.f, "Resonance CV");
		configParam(DRIVE_PARAM, 0.f, 1.f, 0.f, "Drive");
		configParam(DRIVE_ATTEN_PARAM, -1.f, 1.f, 0.f, "Drive CV");

		configInput(AUDIO_INPUT, "Audio");
		configInput(CUTOFF_CV_INPUT, "Cutoff CV");
		configInput(RESONANCE_CV_INPUT, "Resonance CV");
		configInput(DRIVE_CV_INPUT, "Drive CV");

		configOutput(LP_OUTPUT, "Lowpass");
		configOutput(HP_OUTPUT, "Highpass");
		configOutput(BP_OUTPUT, "Bandpass");
		configOutput(NOTCH_OUTPUT, "Notch");
	}

	void process(const ProcessArgs& args) override {
		// Get channel count from audio input (polyphonic: 1-16 channels)
		int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());

		// Read global parameters (knob positions)
		float cutoffParam = params[CUTOFF_PARAM].getValue();
		float baseCutoffHz = 20.f * std::pow(1000.f, cutoffParam);  // 20Hz-20kHz log
		float cvAmount = params[CUTOFF_ATTEN_PARAM].getValue();
		float resonanceParam = params[RESONANCE_PARAM].getValue();

		// Process each voice independently
		for (int c = 0; c < channels; c++) {
			// Read per-voice audio
			float input = inputs[AUDIO_INPUT].getPolyVoltage(c);

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

			// Process through this voice's filter
			filters[c].setParams(cutoffHz, resonance, args.sampleRate);
			SVFilterOutputs out = filters[c].process(input);

			// Write outputs for this channel
			outputs[LP_OUTPUT].setVoltage(out.lowpass, c);
			outputs[HP_OUTPUT].setVoltage(out.highpass, c);
			outputs[BP_OUTPUT].setVoltage(out.bandpass, c);
			outputs[NOTCH_OUTPUT].setVoltage(out.notch, c);
		}

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
