#pragma once

#include <sst/core/event.h>

namespace SST {
namespace BYOD {

class DigitalEvent : public Event {
  public:
	void serialize_order(SST::Core::Serialization::serializer &ser) override {
		Event::serialize_order(ser);
		ser & id;
		ser & resolution;
		ser & data;
	}

	DigitalEvent(uint32_t id, uint32_t resolution, std::vector<uint64_t> data) //constructor
		: Event(),
		id(id),
		resolution(resolution), 
		data(data)
	{}

	uint32_t getId() { return id; }
	uint32_t getResolution() { return resolution; }
	std::vector<uint64_t> getData() { return data; }

  private:
	DigitalEvent() {} // for serialization only

	uint32_t id;
	uint32_t resolution;
	std::vector<uint64_t> data;

	ImplementSerializable(SST::BYOD::DigitalEvent);
};
} // namespace BYOD
} // namespace SST