#include "plugin.hpp"
#include <random>
#include <iostream>


struct ChanceSeq : Module {
	enum ParamIds {
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		ENUMS(ROW1_PARAM, 16),
		ENUMS(ROW2_PARAM, 16),
		ENUMS(ROW3_PARAM, 16),
		ENUMS(ROW4_PARAM, 16),
		ENUMS(ROW1_CHANCE_GATE_PARAM, 16),
		ENUMS(ROW2_CHANCE_GATE_PARAM, 16),
		ENUMS(ROW3_CHANCE_GATE_PARAM, 16),
		ENUMS(ROW4_CHANCE_GATE_PARAM, 16),
		ENUMS(ROW1_CHANCE_PITCH_PARAM, 16),
		ENUMS(ROW2_CHANCE_PITCH_PARAM, 16),
		ENUMS(ROW3_CHANCE_PITCH_PARAM, 16),
		ENUMS(ROW4_CHANCE_PITCH_PARAM, 16),
		ENUMS(TRIGGER_PARAM, 16),
		NUM_PARAMS
	};
	enum InputIds {
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		ENUMS(TRIGGER_INPUT, 16),
		NUM_INPUTS
	};
	enum OutputIds {
		ROW1_OUTPUT,
		ROW2_OUTPUT,
		ROW3_OUTPUT,
		ROW4_OUTPUT,
		ROW1_GATE_OUTPUT,
		ROW2_GATE_OUTPUT,
		ROW3_GATE_OUTPUT,
		ROW4_GATE_OUTPUT,
		ROW1_TRIGGER_OUTPUT,
		ROW2_TRIGGER_OUTPUT,
		ROW3_TRIGGER_OUTPUT,
		ROW4_TRIGGER_OUTPUT,
		ENUMS(TRIGGER_OUTPUT, 16),
		NUM_OUTPUTS
	};
	enum LightIds {
		RESET_LIGHT,
		GATES_LIGHT,
		ENUMS(ROW_LIGHTS, 4),
		ENUMS(TRIGGER_LIGHTS, 16),
		NUM_LIGHTS
	};

	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger gateTriggers[16];
	dsp::SchmittTrigger gateInputTriggers[16];
	/** Phase of internal LFO */
	float phase = 0.f;
	int index = 0;

	/** for chance gate knobs */
	bool coinFlip[4] = {false, false, false, false};

	/** for change pitch knobs */
	float pitchShift[4] = {0, 0, 0, 0};

	bool triggerHeld[16];
	dsp::Timer releaseTimer;

