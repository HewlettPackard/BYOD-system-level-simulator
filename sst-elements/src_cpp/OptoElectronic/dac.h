#ifndef _DAC_H
#define _DAC_H

#include "../Events/digital_event.h"
#include "../Events/analog_event.h"

#include <cstdint>
#include <math.h>
#include <util.h>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/unitAlgebra.h>

#include <xtensor-blas/xlinalg.hpp>
#include <xtensor/containers/xadapt.hpp>
#include <xtensor/io/xio.hpp>
#include <xtensor/core/xvectorize.hpp>
#include <xtensor/containers/xarray.hpp>


namespace SST {
namespace BYOD {

enum DACType { C2C, R2R, CUSTOM };

/**
* @brief BRIEF.
* @details DETAILS
*/
DACType parseDACStr(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(),
				   [](unsigned char c) { return std::tolower(c); });
	if (str == "c2c") {
		return DACType::C2C;
	} else if (str == "r2r") {
		return DACType::R2R;
	} else if (str == "custom") {
		return DACType::CUSTOM;
	} else {
		throw std::invalid_argument("DAC type %s not supported.\n");
	}
}

/**
* @brief BRIEF.
* @details DETAILS
*/
class DAC : public SST::Component {
  public:
	SST_ELI_REGISTER_COMPONENT(
		DAC,
		"byod",
		"DAC",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"Digital-to-Analog-Converter (DAC) in BYOD library",
		COMPONENT_CATEGORY_UNCATEGORIZED
	);

	SST_ELI_DOCUMENT_PARAMS(
		{"dacType", 		"(string) DAC architecture C2C/R2R/CUSTOM", "R2R"},
		{"size", 			"(uint32) size of the input/output vector", "1"}, 
		{"latency", 		"(uint32) DAC processing latency in ps", "1"}, 
		{"resolution", 		"(uint32) bit resolution", "8"},
		{"verbose", 		"(uint32) level of debuggin output", "0"},
		{"minVout", 		"(double) min. voltage level", "0"},
		{"maxVout", 		"(double) max. voltage level", "1"},
		{"element", 		"(double) unit resistance or capacitance", "5e3 (R2R)/1e-12 (C2C)"},
		{"frequency",		"(string) conversion frequency (with unit)", "1"},
		{"controllerEnergy","(double) static controller energy usage per convert in pJ", "1"},
		{"energyPerState",	"(vector<double>) array containing the energy consumption per DAC state in pJ. Only used for dacType=custom", "1"},
	);

	SST_ELI_DOCUMENT_PORTS(
		{"input", 			"receiving input signal (digital)", {"sst.byod.digitalEvent"}},
		{"output", 			"sending output signal (analog)", {"sst.byod.analogEvent"}}
	);

	SST_ELI_DOCUMENT_STATISTICS(
		{"energyDAC", 		"Cumulative energy consumption of DAC", "pJ", 1}
	);

	DAC(ComponentId_t id, Params &params);
	~DAC() {};

	void setup();
	void finish();
	virtual void init(unsigned int phase) override;

	bool clockTick(Cycle_t cycle);
	void handleInput(Event *ev);
	void handleSelf(Event *ev);
	xt::xarray<double> convert(xt::xarray<double> input);
	void updateEnergy();


  private:
	/** IO *****************************************************/

	Output outputStr;

	/** Link/Port **********************************************/

	Link *inputlink;
	Link *outputlink;
	Link *selflink;

	/** Parameters ********************************************/

	DACType dacType;
	uint32_t size;
	uint32_t resolution;
	UnitAlgebra frequency;
	uint32_t latency;
	double element;
	double minVout;
	double maxVout;
	double controllerEnergy;
	std::vector<double> energyPerValue;

	/** Statistics *********************************************/

	Statistic<double> *energyConsumption;

	/** operation *********************************************/

	SimTime_t lastSwitch;
	Event *inputEvent;
	uint32_t verbose;
	double currentEnergy;
	double glockPeriod;
	TimeConverter *nanoTimeConverter;
	TimeConverter *picoTimeConverter;
	// inverse of conversion node matrix
	xt::xarray<double_t> conversionNodeMatrix;


	/**
	 * @brief current at each vector element required for conversion [V]
	 * @details
	 * for one elements: v_max * count_ones_binary - v_out
	 *
	 */
	double_t currentForConversion(xt::xarray<int32_t> value);

	/**
	 * @brief energy for converting given input value
	 * @param inp integer input value
	 * @returns energy for conversion
	 */
	double_t convertEnergy(int32_t inp);

	/**
	 * @brief get voltages for each of the bit_res nodes
	 * @details DAC consists of bit_res nodes.
	 * An input integer is converted to its binary representation.
	 * Each bit_res node has an output voltage
	 * The input->output equation can be represented by the Mx = y equation
	 * [x_0]   [ 4  -2   0   0   0  ...]   [v_0]
	 * [x_1]   [-2   5  -2   0   0  ...]   [v_1]
	 * [x_2] = [ 0  -2   5  -2   0  ...] x [v_2]
	 * [...]   [ ......................]   [...]
	 * [x_n]   [ 0   0   0  ... -2   3 ]   [v_n]
	 * which is solved by this method
	 * @param inp input integer value
	 * @returns output voltages for each bit
	 */
	xt::xarray<double_t> voltagesAtEachNode(xt::xarray<int8_t> inp);

	void precomputeEnergy();

	/**
	 * @brief convert value to array of bits
	 * @param x input integer value
	 * @returns array of bits with LSB=out(0) LSB and MSB=out(bit_res)
	 */
	xt::xarray<int8_t> value2bits(int32_t x);
};
} // namespace BYOD
} // namespace SST

#endif