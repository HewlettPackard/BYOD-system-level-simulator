#ifndef _ampltiudeModulator_H
#define _amplitudeModulator_H

#include "../Events/complex_event.h"
#include "../Events/analog_event.h"
#include "Submodules/modulators.h"

#include <math.h>
#include <util.h>

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
class amplitudeModulator : public SST::Component {
  public:
	SST_ELI_REGISTER_COMPONENT(
		amplitudeModulator,
		"byod",
		"AmplitudeModulator",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"An array of optical amplitude modulator in the BYOD library",
		COMPONENT_CATEGORY_UNCATEGORIZED
	);

	SST_ELI_DOCUMENT_PARAMS(
		{"size", 			"(uint32) number of amplitude modulators", "1"},
		{"latency", 		"(uint32) processing latency in ps", "1"},
		{"verbose", 		"(uint32) level of debugging output", "0"},
		{"laserPower", 		"(double) optical power provided by each single laser in the modulator array in W", "1"},
		{"laserWpe", 		"(double) wall-plug efficiency by each single laser in the modulator array", "1"},
		{"insertionLoss", 	"(double) optical insertion loss by each single laser in the modulator array", "1"},
	);

	SST_ELI_DOCUMENT_PORTS(
		{"input", 			"receiving input signal (analog)", {"sst.byod.analogEvent"}},
		{"output", 			"sending output signal (complex)", {"sst.byod.complexEvent"}}
	);

	SST_ELI_DOCUMENT_STATISTICS(
		{"energyModulator", "Cumulative energy consumption of amplitude modulator", "pJ", 1}
	);

	SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
		{"modulator",		"Subcomponent slot for the optic modulator", "SST::byod::BasicModulator"}
	);

	amplitudeModulator(ComponentId_t id, Params &params);
	~amplitudeModulator() {};

	void setup();
	void finish();
	virtual void init(unsigned int phase) override;

	void handleInput(Event *ev);
	void handleSelf(Event *ev);
	void updateEnergy();

  private:
	/** IO *****************************************************/

	Output outputStr;

	/** Link/Port **********************************************/

	Link *inputLink;
	Link *outputLink;
	Link *selfLink;

	/** Parameters ********************************************/

	uint32_t latency;
	uint32_t size;
	double laserPower;
	double laserWpe;
	double opticalLoss;

	/** Statistics *********************************************/

	Statistic<double_t> *energyConsumption;

	/** operation *********************************************/

	TimeConverter *nanoTimeConverter;
	TimeConverter *picoTimeConverter;
	SimTime_t lastSwitch;
	SST::BYOD::basicModulator* modulator;
	std::vector<double> empty;
	uint32_t verbose;
	double modulatorEnergy;
};
} // namespace BYOD
} // namespace SST

#endif