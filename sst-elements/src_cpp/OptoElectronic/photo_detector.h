#ifndef _photoDetector_H
#define _photoDetector_H

#include "../Events/complex_event.h"
#include "../Events/analog_event.h"

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/unitAlgebra.h>

#include <complex>
#include <cmath>
#include <util.h>

#include <xtensor-blas/xlinalg.hpp>
#include <xtensor/containers/xadapt.hpp>
#include <xtensor/io/xio.hpp>
#include <xtensor/core/xvectorize.hpp>
#include <xtensor/containers/xarray.hpp>

namespace SST {
namespace BYOD {

enum DetectorMode { Single, Balanced }; //enum for representing the type of the photodetector

/**
* @brief BRIEF.
* @details DETAILS
*/
DetectorMode parseDetectorModeStr(std::string str) {

	std::transform(str.begin(), str.end(), str.begin(),
				   [](unsigned char c) { return std::tolower(c); });

	if (str == "single") {
		return DetectorMode::Single;
	} else if (str == "Balanced") {
		return DetectorMode::Balanced;
	} else {
		throw std::invalid_argument("Detector mode %s not supported. Supported modes are \"Single\", \"Balanced\" or \"BalancedComplex\" \n");
	}
}

/**
* @brief BRIEF.
* @details DETAILS
*/
class photoDetector : public SST::Component {

  public:
	SST_ELI_REGISTER_COMPONENT(
		photoDetector,
		"byod",
		"Photodetector",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"Photodector in the BYOD library",
		COMPONENT_CATEGORY_UNCATEGORIZED
	);

	SST_ELI_DOCUMENT_PARAMS(
		{"pdType", 				"(string) type of photodector Single/custom", "1"},
		{"size", 				"(uint32) size of the input/output vector", "1"},
		{"latency", 			"(uint32) processing latency in ps", "1"},
		{"verbose", 			"(uint32) level of debugging output", "0"},
		{"sensitivity", 		"(double) photodetector sensitivity in A/W", "1"},
		{"darkCurrent", 		"(double) photodetector dark current in A", "1"},
		{"biasVoltage", 		"(double) photodetector bias voltage in V", "1"},
		{"tiaGain", 			"(double) closed-loop TIA gain in V/A", "1"},
		{"tiaPower", 			"(double) static TIA power consumption in W", "1"},
	);

	SST_ELI_DOCUMENT_PORTS(
		{"input", 				"receiving input signal (complex)", {"sst.byod.complexEvent"}},
		{"output", 				"sending output signal (analog)", {"sst.byod.analogEvent"}}
	);

	SST_ELI_DOCUMENT_STATISTICS(
		{"energyPhotoDetector", "Cumulative energy consumption of Photodector", "pJ", 1}
	);

	photoDetector(ComponentId_t id, Params &params);
	~photoDetector() {};

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

	uint32_t size;
	uint32_t latency;
	uint32_t verbose;
	double sensitivity;
	double tiaGain;
	double tiaPower;
	double darkCurrent;
	double biasVoltage;
	double maxVout;
	DetectorMode pdType;

	/** Statistics *********************************************/
	
	Statistic<double> *energyConsumption;

	/** operation *********************************************/
	
	TimeConverter *nanoTimeConverter;
	TimeConverter *picoTimeConverter;
	SimTime_t lastSwitch;
	double currentPdPower;
	const std::complex<double> jj = std::complex<double>(0.0, 1.0);
};
} // namespace BYOD
} // namespace SST

#endif