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

#include "clements.h"

namespace SST {
namespace BYOD {

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
clements::clements(ComponentId_t id, Params &params) : Component(id) {

	size = 				params.find<uint32_t>("size", 1);
	latency = 			params.find<uint32_t>("latency", 1);
	verbose = 			params.find<uint32_t>("verbose", 0);
	opticalLoss = 		params.find<double>("opticalLoss", 0.0);

	inputDataLink = 		configureLink("inputData",	new Event::Handler<clements>(this, &clements::handleDataInput));
	inputWeightLink = 		configureLink("inputWeight",	new Event::Handler<clements>(this, &clements::handleWeightInput));
	selfLink = 			configureSelfLink("selfLink", new Event::Handler<clements>(this, &clements::handleSelf));
	outputLink = 		configureLink("output");

	modulator = 		loadUserSubComponent<basicModulator>("modulator");
	energyConsumption = registerStatistic<double_t>("energyMesh");

	nanoTimeConverter = getTimeConverter("1ns");
	picoTimeConverter = getTimeConverter("1ps");

	std::string prefix = "@t\t@X\t[SVDMESH::" + std::to_string(id) + "]:\t";
	outputStr.init(prefix, verbose, 0, SST::Output::STDOUT);

	if (!modulator) {
		outputStr.fatal(
			CALL_INFO, -1,
			"Unable to load basicModulator subcomponent; "
			"check that 'modulator' slot is filled in input.\n");
	}

	transfer_matrix = xt::eye(size);
	even_dc_couplers = dc_coupler(0);
	odd_dc_couplers = dc_coupler(1);
	phases = xt::zeros<double>({size * size});
	lastSwitch = 0;
}


void clements::setup() { }

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::init(unsigned int phase){

    while (SST::Event* ev = inputDataLink->recvUntimedData()) 
	{
		
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
		if(event) {
			//TODO!!!
		}
	}

	while (SST::Event* ev = inputWeightLink->recvUntimedData()) 
	{
		
		AnalogEvent* event2 = dynamic_cast<AnalogEvent*>(ev);

        if (!event2) { //check for correct data type of input vector
            outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Expected input port to be connected to an Element with analog output. Please check that components connected to %s have the correct output type\n", 
				getName().c_str(), getName().c_str());
        } 
		if (event2->getData().size() != (size * size)) { //check for correct size of input vector
			outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Data received at input port has a size of %u, while the internal size is %u. Please make sure that size of connected components is identical by setting the \"size\" parameter \n", 
				getName().c_str(), int(event2->getData().size()), (size * size));
		}
		if(event2) {
				xt::xarray<double> voltages = xt::adapt(event2->getData(), {size * size});
				phases = modulator->getPhasesFromVoltages(voltages);
				reconstructUnitaryMatrix();
				std::cout << transfer_matrix << " here's your matrix" << std::endl;
		}
	}

}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::finish() {

	updateEnergy();
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::handleDataInput(Event *ev) {

	selfLink->send(latency, picoTimeConverter, ev);
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::handleWeightInput(Event *ev) {

	AnalogEvent *input = static_cast<AnalogEvent *>(ev);
	updateEnergy();
	xt::xarray<double> voltages = xt::adapt(input->getData(),{size * size});
	phases = modulator->getPhasesFromVoltages(voltages);
	reconstructUnitaryMatrix();

	delete input;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::handleSelf(Event *ev) {

	ComplexEvent *input = static_cast<ComplexEvent *>(ev);
	xt::xarray<std::complex<double>> signal = xt::adapt(input->getReal(),{size}) + jj * xt::adapt(input->getImag(),{size});
	
	signal = xt::linalg::dot(transfer_matrix, signal); //TODO add optical loss!!!
	
	std::vector<double> signal_real(xt::real(signal).begin(), xt::real(signal).end());
	std::vector<double> signal_imag(xt::imag(signal).begin(), xt::imag(signal).end());
    ComplexEvent* output = new ComplexEvent(input->getId(), 3.0, signal_real, signal_imag);
	outputLink->send(output);

	delete input;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::reconstructUnitaryMatrix() {

	transfer_matrix = xt::eye(size);

	int32_t index_counter = 0;

	for(int i = 0; i < size; i++) {
 
		phase_shifter(i%2, index_counter, 1.0);
		transfer_matrix = xt::linalg::tensordot(mesh_layer, transfer_matrix, 1);
		index_counter += int(floor((size - i%2) / 2));

		if(i%2 == 0)
			transfer_matrix = xt::linalg::tensordot(even_dc_couplers, transfer_matrix, 1);

		else
			transfer_matrix = xt::linalg::tensordot(odd_dc_couplers, transfer_matrix, 1);

		phase_shifter(i%2, index_counter, 1.0);
		transfer_matrix = xt::linalg::tensordot(mesh_layer, transfer_matrix, 1);
		index_counter += int(floor((size - i%2) / 2));

		if(i%2 == 0)
			transfer_matrix = xt::linalg::tensordot(even_dc_couplers, transfer_matrix, 1);

		else
			transfer_matrix = xt::linalg::tensordot(odd_dc_couplers, transfer_matrix, 1);

	}

	diagonals(index_counter);
	transfer_matrix = xt::linalg::tensordot(mesh_layer, transfer_matrix, 1);
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
xt::xarray<std::complex<double>> clements::dc_coupler(int32_t start) {

	xt::xarray<std::complex<double>> out = xt::eye(size);

	for(int i = start; i < size - 1; i +=2 )
		xt::view(out, xt::range(i,i+2), xt::range(i,i+2)) = beam_splitter_matrix;

	return out; 
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::phase_shifter(int32_t start, int32_t start_index, double multiplier = 1.0) {

	mesh_layer = xt::eye(size);
	int32_t counter = start_index;

	for(int i = start; i < size - 1; i +=2 ) {
		mesh_layer(i, i) = exp(phases(counter) * multiplier * jj);
		counter++;
	}
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::diagonals(int32_t start_index) {

	mesh_layer = xt::eye(size);

	for(int i = 0; i < size; i++)
		mesh_layer(i,i) = exp(phases(start_index + i) * jj);

}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void clements::updateEnergy() {

	SimTime_t currentTime = getCurrentSimTime(picoTimeConverter);
	SimTime_t elapsedTime = currentTime - lastSwitch;
	energyConsumption->addData(elapsedTime * modulator->staticModulatorPower + modulator->switchingEnergy);

	lastSwitch = currentTime;
}
} // namespace BYOD
} // namespace SST