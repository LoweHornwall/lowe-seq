#include "plugin.hpp"
#include <random>


struct ChanceSeq : Module {
	enum ParamIds {
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		ENUMS(ROW1_PARAM, 16),
		ENUMS(ROW2_PARAM, 16),
		ENUMS(ROW3_PARAM, 16),
		ENUMS(ROW4_PARAM, 16),
		ENUMS(ROW1_CHANCE_PARAM, 16),
		ENUMS(ROW2_CHANCE_PARAM, 16),
		ENUMS(ROW3_CHANCE_PARAM, 16),
		ENUMS(ROW4_CHANCE_PARAM, 16),
		ENUMS(GATE_PARAM, 16),
		NUM_PARAMS
	};
	enum InputIds {
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT,
		TRIGGER_OUTPUT,
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
		ENUMS(GATE_OUTPUT, 16),
		NUM_OUTPUTS
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		GATES_LIGHT,
		ENUMS(ROW_LIGHTS, 4),
		ENUMS(GATE_LIGHTS, 16),
		NUM_LIGHTS
	};

	bool running = true;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger runningTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger gateTriggers[16];
	/** Phase of internal LFO */
	float phase = 0.f;
	int index = 0;
	bool gates[16] = {};

	/** */
	bool coinFlip = false;

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
			configParam(ROW1_CHANCE_PARAM + i, 0.f, 1.f, 1.f);
			configParam(ROW2_CHANCE_PARAM + i, 0.f, 1.f, 1.f);
			configParam(ROW3_CHANCE_PARAM + i, 0.f, 1.f, 1.f);
			configParam(ROW4_CHANCE_PARAM + i, 0.f, 1.f, 1.f);
			configParam(GATE_PARAM + i, 0.f, 1.f, 0.f);
		}

		onReset();
	}

	void onReset() override {
		for (int i = 0; i < 16; i++) {
			gates[i] = true;
		}
	}

	void onRandomize() override {
		for (int i = 0; i < 16; i++) {
			gates[i] = (random::uniform() > 0.5f);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));

		// gates
		json_t* gatesJ = json_array();
		for (int i = 0; i < 16; i++) {
			json_array_insert_new(gatesJ, i, json_integer((int) gates[i]));
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		// running
		json_t* runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);

		// gates
		json_t* gatesJ = json_object_get(rootJ, "gates");
		if (gatesJ) {
			for (int i = 0; i < 16; i++) {
				json_t* gateJ = json_array_get(gatesJ, i);
				if (gateJ)
					gates[i] = !!json_integer_value(gateJ);
			}
		}
	}

	void setIndex(int index) {
		int numSteps = (int) clamp(std::round(params[STEPS_PARAM].getValue() + inputs[STEPS_INPUT].getVoltage()), 1.f, 16.f);
		phase = 0.f;
		this->index = index;
		if (this->index >= numSteps)
			this->index = 0;
	}

	void process(const ProcessArgs& args) override {
		// Run
		if (runningTrigger.process(params[RUN_PARAM].getValue())) {
			running = !running;
		}

		bool gateIn = false;
		bool trigIn = false;
		if (running) {
			if (inputs[EXT_CLOCK_INPUT].isConnected()) {
				// External clock, Stuff within block called when triggered
				if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].getVoltage())) {
					setIndex(index + 1);
					trigIn = true;
					coinFlip = random::uniform() < params[ROW1_CHANCE_PARAM + index].getValue();
				}
				gateIn = clockTrigger.isHigh();
			}
		}

		// Reset
		if (resetTrigger.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
			setIndex(0);
		}

		// Gate buttons
		for (int i = 0; i < 16; i++) {
			if (gateTriggers[i].process(params[GATE_PARAM + i].getValue())) {
				gates[i] = !gates[i];
			}
			outputs[GATE_OUTPUT + i].setVoltage((running && gateIn && i == index && gates[i]) ? 10.f : 0.f);
			lights[GATE_LIGHTS + i].setSmoothBrightness((gateIn && i == index) ? (gates[i] ? 1.f : 0.12) : (gates[i] ? 0.25 : 0.0), args.sampleTime);
		}

		// Outputs
		outputs[ROW1_OUTPUT].setVoltage(params[ROW1_PARAM + index].getValue());
		outputs[ROW2_OUTPUT].setVoltage(params[ROW2_PARAM + index].getValue());
		outputs[ROW3_OUTPUT].setVoltage(params[ROW3_PARAM + index].getValue());
		outputs[ROW4_OUTPUT].setVoltage(params[ROW4_PARAM + index].getValue());
		outputs[GATES_OUTPUT].setVoltage((gateIn && gates[index] && coinFlip) ? 10.f : 0.f);
		outputs[TRIGGER_OUTPUT].setVoltage((trigIn && gates[index] && coinFlip) ? 10.f : 0.f);
		lights[RUNNING_LIGHT].value = (running);
		lights[RESET_LIGHT].setSmoothBrightness(resetTrigger.isHigh(), args.sampleTime);
		lights[GATES_LIGHT].setSmoothBrightness(gateIn, args.sampleTime);
	}
};


