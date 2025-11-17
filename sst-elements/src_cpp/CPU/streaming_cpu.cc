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

#include "streaming_cpu.h"

namespace SST {
namespace BYOD {

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
streamingCPU::streamingCPU(ComponentId_t id, Params &params) : Component(id) {

	frequency = 		params.find<UnitAlgebra>("frequency", "1GHz");
	addr_data = 		params.find<Addr>("vector_base_addr", 0);
	vector_count = 		params.find<int32_t>("vector_count", 0);
	size = 				params.find<int32_t>("size", 12);
	resolution = 		params.find<int32_t>("resolution", 8); 
	verbose = 			params.find<int32_t>("verbose", 0);

	clockTC = 			registerClock(frequency, new Clock::Handler<streamingCPU>(this, &streamingCPU::clockTick));
	inputLink = 		configureLink("input",	new Event::Handler<streamingCPU>(this, &streamingCPU::handleInput));
	dataOutputLink = 	configureLink("outputData");

	energyConsumption = registerStatistic<double>("energyCPU"); //currently unused, remove?
	memory = 			loadUserSubComponent<StandardMem>("memory", ComponentInfo::SHARE_NONE, clockTC, new StandardMem::Handler<streamingCPU>(this, &streamingCPU::handleMemEvent));
	nanoTimeConverter = getTimeConverter("1ns");
	picoTimeConverter = getTimeConverter("1ps");

	params.find_array("memory", memory_data);
	params.find_array("weight_address", weight_addresses);
	params.find_array("classes", classes);

	num_meshes = weight_addresses.size() / 4; //compute the number of meshes connected to the CPU

	for(int i = 0; i < num_meshes; i++) {
		weightOutputLink.push_back(configureLink("outputWeight" + std::to_string(i)));
	}

	std::string prefix = "@t\t@X\t[CPU::" + std::to_string(id) + "]:\t";
	outputStr.init(prefix, verbose, 0, SST::Output::STDOUT);
	
	if (!memory) {
		outputStr.fatal(
			CALL_INFO, -1,
			"Unable to load memHierarchy.standardInterface subcomponent; "
			"check that 'memory' slot is filled in input.\n");
	}

	num_bits = int(ceil(float(resolution) / float(8))) * 8; //number of bits needed to store a data entry with the given resolution
	data_buffer = std::vector<uint8_t>(size * num_bits, 0); //buffer for collecting data vector after reading from memory
	buffer_start_index = 0;

	maxAddr = 512 * 1024 * 1024 - 1;
	vector_counter = 0;

	registerAsPrimaryComponent();
	primaryComponentDoNotEndSim();
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void streamingCPU::setup() {

	memory->setup();
	line_size = memory->getLineSize();
}

void streamingCPU::finish() { }

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void streamingCPU::init(unsigned int phase) {
	
	outputStr.verbose(CALL_INFO, 1, 0, "init phase: %u \n", phase);
	
	memory->init(phase);
	if(phase == 0) { // write data vectors during init phase

		int num_chunks = (memory_data.size() - addr_data + 63)  / 64; //compute the number of 64 bit chunks required to send/receive all data vectors

		for(int i = 0; i < num_chunks; i++) { //send data vectors in 64 bit chunks

			if(i == num_chunks - 1)
				memory->sendUntimedData(new SST::Interfaces::StandardMem::Write(i*64, 64, std::vector<uint8_t>(memory_data.begin() + i * 64 + addr_data, memory_data.end())));
			else
				memory->sendUntimedData(new SST::Interfaces::StandardMem::Write(i*64, 64, std::vector<uint8_t>(memory_data.begin() + i * 64 + addr_data, memory_data.begin() + (i + 1) * 64 + addr_data)));
			pending_memory_accesses.push(i*64); //add to the list of data chunk that have to be read during the inference operation
		}
		std::vector<uint64_t> test(size, 0);
		dataOutputLink->sendUntimedData(new DigitalEvent(0, resolution, test)); //send an empty event to the dataOutput port to test signal path integretiy
	}

	if(phase > 0 && phase <= num_meshes) { //initialize the weights for each mesh

		int mesh_index = (phase - 1) ;
		std::vector<uint64_t> weights = memory_to_intVector(weight_addresses[mesh_index * 4], weight_addresses[mesh_index * 4 + 1], weight_addresses[mesh_index * 4 + 2], weight_addresses[mesh_index * 4 + 3]);
		weightOutputLink[mesh_index]->sendUntimedData(new DigitalEvent(0, resolution, weights)); //todo???
	}

    while (SST::Event* ev = inputLink->recvUntimedData()) {  // Check if the init event from phase 0 has reveived back at the CPU to verify signal path integrity

        DigitalEvent* event = dynamic_cast<DigitalEvent*>(ev);

        if (!event) { //check for correct data type of input vector
            outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Expected input port to be connected to an Element with digital output. Please check that components connected to %s have the correct output type\n", 
				getName().c_str(), getName().c_str());
        } 
		if (event->getResolution() != resolution) { //check for correct resolution/vmax of input vector
			outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Digital data received at input port has a resolution of %u bits, while the internal resolution is %u bits. Please make sure that the resolution of connected components is identical by setting the \"resolution\" parameter\n", 
				getName().c_str(), event->getResolution(), resolution);
		}
		if (event->getData().size() != size) { //check for correct size of input vector
			outputStr.fatal(
				CALL_INFO, -1, 
				"Error in %s: Data received at input port has a size of %u, while the internal size is %u. Please make sure that size of connected components is identical by setting the \"size\" parameter \n", 
				getName().c_str(), int(event->getData().size()), size);
		}
		if(event)
			outputStr.verbose(CALL_INFO, 1, 0, "Signal integrity tested, initialization finished \n");
	}
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
bool streamingCPU::clockTick(Cycle_t cycle) {

	outputStr.verbose(CALL_INFO, 1, 0, "cycle%lu: \n", cycle);

	if(!pending_memory_accesses.empty()) //check if all memory has been read and read another subchunk of 10 vectors from memory
		read_stream(10);
	
	if(!output_buffer.empty()) { //check if there is data in the output buffer and send it to the dataOutput port
		
		dataOutputLink->send(new DigitalEvent(vector_counter, resolution, output_buffer.front()));
		outputStr.verbose(CALL_INFO, 1, 0, "Data sent \n");
		output_buffer.pop();
		vector_counter += 1;
	}

	if(cycle>20000) { //check for timeout condition to end the simulation in case something breaks
		outputStr.verbose(CALL_INFO, 1, 0, "Warning: Simulation terminated due to timeout, not all data has been received \n");
		primaryComponentOKToEndSim();
	}
	return false;
}


void streamingCPU::handleInput(Event *ev) {

	DigitalEvent *input = static_cast<DigitalEvent *>(ev);
	xt::xarray<int32_t> result = xt::adapt(input->getData(), {size});

	outputStr.verbose(CALL_INFO, 1, 0, "Data received \n");
	std::cout << "Data in: " << result << " " << input->getId() << "  " << vector_count - 1<< std::endl;

	if(input->getId() == ( vector_count - 1)) {
		outputStr.verbose(CALL_INFO, 1, 0, "all memory operations complete, ending simulation \n");
		primaryComponentOKToEndSim();
	}

	delete input;

}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void streamingCPU::read_stream(int batch_size) {

	int counter = 0;
	if(memory_requests.empty()) { //only start a batch request if there are no pending memory reads

		while(!pending_memory_accesses.empty() && (counter < batch_size) ) { //queue memory reads until all memory has been read or if the number of requests exceeds the batch size
			memory->send(createRead(pending_memory_accesses.front(), 64));
			pending_memory_accesses.pop();
			counter++;
		}
	}
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void streamingCPU::buffer_data(std::vector<uint8_t> inData) { //cast and order incoming bits from memory into data vectors

	int32_t byte_index = 0;
	int32_t bit_index = 0;
	for (int i = 0; i < 64 * 8; i++) {

		data_buffer[buffer_start_index] = ((inData[byte_index] >> bit_index ) & 0x01);
		bit_index = (bit_index + 1) % 8;
		byte_index = (i + 1) / 8;
		buffer_start_index = (buffer_start_index + 1 ) % (size * num_bits);
		
		if(buffer_start_index == 0) {

			std::vector<uint64_t> out(size, 0);
			for (int k = 0; k < size * num_bits; k++)
				out[k / num_bits] |= data_buffer[k] << (k % num_bits);

			output_buffer.push(out);
		}
	}
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
std::vector<uint64_t> streamingCPU::memory_to_intVector(int start_address, int num_bytes, int num_elements, int resolution) {

	std::vector<uint64_t> output(num_elements, 0);

	int byte_index = start_address;
	int bit_index = 0;
	int element_byte_index = 0;
	int element_bit_index = 0;

	for (int i = 0; i < num_bytes * 8; i++) {
		output[element_byte_index] |= ((memory_data[byte_index] >> bit_index ) & 0x01) << element_bit_index;

		bit_index = (bit_index + 1) % 8;
		element_bit_index = (element_bit_index + 1) % num_bits;
		byte_index = start_address + (i + 1) / 8;
		element_byte_index = (i + 1) / (num_bits);
	}
	return output;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void streamingCPU::handleMemEvent(Req *req) {

	std::map<uint64_t, std::pair<SimTime_t, std::string>>::iterator i = memory_requests.find(req->getID()); //check whether the response is associated with a pending memory request
	if (memory_requests.end() == i)
		outputStr.fatal(CALL_INFO, -1, "Event (%lu) not found!\n", req->getID());
	else
		memory_requests.erase(i);

	SST::Interfaces::StandardMem::ReadResp* event = dynamic_cast<SST::Interfaces::StandardMem::ReadResp*>(req); //try to cast input event to a read response
	if(event)  //if the event is a read response, buffer the data from the memory
		buffer_data(event->data);
	
	outputStr.verbose(CALL_INFO, 1, 0, "Memory operation done.\n");
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
Req *streamingCPU::createWrite(Addr addr, std::vector<uint8_t> data) {
	//addr = ((addr % maxAddr) >> 2) << 2;

	//stat_energy_consumption_mem->addData(memory_write_energy);

	Req *req = new Interfaces::StandardMem::Write(addr, data.size(), data);
	if (req->needsResponse()) {
		memory_requests[req->getID()] =
			std::make_pair(getCurrentSimTime(picoTimeConverter), "write");
	}
	return req;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
Req *streamingCPU::createRead(Addr addr, size_t size) {
	//addr = ((addr % maxAddr) >> 2) << 2;

	uint32_t num_requests = std::ceil(size / memory_request_width);
	//stat_energy_consumption_mem->addData(memory_read_energy * num_requests);
	outputStr.verbose(CALL_INFO, 1, 0, "%u reads at 0x%lu.\n", num_requests, addr);

	Req *req = new Interfaces::StandardMem::Read(addr, size);
	if (req->needsResponse()) {
		memory_requests[req->getID()] =
			std::make_pair(getCurrentSimTime(picoTimeConverter), "read");
	}
	return req;
}

/**
* @brief BRIEF.
* @details DETAILS
* @param PARAMETER PARAMETER DESCRIPTION
* @return RETURN
*/
void streamingCPU::sendMemWrite(std::vector<uint8_t> data, Addr addr) {
	Req *req;
	std::vector<uint8_t> memInp;

	//*addr = rng.generateNextUInt32();
	uint32_t num_requests = std::ceil(data.size() / memory_request_width);
	outputStr.verbose(CALL_INFO, 1, 0, "%u writes for %lu bytes at 0x%lu\n",
				   num_requests, data.size(), addr);

	for (size_t i = 0; i < data.size(); i += memory_request_width) {
		memInp.insert(
			memInp.end(), data.begin() + i,
			std::min(data.begin() + i + memory_request_width, data.end()));
		req = createWrite(addr + i, memInp);
		memory->send(req);
		memInp.clear();
	}
}
} // namespace BYOD
} // namespace SST