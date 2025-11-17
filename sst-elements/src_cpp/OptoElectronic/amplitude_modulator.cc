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

#include "amplitude_modulator.h"

namespace SST {
namespace BYOD {

/**
* @brief BRIEF.
* @details DETAILS
*/
amplitudeModulator::amplitudeModulator(ComponentId_t id, Params &params) : Component(id) {

	size = 				params.find<uint32_t>("size", 1);
	latency = 			params.find<uint32_t>("latency", 1);
	verbose = 			params.find<uint32_t>("verbose", 0);
	laserPower = 		params.find<double>("laserPower", 1.0);
	laserWpe = 			params.find<double>("laserWpe", 0.2);
	opticalLoss = 		params.find<double>("opticalLoss", 0.0);

	energyConsumption = registerStatistic<double_t>("energyModulator");

	inputLink = 		configureLink("input",	new Event::Handler<amplitudeModulator>(this, &amplitudeModulator::handleInput));
	selfLink = 			configureSelfLink("selfLink", new Event::Handler<amplitudeModulator>(this, &amplitudeModulator::handleSelf));
	outputLink = 		configureLink("output");
	modulator = 		loadUserSubComponent<basicModulator>("modulator");

	std::string prefix = "@t\t@X\t[AMPLITUDE_MODULATOR::" + std::to_string(id) + "]:\t";
	outputStr.init(prefix, verbose, 0, SST::Output::STDOUT);

	if (!modulator) {
		outputStr.fatal(
			CALL_INFO, -1,
			"Unable to load basicModulator subcomponent; "
			"check that 'modulator' slot is filled in input.\n");
	}

	nanoTimeConverter = getTimeConverter("1ns");
	picoTimeConverter = getTimeConverter("1ps");

	empty = std::vector<double>(size, 0);
	lastSwitch = 0;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void amplitudeModulator::setup() { }

/**
* @brief BRIEF.
* @details DETAILS
*/
void amplitudeModulator::init(unsigned int phase){

    while (SST::Event* ev = inputLink->recvUntimedData()) {

		AnalogEvent* event = dynamic_cast<AnalogEvent*>(ev);

        if (!event) { //check for correct data type of input vector
            outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Expected input port to be connected to an Element with analog output. Please check that components connected to %s have the correct output type\n", 
				getName().c_str(), getName().c_str());
        } 
		//if (event->getVmax() != resolution) { //check for correct resolution/vmax of input vector
		//	output.fatal(CALL_INFO, -1, "Error in %s: Analog data received at input port has a vmax of %f V, while the internal vmax is %f V. Please make sure that vmax of connected components is identical by setting the \"max_vin/vout\" parameter\n", getName().c_str(), event->getResolution(), resolution);
		//}
		if (event->getData().size() != size) { //check for correct size of input vector
			outputStr.fatal(
				CALL_INFO, -1,
				"Error in %s: Data received at input port has a size of %u, while the expected size is %u. Please make sure that size of connected components is identical to the \"size\" parameter \n", 
				getName().c_str(), int(event->getData().size()), size);
		}
		if(event) {

			auto XTinputData = xt::adapt(event->getData(), {size});
			XTinputData = sqrt(laserPower) * sqrt(1 - opticalLoss) * XTinputData; //TODO!!!
			outputLink->sendUntimedData(new ComplexEvent(event->getId(), 0.0, event->getData(), empty));
		}
	}
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void amplitudeModulator::finish() {

	updateEnergy();
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void amplitudeModulator::handleInput(Event *ev) {

	selfLink->send(latency, picoTimeConverter, ev);
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void amplitudeModulator::handleSelf(Event *ev) {

	AnalogEvent *input = static_cast<AnalogEvent *>(ev);

	updateEnergy();

	xt::xarray<double> XTinputData = xt::adapt(input->getData(), {size});
	XTinputData = sqrt(laserPower) * sqrt(1 - opticalLoss) * modulator->getAmplitudesFromVoltages(XTinputData);
	std::vector<double> output(XTinputData.begin(), XTinputData.end());

	outputLink->send(new ComplexEvent(input->getId(), 0.0, output, empty));
	
	delete input;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void amplitudeModulator::updateEnergy() {

	SimTime_t currentTime = getCurrentSimTime(picoTimeConverter);
	SimTime_t elapsedTime = currentTime - lastSwitch;

	energyConsumption->addData( elapsedTime * (size * laserPower / laserWpe + modulator->staticModulatorPower) + modulator->switchingEnergy);

	lastSwitch = currentTime;
}
} // namespace BYOD
} // namespace SST