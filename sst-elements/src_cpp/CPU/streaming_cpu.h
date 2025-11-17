#ifndef _streamingCPU_H
#define _streamingCPU_H

#include "../Events/digital_event.h"
#include "../Events/analog_event.h"

#include <cstdint>
#include <vector>
#include <cmath>
#include <util.h>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/rng/marsaglia.h> // for random number generation for memory addresses
#include <sst/core/component.h>
#include <sst/core/link.h>

#include <xtensor-blas/xlinalg.hpp>
#include <xtensor/containers/xadapt.hpp>
#include <xtensor/io/xio.hpp>
#include <xtensor/core/xvectorize.hpp>
#include <xtensor/misc/xsort.hpp>
#include <xtensor/containers/xarray.hpp>

namespace SST {
namespace BYOD {

using StandardMem = SST::Interfaces::StandardMem;
using Req = SST::Interfaces::StandardMem::Request;
using Addr = SST::Interfaces::StandardMem::Addr;

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
class streamingCPU : public SST::Component {
  public:
	SST_ELI_REGISTER_COMPONENT(
		streamingCPU,
		"byod",
		"StreamingCPU",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"A basic CPU for streaming data from memory in the BYOD library",
		COMPONENT_CATEGORY_UNCATEGORIZED
	);

	SST_ELI_DOCUMENT_PARAMS(
		{"size", 			"(uint32) size of the input/output vector", "1"}, 
		{"resolution", 		"(uint32) DAC processing latency in ps", "1"}, 
		{"verbose", 		"(uint32) level of debuggin output", "0"},
		{"vectorCount", 	"(uint32) total optical intensity loss of the mesh in percentage", "0"},
		{"vectorBaseAddr", 	"(uint32) total optical intensity loss of the mesh in percentage", "0"},
		{"frequency", 		"(double) maximal input voltage for the phase shifters in V", "0"},
	);

	SST_ELI_DOCUMENT_PORTS(
		{"input", 				"receiving input signal (digital)", {"sst.byod.digitalEvent"}},
		{"outputData", 			"sending data output signal (digital)", {"sst.byod.digitalEvent"}},
		{"outputWeight%d", 		"sending weight output signal (digital)", {"sst.byod.digitalEvent"}}
	);

	SST_ELI_DOCUMENT_STATISTICS(
		{"energyCPU", 			"Cumulative energy consumption of CPU", "pJ", 1}
	);

	SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
		{"memory",				"Interface to memory hierarchy", "SST::Interfaces::StandardMem"}
	);

	streamingCPU(ComponentId_t id, Params &params);
	~streamingCPU() {};

	void setup();
	void finish();
	bool clockTick(Cycle_t cycle);
	void init(unsigned int phase) override;
	void handleInput(Event *ev);
	void handleMemEvent(Req *ev);

  private:
	/** IO *****************************************************/

	Output outputStr;

	/** Link/Port **********************************************/

	Link *inputLink;
	Link *dataOutputLink;
	std::vector<Link*> weightOutputLink;
	std::vector<Link*> biasOutputLink;

	/** Parameters ********************************************/

	uint32_t size;
	uint32_t latency;
	uint32_t verbose;
	UnitAlgebra frequency;
	StandardMem *memory;
	SST::RNG::MarsagliaRNG rng;
	uint32_t resolution;
	uint32_t num_bits;
	Addr addr_data;

	/** Statistics *********************************************/

	Statistic<double_t> *energyConsumption;

	/** operation *********************************************/

	uint64_t maxAddr;
	uint64_t line_size;
	TimeConverter *nanoTimeConverter;
	TimeConverter *picoTimeConverter;
	TimeConverter *clockTC;
	const double_t memory_request_width = 64;

	std::queue<Addr> pending_memory_accesses;
	std::queue<std::vector<uint64_t>> output_buffer;

	std::map<Interfaces::StandardMem::Request::id_t, std::pair<SimTime_t, std::string>>	memory_requests;

	std::vector<uint8_t> memory_data;
	std::vector<int32_t> weight_addresses;
	std::vector<int32_t> bias_addresses;
	std::vector<int32_t> sigma_addresses;

	int32_t num_meshes;
	int32_t num_ALU;

	std::vector<uint64_t> memory_to_intVector(int start_address, int num_bytes, int num_elements, int resolution);

	Req *createRead(Addr addr, size_t size);

	/**
	 * @brief create a write request
	 * @param addr memory address to write to
	 * @returns memory request for a write
	 */
	Req *createWrite(Addr addr, std::vector<uint8_t> data);

	/**
	 * @brief send data to memory backend
	 * @details create correct number of memory requests
	 * and send them to the memory backend
	 * @param data data to be written
	 * @param addr pointer to address store (depending on which data is written)
	 */
	void sendMemWrite(std::vector<uint8_t> data, Addr addr);

	void read_stream(int batch_size);

	std::vector<uint8_t> data_buffer;
	std::vector<int32_t> classes;
	int32_t vector_count;
	int32_t buffer_start_index;
	int32_t vector_counter;
	void buffer_data(std::vector<uint8_t> inData);
};
} // namespace BYOD
} // namespace SST

#endif