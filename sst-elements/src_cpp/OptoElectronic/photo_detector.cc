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

#include "photo_detector.h"

namespace SST {
namespace BYOD {

/**
* @brief BRIEF.
* @details DETAILS
*/
photoDetector::photoDetector(ComponentId_t id, Params &params) : Component(id) {

	pdType = 			parseDetectorModeStr(params.find<std::string>("pdType", "Single"));
	size = 				params.find<uint32_t>("size", 1);
	verbose = 			params.find<uint32_t>("verbose", 1);
	latency = 			params.find<uint32_t>("latency", 1);
	sensitivity = 		params.find<double>("sensitivity", 1);
	tiaGain = 			params.find<double>("tiaGain", 1);
	maxVout = 			params.find<double>("maxVout", 1);
	tiaPower = 			params.find<double>("tiaPower", 0.0003);
	darkCurrent = 		params.find<double>("darkCurrent", 0.000000001);
	biasVoltage = 		params.find<double>("biasVoltage", 1);

	inputLink = 		configureLink("input",	new Event::Handler<photoDetector>(this, &photoDetector::handleInput));
	selfLink = 			configureSelfLink("selfLink", new Event::Handler<photoDetector>(this, &photoDetector::handleSelf));
	outputLink = 		configureLink("output");
	
	energyConsumption = registerStatistic<double_t>("energyPhotoDetector");

	std::string prefix = "@t\t@X\t[PHOTODETECTOR::" + std::to_string(id) + "]:\t";
	outputStr.init(prefix, verbose, 0, SST::Output::STDOUT);

	nanoTimeConverter = getTimeConverter("1ns");
	picoTimeConverter = getTimeConverter("1ps");

	currentPdPower = 0;
	lastSwitch = 0;
}


void photoDetector::setup() { }

/**
* @brief BRIEF.
* @details DETAILS
*/
void photoDetector::init(unsigned int phase){

    // Check if an event is received. recvUntimedData returns nullptr if no event is available
    while (SST::Event* ev = inputLink->recvUntimedData()) {

        ComplexEvent* event = dynamic_cast<ComplexEvent*>(ev);
        if (!event) { //check for correct data type of input vector

            outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Expected input port to be connected to an Element with complex output. Please check that components connected to %s have the correct output type\n", 
				getName().c_str(), getName().c_str());
        } 
		if (event->getReal().size() != size) { //check for correct size of input vector

			outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Data received at input port has a size of %u, while the expected size is %u. Please make sure that size of connected components is identical to the \"size\" parameter \n", 
				getName().c_str(), int(event->getReal().size()), size);
		}
		if(event) { //TODO!!!

			std::vector<double> signal_out(size, 0);
			outputLink->sendUntimedData(new AnalogEvent(event->getId(), 1.0, signal_out));
		}
	}
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void photoDetector::finish() {

	updateEnergy();
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void photoDetector::handleInput(Event *ev) {

	selfLink->send(latency, picoTimeConverter, ev);
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void photoDetector::handleSelf(Event *ev) {

	ComplexEvent *input = static_cast<ComplexEvent *>(ev);
	xt::xarray<std::complex<double>> signal = xt::adapt(input->getReal(),{size}) + jj * xt::adapt(input->getImag(),{size});

	switch(pdType) {
		case(DetectorMode::Single):
			signal = tiaGain * sensitivity * xt::square(xt::abs(signal));
			updateEnergy();
			break;
		default:
			break;
	}

	std::vector<double> signal_out(xt::real(signal).begin(), xt::real(signal).end()); 
    AnalogEvent* output = new AnalogEvent(input->getId(), 3.0, signal_out); //TODO!!!
	outputLink->send(output);

	delete input;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void photoDetector::updateEnergy() {

	SimTime_t currentTime = getCurrentSimTime(picoTimeConverter);
	SimTime_t elapsedTime = currentTime - lastSwitch;

	energyConsumption->addData( elapsedTime * size * (darkCurrent * biasVoltage + tiaPower) );

	lastSwitch = currentTime;
}
} // namespace BYOD
} // namespace SST