	ChanceSeq() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RUN_PARAM, 0.f, 1.f, 0.f);
		configParam(RESET_PARAM, 0.f, 1.f, 0.f);
		configParam(STEPS_PARAM, 1.f, 16.f, 16.f);
		for (int i = 0; i < 16; i++) {
			configParam(ROW1_PARAM + i, 0.f, 10.f, 0.f);
			configParam(ROW2_PARAM + i, 0.f, 10.f, 0.f);
			configParam(ROW3_PARAM + i, 0.f, 10.f, 0.f);
			configParam(ROW4_PARAM + i, 0.f, 10.f, 0.f);
			configParam(ROW1_CHANCE_GATE_PARAM + i, 0.f, 1.f, 1.f);
			configParam(ROW2_CHANCE_GATE_PARAM + i, 0.f, 1.f, 1.f);
			configParam(ROW3_CHANCE_GATE_PARAM + i, 0.f, 1.f, 1.f);
			configParam(ROW4_CHANCE_GATE_PARAM + i, 0.f, 1.f, 1.f);
			configParam(ROW1_CHANCE_PITCH_PARAM + i, -1.f, 1.f, 0.f);
			configParam(ROW2_CHANCE_PITCH_PARAM + i, -1.f, 1.f, 0.f);
			configParam(ROW3_CHANCE_PITCH_PARAM + i, -1.f, 1.f, 0.f);
			configParam(ROW4_CHANCE_PITCH_PARAM + i, -1.f, 1.f, 0.f);
			configParam(TRIGGER_PARAM + i, 0.f, 1.f, 0.f);
		}

		onReset();
	}

	void onReset() override {
	}

	void onRandomize() override {
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
	}

	void setIndex(int index) {
		int numSteps = (int) clamp(std::round(params[STEPS_PARAM].getValue() + inputs[STEPS_INPUT].getVoltage()), 1.f, 16.f);
		phase = 0.f;
		this->index = index;
		if (this->index >= numSteps)
			this->index = 0;
	}

	bool allTriggersReleased() {
		for (int i = 0; i < 16; i++) {
			if (triggerHeld[i])
				return false;
		}
		return true;
	}

	void resetReleaseClock() {
		if (someTriggerHeld()) {
			releaseTimer.reset();
		}
	}

	bool someTriggerHeld() {
		for (int i = 0; i < 16; i++) {
			if (triggerHeld[i])
				return true;
		}
		return false;
	}

	void process(const ProcessArgs& args) override {
		releaseTimer.process(1);
		bool gateIn = false;
		bool trigIn = false;

		// TRIGGER inputs
		for (int i = 0; i < 16; i++) {
			if (inputs[TRIGGER_INPUT + i].isConnected() && gateInputTriggers[i].process(inputs[TRIGGER_INPUT + i].getVoltage()))
				trigIn = true;

			// If value is different from the one before then trigger
			if (inputs[TRIGGER_INPUT + i].getVoltage() > 0.f) {
				if (index != i)
					trigIn = true;

				setIndex(i);
				triggerHeld[i] = true;
			} else {
				triggerHeld[i] = false;
			}
		}
		resetReleaseClock();

		std::cout << releaseTimer.time;
		std::cout << "\n";

		// // TRIGGER buttons
		// for (int i = 0; i < 16; i++) {
		// 	// trigger if a button is pressed
		// 	if (gateTriggers[i].process(params[TRIGGER_PARAM + i].getValue()))
		// 		trigIn = true;

		// 	if (params[TRIGGER_PARAM + i].getValue() == 1) {
		// 		setIndex(i);
		// 		triggerHeld = true;
		// 	}
		// }

		if (inputs[EXT_CLOCK_INPUT].isConnected()) {
			/**
			 * External clock, Stuff within block called when triggered.
			 * Avoid going to next index when trigger button/output is held.
			 */
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].getVoltage()) && !someTriggerHeld() && releaseTimer.time > 1) {
				setIndex(index + 1);
				trigIn = true;
			}
			gateIn = clockTrigger.isHigh();
		}

		// Reset
		if (resetTrigger.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
			setIndex(0);
		}

		// randomize values based on params if triggered
		if (trigIn) {
			coinFlip[0] = random::uniform() < params[ROW1_CHANCE_GATE_PARAM + index].getValue();
			coinFlip[1] = random::uniform() < params[ROW2_CHANCE_GATE_PARAM + index].getValue();
			coinFlip[2] = random::uniform() < params[ROW3_CHANCE_GATE_PARAM + index].getValue();
			coinFlip[3] = random::uniform() < params[ROW4_CHANCE_GATE_PARAM + index].getValue();
			// std::cout << random::uniform();
			// std::cout << "\n";

			pitchShift[0] = random::uniform() * params[ROW1_CHANCE_PITCH_PARAM + index].getValue();
			pitchShift[1] = random::uniform() * params[ROW2_CHANCE_PITCH_PARAM + index].getValue();
			pitchShift[2] = random::uniform() * params[ROW3_CHANCE_PITCH_PARAM + index].getValue();
			pitchShift[3] = random::uniform() * params[ROW4_CHANCE_PITCH_PARAM + index].getValue();

			// std::cout << pitchShift[0];
			// std::cout << "\n";
		}

		// Outputs
		for (int i = 0; i < 16; i++) {
			outputs[TRIGGER_OUTPUT + i].setVoltage((trigIn && i == index) ? 10.f : 0.f);
			lights[TRIGGER_LIGHTS + i].setSmoothBrightness((i == index) ? 1.f : 0.f, args.sampleTime);
		}

		outputs[ROW1_OUTPUT].setVoltage(params[ROW1_PARAM + index].getValue() + pitchShift[0]);
		outputs[ROW1_GATE_OUTPUT].setVoltage((gateIn && coinFlip[0]) ? 10.f : 0.f);
		outputs[ROW1_TRIGGER_OUTPUT].setVoltage((trigIn && coinFlip[0]) ? 10.f : 0.f);

		outputs[ROW2_OUTPUT].setVoltage(params[ROW2_PARAM + index].getValue() + pitchShift[1]);
		outputs[ROW2_GATE_OUTPUT].setVoltage((gateIn && coinFlip[1]) ? 10.f : 0.f);
		outputs[ROW2_TRIGGER_OUTPUT].setVoltage((trigIn && coinFlip[1]) ? 10.f : 0.f);

		outputs[ROW3_OUTPUT].setVoltage(params[ROW3_PARAM + index].getValue() + pitchShift[2]);
		outputs[ROW3_GATE_OUTPUT].setVoltage((gateIn && coinFlip[2]) ? 10.f : 0.f);
		outputs[ROW3_TRIGGER_OUTPUT].setVoltage((trigIn && coinFlip[2]) ? 10.f : 0.f);

		outputs[ROW4_OUTPUT].setVoltage(params[ROW4_PARAM + index].getValue() + pitchShift[3]);
		outputs[ROW4_GATE_OUTPUT].setVoltage((gateIn && coinFlip[3]) ? 10.f : 0.f);
		outputs[ROW4_TRIGGER_OUTPUT].setVoltage((trigIn && coinFlip[3]) ? 10.f : 0.f);

		lights[RESET_LIGHT].setSmoothBrightness(resetTrigger.isHigh(), args.sampleTime);
		lights[GATES_LIGHT].setSmoothBrightness(gateIn, args.sampleTime);
	}
};