struct ChanceSeqWidget : ModuleWidget {
	ChanceSeqWidget(ChanceSeq* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ChanceSeq.svg")));

		static const float portX[16] = {20, 70, 120, 170, 220, 270, 320, 370, 420, 470, 520, 570, 620, 670, 720, 770};

		addParam(createParam<LEDButton>(Vec(portX[1], 47 - 1), module, ChanceSeq::RUN_PARAM));
		addChild(createLight<MediumLight<BlueLight>>(Vec(portX[2], 50.4f), module, ChanceSeq::RUNNING_LIGHT));
		addParam(createParam<LEDButton>(Vec(portX[3], 47 - 1), module, ChanceSeq::RESET_PARAM));
		addChild(createLight<MediumLight<BlueLight>>(Vec(portX[4], 50.4f), module, ChanceSeq::RESET_LIGHT));
		addParam(createParam<RoundBlackSnapKnob>(Vec(portX[5], 38), module, ChanceSeq::STEPS_PARAM));

		addInput(createInput<PJ301MPort>(Vec(portX[1] - 1, 70), module, ChanceSeq::EXT_CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(Vec(portX[2] - 1, 70), module, ChanceSeq::RESET_INPUT));
		addInput(createInput<PJ301MPort>(Vec(portX[3] - 1, 70), module, ChanceSeq::STEPS_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(portX[4] - 1, 70), module, ChanceSeq::GATES_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(portX[5] - 1, 70), module, ChanceSeq::TRIGGER_OUTPUT));

		for (int i = 0; i < 16; i++) {
			addParam(createParam<ChanceKnob>(Vec(portX[i] + 20, 105), module, ChanceSeq::ROW1_CHANCE_PARAM + i));
			addParam(createParam<PitchKnob>(Vec(portX[i] - 2, 120), module, ChanceSeq::ROW1_PARAM + i));

			addParam(createParam<ChanceKnob>(Vec(portX[i] + 20, 155), module, ChanceSeq::ROW2_CHANCE_PARAM + i));
			addParam(createParam<PitchKnob>(Vec(portX[i] - 2, 170), module, ChanceSeq::ROW2_PARAM + i));

			addParam(createParam<ChanceKnob>(Vec(portX[i] + 20, 205), module, ChanceSeq::ROW3_CHANCE_PARAM + i));
			addParam(createParam<PitchKnob>(Vec(portX[i] - 2, 220), module, ChanceSeq::ROW3_PARAM + i));

			addParam(createParam<ChanceKnob>(Vec(portX[i] + 20, 255), module, ChanceSeq::ROW4_CHANCE_PARAM + i));
			addParam(createParam<PitchKnob>(Vec(portX[i] - 2, 270), module, ChanceSeq::ROW4_PARAM + i));

			addParam(createParam<LEDButton>(Vec(portX[i] + 2.5f, 320 - 1), module, ChanceSeq::GATE_PARAM + i));
			addChild(createLight<MediumLight<BlueLight>>(Vec(portX[i] + 6.9f, 323.4f), module, ChanceSeq::GATE_LIGHTS + i));
			addOutput(createOutput<PJ301MPort>(Vec(portX[i] - 1.5f, 349), module, ChanceSeq::GATE_OUTPUT + i));
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
