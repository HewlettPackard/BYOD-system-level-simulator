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

#include "dac.h"

namespace SST {
namespace BYOD {

/**
* @brief BRIEF.
* @details DETAILS
*/
DAC::DAC(ComponentId_t id, Params &params) : Component(id) {

	dacType = 			parseDACStr(params.find<std::string>("dacType", "R2R"));
	size = 				params.find<uint32_t>("size", 8);
	verbose = 			params.find<uint32_t>("verbose", 0);
	latency = 			params.find<uint32_t>("latency", 1);
	resolution = 		params.find<uint32_t>("resolution", 8);
	element = 			params.find<double>("element", dacType == DACType::R2R ? 5e3 : 1e-12);
	minVout = 			params.find<double>("minVout", 0.0);
	maxVout = 			params.find<double>("maxVout", 1.0);
	controllerEnergy = 	params.find<double>("controllerEnergy", 1);
	frequency = 		params.find<UnitAlgebra>("frequency", "1GHz");

	inputlink = 		configureLink("input",	new Event::Handler<DAC>(this, &DAC::handleInput));
	outputlink = 		configureLink("output");
	selflink = 			configureSelfLink("selflink", new Event::Handler<DAC>(this, &DAC::handleSelf));
	energyConsumption = registerStatistic<double_t>("energyDAC");

	std::string prefix = "@t\t@X\t[DAC::" + std::to_string(id) + "]:\t";
	outputStr.init(prefix, verbose, 0, SST::Output::STDOUT);
	
	registerClock(frequency, new Clock::Handler<DAC>(this, &DAC::clockTick));
	glockPeriod = 1 / frequency.getDoubleValue() * 1e12;

	params.find_array("energyPerValue", energyPerValue);

	if (dacType == DACType::R2R) //set conductance for R2R ladder cells
		element = 1 / element / 2;
	if(dacType == DACType::CUSTOM && energyPerValue.size() != std::pow(2, resolution)) { //use and check user-provided energyPerValue

		outputStr.output(
			CALL_INFO, 
			"Warning in %s: Size of energyPerValue is %lu, while the internal size is %f. EnergyPerValue will be set to zero \n", 
			getName().c_str(), energyPerValue.size(), std::pow(2, resolution));
		energyPerValue = std::vector<double>(std::pow(2, resolution), 0.0);
	}
	else if(dacType !=  DACType::CUSTOM) //pre-compute the energy per Value for R2R and C2C DAC
		precomputeEnergy();	



	nanoTimeConverter = getTimeConverter("1ns");
	picoTimeConverter = getTimeConverter("1ps");

	inputEvent = NULL;
	lastSwitch = 0;
}

void DAC::setup() { }

/**
* @brief BRIEF.
* @details DETAILS
*/
void DAC::init(unsigned int phase){

    // Check if an event is received. recvUntimedData returns nullptr if no event is available
    while (SST::Event* ev = inputlink->recvUntimedData()) {

        DigitalEvent* event = dynamic_cast<DigitalEvent*>(ev);

        if (!event) {
            outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Expected input port to be connected to an element with digital output. Please check that components connected to %s have the correct output type\n", 
				getName().c_str(), getName().c_str());
        } 
		if (event->getResolution() != resolution) {
			outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Data received at input port has a resolution of %u, while the expected resolution is %u. Please make sure that the resolution of connected components is identical to the \"resolution\" parameter \n", 
				getName().c_str(), event->getResolution(), resolution);
		}
		if (event->getData().size() != size) { //check for correct size of input vector
			outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Data received at input port has a size of %u, while the expected size is %u. Please make sure that size of connected components is identical to the \"size\" parameter \n", 
				getName().c_str(), int(event->getData().size()), size);
		}
		if(event) {
			xt::xarray<double> voltages_inp = xt::adapt(event->getData(), {size});
			auto voltages_out = this->convert(voltages_inp);
			currentEnergy = currentForConversion(voltages_inp) + controllerEnergy * size;
			outputlink->sendUntimedData(new AnalogEvent(event->getId(), maxVout, xarray2vector<double>(voltages_out)));
		}
	}
}


