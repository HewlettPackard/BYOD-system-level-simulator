#include "boilerplate_new.h"



namespace SST {
namespace BYOD {
XXX::XXX(ComponentId_t id, Params &params) : Component(id) {
	std::string prefix = "@t\t@X\t[DAC::" + std::to_string(id) + "]:\t";
	output.init(prefix, 1, 0, SST::Output::STDOUT);

	frequency = params.find<UnitAlgebra>("frequency", "1GHz");
	controller_energy = params.find<float_t>("controller_energy", 1);
	
	//area = params.find<float_t>("area", 1);
	//clocked = params.find<bool>("synchronous", false);


	num_conversions = registerStatistic<uint64_t>("conversions");
	stat_energy_consumption = registerStatistic<float_t>("energyDAC");
	//stat_area_usage = registerStatistic<float_t>("areaDAC");
	//stat_latency = registerStatistic<SimTime_t>("latency");

	latency = frequency.invert().getDoubleValue() * 1e12;


	//stat_area_usage->addData(area * 8);

	busy = false;

	nanoTimeConverter = getTimeConverter("1ns");
	picoTimeConverter = getTimeConverter("1ps");

	inputEvent = NULL;
	registerClock("1GHz", new Clock::Handler<XXX>(this, &XXX::clockTick));
	inputlink = configureLink("inputlink",	new Event::Handler<XXX>(this, &XXX::handle_input_sync));
	outputlink = configureLink("outputlink");

	selflink = configureSelfLink("selflink", new Event::Handler<XXX>(this, &XXX::handle_self));
}


void XXX::setup() {
	// output.verbose(CALL_INFO, 1, 0, "DAC (Async) is being set up.\n");
	// build initial matrix for conversion nodes
}


void XXX::init(unsigned int phase){
    // Only send our info on phase 0
    if (phase == 0) {
		std::vector<uint64_t> v(size, 0);
		//std::vector<float> v(size, 0.0);
        DigitalEvent* event = new DigitalEvent(0, resolution, v);
		//AnalogEvent* event = new AnalogEvent(0, max_vout, v);
        outputlink->sendUntimedData(event);
    }

    // Check if an event is received. recvUntimedData returns nullptr if no event is available
    while (SST::Event* ev = inputlink->recvUntimedData()) {

        DigitalEvent* event = dynamic_cast<DigitalEvent*>(ev);
		//AnalogEvent* event = dynamic_cast<AnalogEvent*>(ev);

        if (!event) { //check for correct data type of input vector
            output.fatal(CALL_INFO, -1, "Error in %s: Expected input port to be connected to an Element with digital output. Please check that components connected to %s have the correct output type\n", getName().c_str(), getName().c_str());
        } 
		if (event->getResolution() != resolution) { //check for correct resolution/vmax of input vector
			output.fatal(CALL_INFO, -1, "Error in %s: Digital data received at input port has a resolution of %u bits, while the internal resolution is %u bits. Please make sure that the resolution of connected components is identical by setting the \"resolution\" parameter\n", getName().c_str(), event->getResolution(), resolution);
			//output.fatal(CALL_INFO, -1, "Error in %s: Analog data received at input port has a vmax of %f V, while the internal vmax is %f V. Please make sure that vmax of connected components is identical by setting the \"max_vin/vout\" parameter\n", getName().c_str(), event->getResolution(), resolution);
		}
		if (event->getData().size() != size) { //check for correct size of input vector
			output.fatal(CALL_INFO, -1, "Error in %s: Data received at input port has a size of %u, while the internal size is %u. Please make sure that size of connected components is identical by setting the \"size\" parameter \n", getName().c_str(), int(event->getData().size()), size);
		}
	}

}


void XXX::finish() {
	// output.verbose(CALL_INFO, 1, 0, "DAC (Async) is being finished.\n");
}


bool XXX::clockTick(Cycle_t cycle) {
	if (!busy && (inputEvent != NULL)) {
		busy = true;
		selflink->send(latency, picoTimeConverter, inputEvent);
	}
	return false;
}


void XXX::handle_input_sync(Event *ev) {
	if (!busy) {
		inputEvent = ev;
	}
}


void XXX::handle_self(Event *ev) {
	DigitalEvent *input = static_cast<DigitalEvent *>(ev);

	//stat_latency->addData(latency);

	delete input;
}



} // namespace BYOD
} // namespace SST