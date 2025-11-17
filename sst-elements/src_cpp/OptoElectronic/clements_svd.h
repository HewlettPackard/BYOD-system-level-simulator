#ifndef _clementsSVD_H
#define _clementsSVD_H

#include "../Events/complex_event.h"
#include "../Events/analog_event.h"
#include "Submodules/modulators.h"

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

/**
* @brief BRIEF.
* @details DETAILS
*/
class clementsSVD : public SST::Component {
  public:
	SST_ELI_REGISTER_COMPONENT(
		clementsSVD,
		"byod", // component library
		"ClementsSVD",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"An optical mesh based on Clements meshes for implementing an SVD decomposed matrix-vector multiplication in the BYOD library",
		COMPONENT_CATEGORY_UNCATEGORIZED
	);

	SST_ELI_DOCUMENT_PARAMS(
		{"size", 			"(uint32) size of the input/output vector", "1"}, 
		{"latency", 		"(uint32) DAC processing latency in ps", "1"}, 
		{"verbose", 		"(uint32) level of debuggin output", "0"},
		{"opticalLoss", 	"(double) total optical intensity loss of the mesh in percentage", "0"},
		{"maxVin", 			"(double) maximal input voltage for the phase shifters in V", "0"},
	);

	SST_ELI_DOCUMENT_PORTS(
		{"inputData", 		"receiving input signal (complex)", {"sst.byod.complexEvent"}},
		{"inputWeight", 	"receiving weight input signal (analog)", {"sst.byod.analogEvent"}},
		{"output", 			"sending output signal (complex)", {"sst.byod.complexEvent"}}
	);

	SST_ELI_DOCUMENT_STATISTICS(
		{"energyMesh", 		"Cumulative energy consumption of the optical mesh and the optical modulators", "pJ", 1}
	);

	SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
		{"modulator",		"Subcomponent slot for the optic modulator", "SST::byod::BasicModulator"}
	);

	clementsSVD(ComponentId_t id, Params &params);
	~clementsSVD() {};

	void setup();
	void finish();
	virtual void init(unsigned int phase) override;

	void handleDataInput(Event *ev);
	void handleWeightInput(Event *ev);
	void handleSelf(Event *ev);
	void reconstructUnitaryMatrix();
	void reconstructFullMatrix();
	void updateEnergy();

	//functions for reconstructing a matrix from a set of phases
	void U2MZI(int32_t row, double theta, double phi);
	void diagonals(xt::xarray<std::complex<double>> phases);
	xt::xarray<std::complex<double>> dc_coupler(int32_t start);
	void phase_shifter(int32_t start, int32_t start_index, double multiplier);
	void diagonals(int32_t start_index);
	void reconstructDiagonalMatrix();

  private:
	/** IO *****************************************************/

	Output outputStr;

	/** Link/Port **********************************************/

	Link *inputDataLink;
	Link *inputWeightLink;
	Link *outputLink;
	Link *selfLink;

	/** Parameters ********************************************/

	uint32_t size;
	uint32_t latency;
	uint32_t verbose;
	double opticalLoss;
	double maxVin;

	/** Statistics *********************************************/

	Statistic<double_t> *energyConsumption;

	/** operation *********************************************/

	SST::BYOD::basicModulator* modulator;
	SimTime_t lastSwitch;
	TimeConverter *nanoTimeConverter;
	TimeConverter *picoTimeConverter;

	std::vector<double> test_data;
	xt::xarray<std::complex<double>> transfer_matrix;
	xt::xarray<std::complex<double>> full_matrix;
	xt::xarray<std::complex<double>> mesh_layer;
	xt::xarray<double> phases;
	xt::xarray<double> phasesU;
	xt::xarray<double> phasesS;
	xt::xarray<double> phasesV;
	xt::xarray<std::complex<double>> odd_dc_couplers;
	xt::xarray<std::complex<double>> even_dc_couplers;

	const std::complex<double> jj = std::complex<double>(0.0, 1.0);
	const xt::xarray<std::complex<double>> beam_splitter_matrix
      {{1 / sqrt(2.0), 1 / sqrt(2.0) * jj},
       {1 / sqrt(2.0) * jj, 1 / sqrt(2.0)}};
};
} // namespace BYOD
} // namespace SST

#endif