void DAC::finish() {

	updateEnergy();
}

/**
* @brief BRIEF.
* @details DETAILS
*/
bool DAC::clockTick(Cycle_t cycle) {

	if (inputEvent != NULL) {
		selflink->send(latency, picoTimeConverter, inputEvent);
		inputEvent = NULL;
	}
	return false;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void DAC::handleInput(Event *ev) {

	delete inputEvent;
	inputEvent = ev;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void DAC::handleSelf(Event *ev) {

	DigitalEvent *input = static_cast<DigitalEvent *>(ev);
	xt::xarray<double> voltages_inp = xt::adapt(input->getData(), {size});

	updateEnergy();
	currentEnergy = currentForConversion(voltages_inp) + controllerEnergy * size;

	auto voltages_out = this->convert(voltages_inp);

	outputlink->send(new AnalogEvent(input->getId(), 0, xarray2vector<double>(voltages_out)));

	delete input;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
xt::xarray<double> DAC::convert(xt::xarray<double> input) {

	//std::cout << "adlevls" << input << std::endl;

	input = (maxVout - minVout) / (std::pow(2, resolution) - 1) * input;
	return input;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void DAC::updateEnergy() { 

	SimTime_t currentTime = getCurrentSimTime(picoTimeConverter);
	SimTime_t elapsedTime = currentTime - lastSwitch;
	lastSwitch = currentTime;

	energyConsumption->addData(elapsedTime / glockPeriod * currentEnergy);
}

/**
* @brief BRIEF.
* @details DETAILS
*/
double_t DAC::currentForConversion(xt::xarray<int32_t> value) {

	double_t out = 0.0;
	for (size_t i = 0; i < size; ++i) 
		out += energyPerValue[value(i)];
	
	return out;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
double_t DAC::convertEnergy(int32_t inp) {

	xt::xarray<int8_t> inp_bits = value2bits(inp);
	xt::xarray<double_t> node_voltages = voltagesAtEachNode(inp_bits * maxVout);
	if (dacType == DACType::C2C)
		return xt::sum((maxVout - node_voltages) * xt::flip(inp_bits, 0))[0] * maxVout * element;
	else
		return xt::sum((maxVout - node_voltages) * xt::flip(inp_bits, 0))[0] * maxVout * element * glockPeriod;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
xt::xarray<double_t> DAC::voltagesAtEachNode(xt::xarray<int8_t> inp) {
	return xt::flip(xt::linalg::dot(conversionNodeMatrix, inp), 0);
}

/**
* @brief BRIEF.
* @details DETAILS
*/
xt::xarray<int8_t> DAC::value2bits(int32_t x) {
	if (x > pow(2, resolution) - 1) {
		outputStr.fatal(CALL_INFO, -1,
					 "%d is too large to be represented by binary "
					 "representation with the given bit resolution %u",
					 x, resolution);
		throw std::invalid_argument(
			"x is too large to be represented by binary representation with "
			"the given number of bits\n");
	}

	xt::xarray<int8_t> bits = xt::zeros<int8_t>({resolution});

	for (std::size_t i = 0; i < resolution; ++i) {
		bits(i) = x % 2;
		x /= 2;
	}

	return bits;
}

/**
* @brief BRIEF.
* @details DETAILS
*/
void DAC::precomputeEnergy() {
	// build initial matrix for conversion nodes
	xt::xarray<double_t> M = xt::zeros<double_t>({resolution, resolution});
	M(0, 0) = 4;
	M(0, 1) = -2;

	for (size_t i = 1; i < resolution - 1; ++i) {
		M(i, i - 1) = -2;
		M(i, i) = 5;
		M(i, i + 1) = -2;
	}

	M(resolution - 1, resolution - 2) = -2;
	M(resolution - 1, resolution - 1) = 3;
	// inverse
	conversionNodeMatrix = xt::linalg::inv(M);

	energyPerValue = std::vector<double>(std::pow(2, resolution), 0.0);
	for(int i = 0; i < energyPerValue.size(); i++) {
		energyPerValue[i] = convertEnergy(i);
	}
}
} // namespace BYOD
} // namespace SST