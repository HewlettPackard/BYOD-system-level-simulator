#pragma once

#include <sst/core/event.h>

namespace SST {
namespace BYOD {

class AnalogEvent : public Event {
  public:
	void serialize_order(SST::Core::Serialization::serializer &ser) override {
		Event::serialize_order(ser);
		ser & id;
		ser & max;
		ser & data;
	}

	AnalogEvent(uint32_t id, double max, std::vector<double> data) //constructor
		: Event(),
		id(id),
		max(max), 
		data(data)
	{}

	uint32_t getId() { return id; }
	double getMax() { return max; }
	std::vector<double> getData() { return data; }

  private:
	AnalogEvent() {} // for serialization only

	uint32_t id;
	double max;
	std::vector<double> data;

	ImplementSerializable(SST::BYOD::AnalogEvent);
};
} // namespace BYOD
} // namespace SST