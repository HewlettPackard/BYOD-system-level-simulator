#ifndef _XXX_H
#define _XXX_H

#include "Events/digital_event.h"
#include "Events/analog_event.h"
#include "Events/complex_event.h"

#include <cstdint>
#include <math.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/unitAlgebra.h>
#include <xtensor/containers/xarray.hpp>
#include <cmath>
#include <util.h>
#include <xtensor-blas/xlinalg.hpp>
#include <xtensor/containers/xadapt.hpp>
#include <xtensor/io/xio.hpp>
#include <xtensor/core/xvectorize.hpp>

namespace SST {
namespace BYOD {


class XXX : public SST::Component {
  public:
	SST_ELI_REGISTER_COMPONENT(
		XXX,	   // class
		"byod", // component library
		"XXX",  // component name
		SST_ELI_ELEMENT_VERSION(1, 0, 0), // version
		"XXX",		 // description
		COMPONENT_CATEGORY_UNCATEGORIZED
	);

	SST_ELI_DOCUMENT_PARAMS(
		{"frequency", "conversion frequency (with unit)", "1"},
		{"controller_energy", "static controller energy usage per convert in pJ", "1"},
	);

	SST_ELI_DOCUMENT_PORTS(
		{"inputlink", "receiving input signal (digital)", {"sst.byod.dataEvent"}},
		{"outputlink", "sending output signal (analog)", {"sst.byod.dataEvent"}}
	);

	SST_ELI_DOCUMENT_STATISTICS(
		{"conversions", "Number of overall ADC conversions for this ADC bank", "count", 1},
		{"energyDAC", "Cumulative energy consumption of DAC", "pJ", 1}
	);

	XXX(ComponentId_t id, Params &params);
	~XXX() {};

	void setup();
	void finish();
	virtual void init(unsigned int phase) override;

	bool clockTick(Cycle_t cycle);

	/**
	 * @brief Event handler for receiving digital input signal
	 */
	void handle_input_sync(Event *ev);

	void handle_self(Event *ev);

  private:
	/** IO *****************************************************/

	Output output;

	/** Link/Port **********************************************/

	TimeConverter *nanoTimeConverter;
	TimeConverter *picoTimeConverter;
	Link *inputlink;
	Link *outputlink;
	Link *selflink;

	/** Parameters ********************************************/
	// whether component is clocked
	// input size
	uint32_t size;
	// frequency for conversion
	UnitAlgebra frequency;
	// component latency [ps]
	SimTime_t latency;
	// unit capacitance or resistance

	uint32_t resolution;

	float_t controller_energy; // in pJ
	//float_t area;					  // in nm

	/** Statistics *********************************************/
	Statistic<uint64_t> *num_conversions;
	Statistic<float_t> *stat_energy_consumption;

	/** operation *********************************************/
	bool busy;
	Event *inputEvent;
	// inverse of conversion node matrix

};
} // namespace BYOD
} // namespace SST

#endif