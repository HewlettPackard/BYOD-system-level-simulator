#ifndef _ADC_H
#define _ADC_H

#include "../Events/digital_event.h"
#include "../Events/analog_event.h"

#include <cstdint>
#include <math.h>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/unitAlgebra.h>

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
*/
class ADC : public SST::Component {
  public:
	SST_ELI_REGISTER_COMPONENT(
		ADC,
		"byod",
		"ADC",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"Analog-to-digital-converter (ADC) in BYOD library",
		COMPONENT_CATEGORY_UNCATEGORIZED
	);

	SST_ELI_DOCUMENT_PARAMS(
		{"resolution", 		"(uint32) bit resolution of the ADC", "1"},
		{"size", 			"(uint32) size of the input/output vector", "1"},
		{"latency", 		"(uint32) latency of ADC coversion in ps", "1"},
		{"verbose", 		"(uint32) Output verbosity. The higher verbosity, the more debug info", "0"},
		{"minVin", 			"(double) mimimum value of the input vector", "0"},
		{"maxVin", 			"(double) maximum value of the output vector", "0"},
		{"conversionEnergy","(double) energy per ADC conversion in pJ", "1"},
		{"frequency", 		"(string) clock frequency", "1GHz"},
	);

	SST_ELI_DOCUMENT_PORTS(
		{"input", 			"receiving input signal (analog)", {"sst.byod.analogEvent"}},
		{"output", 			"sending output signal (digital)", {"sst.byod.digitalEvent"}}
	);

	SST_ELI_DOCUMENT_STATISTICS(
		{"energyADC", 		"Cumulative energy consumption of ADC", "pJ", 1}
	);

	ADC(ComponentId_t id, Params &params);
	~ADC() {};

	void setup();
	void finish();
	virtual void init(unsigned int phase) override;

	bool clockTick(Cycle_t cycle);
	void handleInput(Event *ev);
	void handleSelf(Event *ev);
	std::vector<uint64_t> convert(std::vector<double> input);
	void updateEnergy();

  private:
	/** IO *****************************************************/

	Output outputStr;

	/** Link/Port **********************************************/

	Link *inputLink;
	Link *outputLink;
	Link *selfLink;

	/** Parameters ********************************************/

	UnitAlgebra frequency;
	uint32_t latency;
	uint32_t resolution;
	uint32_t size;
	uint32_t verbose;
	double minVin;
	double maxVin;
	double conversionEnergy;

	/** Statistics *********************************************/

	Statistic<double> *energyConsumption;

	/** operation *********************************************/

	Event *inputEvent;
	SimTime_t lastSwitch;
	TimeConverter *nanoTimeConverter;
	TimeConverter *picoTimeConverter;

};
} // namespace BYOD
} // namespace SST

#endif