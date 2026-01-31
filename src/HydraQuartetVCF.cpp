#include "plugin.hpp"
#include "SVFilter.hpp"

struct HydraQuartetVCF : Module {
	enum ParamId {
		CUTOFF_PARAM,
		CUTOFF_ATTEN_PARAM,
		RESONANCE_PARAM,
		DRIVE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_INPUT,
		CUTOFF_CV_INPUT,
		RESONANCE_CV_INPUT,
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

	SVFilter filter;

	HydraQuartetVCF() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff");
		configParam(CUTOFF_ATTEN_PARAM, -1.f, 1.f, 0.f, "Cutoff CV");
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance");
		configParam(DRIVE_PARAM, 0.f, 1.f, 0.f, "Drive");

		configInput(AUDIO_INPUT, "Audio");
		configInput(CUTOFF_CV_INPUT, "Cutoff CV");
		configInput(RESONANCE_CV_INPUT, "Resonance CV");

		configOutput(LP_OUTPUT, "Lowpass");
		configOutput(HP_OUTPUT, "Highpass");
		configOutput(BP_OUTPUT, "Bandpass");
		configOutput(NOTCH_OUTPUT, "Notch");
	}

	void process(const ProcessArgs& args) override {
		// Read cutoff parameter and map to frequency
		float cutoffParam = params[CUTOFF_PARAM].getValue();
		float baseCutoffHz = 20.f * std::pow(1000.f, cutoffParam);  // 20Hz-20kHz log

		// Read resonance parameter
		float resonanceParam = params[RESONANCE_PARAM].getValue();

		// Apply CV modulation if connected
		float cutoffHz = baseCutoffHz;
		if (inputs[CUTOFF_CV_INPUT].isConnected()) {
			float cutoffCV = inputs[CUTOFF_CV_INPUT].getVoltage();
			float cvAmount = params[CUTOFF_ATTEN_PARAM].getValue();
			cutoffHz = baseCutoffHz * std::pow(2.f, cutoffCV * cvAmount);
		}

		// Clamp cutoff to valid range
		cutoffHz = rack::clamp(cutoffHz, 20.f, 20000.f);

		// Update filter parameters
		filter.setParams(cutoffHz, resonanceParam, args.sampleRate);

		// Process audio through filter
		float input = inputs[AUDIO_INPUT].getVoltage();
		SVFilterOutputs out = filter.process(input);

		// Output all four filter modes
		outputs[LP_OUTPUT].setVoltage(out.lowpass);
		outputs[HP_OUTPUT].setVoltage(out.highpass);
		outputs[BP_OUTPUT].setVoltage(out.bandpass);
		outputs[NOTCH_OUTPUT].setVoltage(out.notch);
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
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(55.0, 68.0)), module, HydraQuartetVCF::DRIVE_PARAM));

		// Inputs (green circles in SVG)
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.0, 35.0)), module, HydraQuartetVCF::AUDIO_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.0, 50.0)), module, HydraQuartetVCF::CUTOFF_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.0, 88.0)), module, HydraQuartetVCF::RESONANCE_CV_INPUT));

		// Outputs (blue circles in SVG)
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.0, 107.0)), module, HydraQuartetVCF::LP_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 107.0)), module, HydraQuartetVCF::HP_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.0, 107.0)), module, HydraQuartetVCF::BP_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.0, 118.0)), module, HydraQuartetVCF::NOTCH_OUTPUT));
	}
};

Model* modelHydraQuartetVCF = createModel<HydraQuartetVCF, HydraQuartetVCFWidget>("HydraQuartetVCF");
