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

#include "sst_config.h"
#include "modulators.h"

namespace SST {
namespace BYOD {

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
thermoOpticModulator::thermoOpticModulator(ComponentId_t id, Params &params) : basicModulator(id, params) {

	resistance = 	params.find<double>("resistance", 200);
	p_pi = 			params.find<double>("p_pi", 200);
	size = 			params.find<uint32_t>("size", 1);

	// initialize the power and voltages for the modulator
	staticModulatorPower = 0;
	switchingEnergy = 0;
	previousVoltages = xt::zeros<double>({size});
}


thermoOpticModulator::~thermoOpticModulator() { }

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
xt::xarray<double> thermoOpticModulator::getPhasesFromVoltages(xt::xarray<double> voltages) {

	updateEnergy(voltages);
	voltages = xt::pow(voltages, 2) / resistance / p_pi * xt::numeric_constants<double>::PI;
	return voltages;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
xt::xarray<double> thermoOpticModulator::getAmplitudesFromVoltages(xt::xarray<double> voltages) {

	updateEnergy(voltages);
	voltages = xt::cos(xt::pow(voltages, 2) / resistance / p_pi * xt::numeric_constants<double>::PI);// * xt::cos(xt::pow(voltages, 2) / resistance / p_pi * xt::numeric_constants<double>::PI);
	return voltages;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void thermoOpticModulator::updateEnergy(xt::xarray<double> voltages) {

	//switchingEnergy = 0; //not implemented right now
	staticModulatorPower = xt::sum(xt::pow(voltages, 2) / resistance)();
	//previousVoltages = voltages; //not implemented right now
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void thermoOpticModulator::serialize_order(SST::Core::Serialization::serializer& ser) {
    
	SubComponent::serialize_order(ser);
    SST_SER(resistance);
	SST_SER(p_pi);
	SST_SER(size);
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
kerrModulator::kerrModulator(ComponentId_t id, Params &params) : basicModulator(id, params) {

	resistance = 	params.find<double>("resistance", 200);
	p_pi = 			params.find<double>("p_pi", 200);
	size = 			params.find<uint32_t>("size", 1);

	// initialize the power and voltages for the modulator
	staticModulatorPower = 0;
	switchingEnergy = 0;
	previousVoltages = xt::zeros<double>({size});
}

kerrModulator::~kerrModulator() { }

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
xt::xarray<double> kerrModulator::getPhasesFromVoltages(xt::xarray<double> voltages) {

	//auto signal = xt::adapt(voltages, {voltages.size()});
	//signal = xt::pow(signal, 2) / resistance / p_pi * xt::numeric_constants<double>::PI;
	//signal = xt::remainder(signal, 2 * xt::numeric_constants<double>::PI);
	voltages = 2 * xt::numeric_constants<double>::PI * voltages;

	return voltages;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
xt::xarray<double> kerrModulator::getAmplitudesFromVoltages(xt::xarray<double> voltages) {

	//auto signal = xt::adapt(voltages, {voltages.size()});
	updateEnergy(voltages);
	//signal = xt::cos(xt::pow(signal, 2) / resistance / p_pi * xt::numeric_constants<double>::PI) * xt::cos(xt::pow(signal, 2) / resistance / p_pi * xt::numeric_constants<double>::PI);
	//signal = xt::remainder(signal, 2 * xt::numeric_constants<double>::PI);
	//signal = 2 * xt::numeric_constants<double>::PI * signal;

	return voltages;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void kerrModulator::updateEnergy(xt::xarray<double> voltages) {

	//switchingEnergy = 0; //not implemented right now
	//staticModulatorPower = 0; //not implemented right now
	//previousVoltages = voltages; //not implemented right now
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void kerrModulator::serialize_order(SST::Core::Serialization::serializer& ser) {
    
	SubComponent::serialize_order(ser);
    SST_SER(resistance);
	SST_SER(p_pi);
	SST_SER(size);
}
} // namespace BYOD
} // namespace SST