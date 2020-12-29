#include "plugin.hpp"
#include <random>
#include <iostream>


struct ChanceSeq : Module {
	const static int numSteps = 16;

	enum ParamIds {
		ENUMS(ROW1_PARAM, numSteps),
		ENUMS(ROW2_PARAM, numSteps),
		ENUMS(ROW3_PARAM, numSteps),
		ENUMS(ROW4_PARAM, numSteps),
		ENUMS(ROW1_CHANCE_GATE_PARAM, numSteps),
		ENUMS(ROW2_CHANCE_GATE_PARAM, numSteps),
		ENUMS(ROW3_CHANCE_GATE_PARAM, numSteps),
		ENUMS(ROW4_CHANCE_GATE_PARAM, numSteps),
		ENUMS(ROW1_CHANCE_PITCH_PARAM, numSteps),
		ENUMS(ROW2_CHANCE_PITCH_PARAM, numSteps),
		ENUMS(ROW3_CHANCE_PITCH_PARAM, numSteps),
		ENUMS(ROW4_CHANCE_PITCH_PARAM, numSteps),
		ENUMS(TRIGGER_PARAM, numSteps),
		ENUMS(STEP_MODE_PARAM, numSteps),
		NUM_PARAMS
	};
	enum InputIds {
		EXT_CLOCK_INPUT,
		ENUMS(TRIGGER_INPUT, numSteps),
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
		ENUMS(TRIGGER_OUTPUT, numSteps),
		NUM_OUTPUTS
	};
	enum LightIds {
		GATES_LIGHT,
		ENUMS(ROW_LIGHTS, 4),
		ENUMS(TRIGGER_LIGHTS, numSteps),
		NUM_LIGHTS
	};

	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger gateTriggers[numSteps];
	dsp::SchmittTrigger gateInputTriggers[numSteps];
	/** Phase of internal LFO */
	float phase = 0.f;
	int index = 0;

	/** for chance gate knobs */
	bool coinFlip[4];

	/** for change pitch knobs */
	float pitchShift[4];

	/**
	 * Time trigger buttons/inputs have not been held. This is reset every tick something is held.
	 * Used to avoid sync issues with clock and trigger inputs/switches
	 */
	dsp::Timer releaseTimer;

	ChanceSeq() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < numSteps; i++) {
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
			configParam(STEP_MODE_PARAM + i, 0.f, 2.f, 1.f);
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

	void setOutput(const ProcessArgs& args, bool gateIn, bool trigIn) {
		for (int i = 0; i < numSteps; i++) {
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

		lights[GATES_LIGHT].setSmoothBrightness(gateIn, args.sampleTime);
	}

	void setRandomValues() {
		coinFlip[0] = random::uniform() < params[ROW1_CHANCE_GATE_PARAM + index].getValue();
		coinFlip[1] = random::uniform() < params[ROW2_CHANCE_GATE_PARAM + index].getValue();
		coinFlip[2] = random::uniform() < params[ROW3_CHANCE_GATE_PARAM + index].getValue();
		coinFlip[3] = random::uniform() < params[ROW4_CHANCE_GATE_PARAM + index].getValue();

		pitchShift[0] = random::uniform() * params[ROW1_CHANCE_PITCH_PARAM + index].getValue();
		pitchShift[1] = random::uniform() * params[ROW2_CHANCE_PITCH_PARAM + index].getValue();
		pitchShift[2] = random::uniform() * params[ROW3_CHANCE_PITCH_PARAM + index].getValue();
		pitchShift[3] = random::uniform() * params[ROW4_CHANCE_PITCH_PARAM + index].getValue();
	}

	void setIndex(int index) {
		phase = 0.f;
		this->index = index;
		if (this->index >= numSteps)
			this->index = 0;
	}

	bool isPauseStep() {
		int nextStep = index == numSteps - 1 ? 0 : index + 1;

		return params[STEP_MODE_PARAM + nextStep].getValue() == 2.f;
	}

	int nextClockStep() {
		for (int i = index + 1; i < numSteps; i++) {
			if (params[STEP_MODE_PARAM + i].getValue() != 0.f)
				return i;
		}

		for (int i = 0; i < index + 1; i++) {
			if (params[STEP_MODE_PARAM + i].getValue() != 0.f) {
				return i;
			}
		}
		return 0;
	}

	void process(const ProcessArgs& args) override {
		if (releaseTimer.time < 100000) // Max out timer at 100 seconds to avoid overflow
			releaseTimer.process(1);

		bool gateIn = false;
		bool trigIn = false;
		bool triggerPressed = false;

		// TRIGGER inputs
		for (int i = 0; i < numSteps; i++) {
			if (inputs[TRIGGER_INPUT + i].isConnected()) {
				if (gateInputTriggers[i].process(inputs[TRIGGER_INPUT + i].getVoltage()))
					trigIn = true;

				if (inputs[TRIGGER_INPUT + i].getVoltage() > 0.f) {
					setIndex(i);
					triggerPressed = true;
				}
			}
		}

		// TRIGGER buttons
		for (int i = 0; i < numSteps; i++) {
			if (gateTriggers[i].process(params[TRIGGER_PARAM + i].getValue()))
				trigIn = true;

			if (params[TRIGGER_PARAM + i].getValue() == 1) {
				setIndex(i);
				triggerPressed = true;
			}
		}

		if (triggerPressed)
			releaseTimer.reset();


		if (inputs[EXT_CLOCK_INPUT].isConnected()) {
			/**
			 * External clock, Stuff within block called when triggered.
			 * Avoid going to next index for two ticks after trigger button/output was held.
			 */
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].getVoltage()) && releaseTimer.time > 1 && !isPauseStep()) {
				setIndex(nextClockStep());
				trigIn = true;
			}
			gateIn = clockTrigger.isHigh();
		}

		// randomize values based on params if triggered
		if (trigIn)
			setRandomValues();

		setOutput(args, gateIn, trigIn);
	}
};

struct ChanceSeqWidget : ModuleWidget {
	ChanceSeqWidget(ChanceSeq* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ChanceSeq.svg")));

		static const float portX[16] = {20, 70, 120, 170, 220, 270, 320, 370, 420, 470, 520, 570, 620, 670, 720, 770};

		addInput(createInput<PJ301MPort>(Vec(830 - 1, 70), module, ChanceSeq::EXT_CLOCK_INPUT));
		for (int i = 0; i < 16; i++) {
			addParam(createParam<NKK>(Vec(portX[i] - 1, 20), module, ChanceSeq::STEP_MODE_PARAM + i));

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
	}
};



Model* modelChanceSeq = createModel<ChanceSeq, ChanceSeqWidget>("ChanceSeq");