struct ChanceSeqWidget : ModuleWidget {
	ChanceSeqWidget(ChanceSeq* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ChanceSeq.svg")));

		static const float portX[16] = {20, 70, 120, 170, 220, 270, 320, 370, 420, 470, 520, 570, 620, 670, 720, 770};

		addParam(createParam<LEDButton>(Vec(830, 47 - 1), module, ChanceSeq::RUN_PARAM));
		addParam(createParam<LEDButton>(Vec(930, 47 - 1), module, ChanceSeq::RESET_PARAM));
		addChild(createLight<MediumLight<BlueLight>>(Vec(960, 50.4f), module, ChanceSeq::RESET_LIGHT));
		addParam(createParam<RoundBlackSnapKnob>(Vec(990, 38), module, ChanceSeq::STEPS_PARAM));

		addInput(createInput<PJ301MPort>(Vec(830 - 1, 70), module, ChanceSeq::EXT_CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(Vec(880 - 1, 70), module, ChanceSeq::RESET_INPUT));
		addInput(createInput<PJ301MPort>(Vec(930 - 1, 70), module, ChanceSeq::STEPS_INPUT));

		for (int i = 0; i < 16; i++) {
			addInput(createInput<PJ301MPort>(Vec(portX[i] + 1, 70), module, ChanceSeq::TRIGGER_INPUT + i));

			addParam(createParam<ChanceGateKnob>(Vec(portX[i] + 12, 100), module, ChanceSeq::ROW1_CHANCE_GATE_PARAM + i));
			addParam(createParam<ChancePitchKnob>(Vec(portX[i] + 28.5f, 115), module, ChanceSeq::ROW1_CHANCE_PITCH_PARAM + i));
			addParam(createParam<PitchKnob>(Vec(portX[i] - 2, 120), module, ChanceSeq::ROW1_PARAM + i));

			addParam(createParam<ChanceGateKnob>(Vec(portX[i] + 12, 150), module, ChanceSeq::ROW2_CHANCE_GATE_PARAM + i));
			addParam(createParam<ChancePitchKnob>(Vec(portX[i] + 28.5f, 165), module, ChanceSeq::ROW2_CHANCE_PITCH_PARAM + i));
			addParam(createParam<PitchKnob>(Vec(portX[i] - 2, 170), module, ChanceSeq::ROW2_PARAM + i));

			addParam(createParam<ChanceGateKnob>(Vec(portX[i] + 12, 200), module, ChanceSeq::ROW3_CHANCE_GATE_PARAM + i));
			addParam(createParam<ChancePitchKnob>(Vec(portX[i] + 28.5f, 215), module, ChanceSeq::ROW3_CHANCE_PITCH_PARAM + i));
			addParam(createParam<PitchKnob>(Vec(portX[i] - 2, 220), module, ChanceSeq::ROW3_PARAM + i));

			addParam(createParam<ChanceGateKnob>(Vec(portX[i] + 12, 250), module, ChanceSeq::ROW4_CHANCE_GATE_PARAM + i));
			addParam(createParam<ChancePitchKnob>(Vec(portX[i] + 28.5f, 265), module, ChanceSeq::ROW4_CHANCE_PITCH_PARAM + i));
			addParam(createParam<PitchKnob>(Vec(portX[i] - 2, 270), module, ChanceSeq::ROW4_PARAM + i));

			addParam(createParam<CKD6>(Vec(portX[i] - 2, 310), module, ChanceSeq::TRIGGER_PARAM + i));
			addChild(createLight<LargeLight<BlueLight>>(Vec(portX[i] + 4.5f, 316), module, ChanceSeq::TRIGGER_LIGHTS + i));
			addOutput(createOutput<PJ301MPort>(Vec(portX[i] - 1.f, 349), module, ChanceSeq::TRIGGER_OUTPUT + i));
		}

		addOutput(createOutput<PJ301MPort>(Vec(830, 120), module, ChanceSeq::ROW1_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(880, 120), module, ChanceSeq::ROW1_GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(930, 120), module, ChanceSeq::ROW1_TRIGGER_OUTPUT));

		addOutput(createOutput<PJ301MPort>(Vec(830, 170), module, ChanceSeq::ROW2_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(880, 170), module, ChanceSeq::ROW2_GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(930, 170), module, ChanceSeq::ROW2_TRIGGER_OUTPUT));

		addOutput(createOutput<PJ301MPort>(Vec(830, 220), module, ChanceSeq::ROW3_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(880, 220), module, ChanceSeq::ROW3_GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(930, 220), module, ChanceSeq::ROW3_TRIGGER_OUTPUT));

		addOutput(createOutput<PJ301MPort>(Vec(830, 270), module, ChanceSeq::ROW4_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(880, 270), module, ChanceSeq::ROW4_GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(930, 270), module, ChanceSeq::ROW4_TRIGGER_OUTPUT));

		// addOutput(createOutput(<PJ301MPort>(oi)))
	}
};



Model* modelChanceSeq = createModel<ChanceSeq, ChanceSeqWidget>("ChanceSeq");
