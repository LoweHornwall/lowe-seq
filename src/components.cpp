#include "components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

PitchKnob::PitchKnob() {
    minAngle = -0.75*M_PI;
    maxAngle = 0.75*M_PI;
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PitchKnob.svg")));
};

ChanceKnob::ChanceKnob() {
    minAngle = -0.75*M_PI;
    maxAngle = 0.75*M_PI;
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ChanceKnob.svg")));
};
