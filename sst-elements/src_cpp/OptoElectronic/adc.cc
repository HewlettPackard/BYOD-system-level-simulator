// Copyright (2025) Hewlett Packard Enterprise Development LP
// 
// Licensed under the MIT License (the "License")
//
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "adc.h"

namespace SST {
namespace BYOD {

/**
* @brief BRIEF.
* @details DETAILS
*/
ADC::ADC(ComponentId_t id, Params &params) : Component(id) {

	resolution = 			params.find<uint32_t>("resolution", 1);
	size = 					params.find<uint32_t>("size", 1);
	latency = 				params.find<uint32_t>("latency", 1);
	verbose = 				params.find<uint32_t>("verbose", 0);
	minVin = 				params.find<double>("minVin", 0.0);
	maxVin = 				params.find<double>("maxVin", 1.0);
	conversionEnergy = 		params.find<double>("conversionEnergy", 0.0);
	frequency = 			params.find<UnitAlgebra>("frequency", "1GHz");

	energyConsumption = 	registerStatistic<double_t>("energyADC");

	selfLink = 				configureSelfLink("selfLink", new Event::Handler<ADC>(this, &ADC::handleSelf));
	inputLink = 			configureLink("input",	new Event::Handler<ADC>(this, &ADC::handleInput));
	outputLink = 			configureLink("output");

	registerClock(frequency, new Clock::Handler<ADC>(this, &ADC::clockTick));

	std::string prefix = "@t\t@X\t[ADC::" + std::to_string(id) + "]:\t";
	outputStr.init(prefix, verbose, 0, SST::Output::STDOUT);

	nanoTimeConverter = getTimeConverter("1ns");
	picoTimeConverter = getTimeConverter("1ps");

	inputEvent = NULL;
	lastSwitch = 0;
}


void ADC::setup() { }

/**
* @brief BRIEF.
* @details DETAILS
*/
void ADC::init(unsigned int phase){

    // Check event at input port received from connected component
    while (SST::Event* ev = inputLink->recvUntimedData()) {

        AnalogEvent* event = dynamic_cast<AnalogEvent*>(ev);

        if (!event) { //check for correct data type
            outputStr.fatal(CALL_INFO, -1, "Error in %s: Expected input port to be connected to a component with analog output. Please check that components connected to %s have the correct output type\n", getName().c_str(), getName().c_str());
        } 
		if (event->getMax() > maxVin) { //check for correct vmax of input vector
			outputStr.fatal(CALL_INFO, -1, "Error in %s: Analog data received at input port has a vmax of %f V, while the internal vmax is %f V. Please make sure that vmax of connected components is smaller or equal to the \"maxVin/vout\" parameter\n", getName().c_str(), event->getMax(), maxVin);
		}
		if (event->getData().size() != size) { //check for correct size of input vector
			outputStr.fatal(CALL_INFO, -1, "Error in %s: Data received at input port has a size of %u, while the expected size is %u. Please make sure that size of connected components is identical to the \"size\" parameter \n", getName().c_str(), int(event->getData().size()), size);
		}
		if (event) {
			auto output = this->convert(event->getData());
			outputLink->sendUntimedData(new DigitalEvent(event->getId(), resolution, output));
		}
	}
}


void ADC::finish() {

	updateEnergy();
}

/**
* @brief BRIEF.
* @details DETAILS
*/
bool ADC::clockTick(Cycle_t cycle) {

	if (inputEvent != NULL) {
		selfLink->send(latency, picoTimeConverter, inputEvent);
		inputEvent = NULL;
	}
	return false;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void ADC::handleInput(Event *ev) {

	outputStr.verbose(CALL_INFO, 2, 0, "event received\n ");

	delete inputEvent;
	
	inputEvent = ev;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void ADC::handleSelf(Event *ev) {

	AnalogEvent *input = static_cast<AnalogEvent *>(ev);
	
	outputStr.verbose(CALL_INFO, 2, 0, "event sent\n ");

	auto output = this->convert(input->getData());

	outputLink->send(new DigitalEvent(input->getId(), resolution, output));
	
	delete input;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
std::vector<uint64_t> ADC::convert(std::vector<double> input) {

	auto output = std::vector<uint64_t>(input.size(), 0);
	auto output_ = xt::adapt(output, {size});

	output_ = xt::cast<uint64_t>(xt::adapt(input, {size}) / ((maxVin - minVin) / ((pow(2, resolution) - 1) )));

	return output;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void ADC::updateEnergy() {

	SimTime_t currentTime = getCurrentSimTime(nanoTimeConverter);
	SimTime_t elapsedTime = currentTime - lastSwitch;

	energyConsumption->addData( elapsedTime * size * conversionEnergy );

	lastSwitch = currentTime;
}

} // namespace BYOD
} // namespace SST