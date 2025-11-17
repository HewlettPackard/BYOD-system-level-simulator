#ifndef _MODULATORS_H
#define _MODULATORS_H

#include "../../Events/digital_event.h"
#include "../../Events/analog_event.h"

#include <cmath>
#include <util.h>
#include <cstdint>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/subcomponent.h>

#include <xtensor/containers/xarray.hpp>
#include <xtensor-blas/xlinalg.hpp>
#include <xtensor/containers/xadapt.hpp>
#include <xtensor/io/xio.hpp>
#include <xtensor/core/xvectorize.hpp>

namespace SST {
namespace BYOD {

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
class basicModulator : public SST::SubComponent {
  public:

  	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::BYOD::basicModulator)

	basicModulator(ComponentId_t id, Params& params) : SubComponent(id) { }
    virtual ~basicModulator() { }

	virtual xt::xarray<double> getPhasesFromVoltages(xt::xarray<double> voltages) =0;
    virtual xt::xarray<double> getAmplitudesFromVoltages(xt::xarray<double> voltages) =0;
	virtual void updateEnergy(xt::xarray<double> voltages) =0;

    double staticModulatorPower; //variable to store current static power drain
    double switchingEnergy; //variable to store 
    uint32_t size; //number of modulators
    xt::xarray<double> previousVoltages; //variables to store previous set of voltages applied to the modulator, needed to compute switching energy

	// Serialization
    basicModulator() {};
    ImplementVirtualSerializable(SST::BYOD::basicModulator);
};

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
class thermoOpticModulator : public basicModulator {
public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        thermoOpticModulator,
        "byod",
        "thermoOpticModulator",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Subcomponent for an array of thermo-optic modulators in the BYOD library",
        SST::BYOD::basicModulator
    )

    // Other ELI macros as needed for parameters, ports, statistics, and subcomponent slots
    SST_ELI_DOCUMENT_PARAMS( 
		{ "resistance",     "Ohmic resistance of the heater in Ohm", "1" },
		{ "p_pi",           "Electric power needed to induces a pi phase shift in W", "1" },
        { "size",           "Size of the modulator array", "1"},
	)

    thermoOpticModulator(ComponentId_t id, Params& params);
    ~thermoOpticModulator();

	xt::xarray<double> getPhasesFromVoltages(xt::xarray<double> voltages) override;
    xt::xarray<double> getAmplitudesFromVoltages(xt::xarray<double> voltages) override;
	void updateEnergy(xt::xarray<double> voltages) override;

    // serialization
    thermoOpticModulator() : basicModulator() {};
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::BYOD::thermoOpticModulator);

private:
    double resistance;
	double p_pi;
};

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
class kerrModulator : public basicModulator {
public:

    // Register this subcomponent with SST and tell SST that it implements the 'basicSubComponentAPI' API
    SST_ELI_REGISTER_SUBCOMPONENT(
            kerrModulator,
            "byod",
            "kerrModulator",
            SST_ELI_ELEMENT_VERSION(1,0,0),
            "Subcomponent for a kerr Modulator",
            SST::BYOD::basicModulator
    )

    // Other ELI macros as needed for parameters, ports, statistics, and subcomponent slots
    SST_ELI_DOCUMENT_PARAMS( 
		{ "resistance", "Amount to increment by", "1" },
		{ "p_pi", "Amount to increment by", "1" }
	)

    kerrModulator(ComponentId_t id, Params& params);
    ~kerrModulator();

	xt::xarray<double> getPhasesFromVoltages(xt::xarray<double> voltages) override;
    xt::xarray<double> getAmplitudesFromVoltages(xt::xarray<double> voltages) override;
	void updateEnergy(xt::xarray<double> voltages) override;

    // serialization
    kerrModulator() : basicModulator() {};
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::BYOD::kerrModulator);

private:
    double resistance;
	double p_pi;
};
} //namespace BYOD
} //namespace SST

#endif