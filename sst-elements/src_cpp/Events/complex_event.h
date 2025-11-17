#pragma once

#include <sst/core/event.h>

namespace SST {
namespace BYOD {

class ComplexEvent : public Event {
  public:
	void serialize_order(SST::Core::Serialization::serializer &ser) override {
		Event::serialize_order(ser);
		ser & id;
		ser & max;
		ser & dataReal;
		ser & dataImag;
	}

	ComplexEvent(uint32_t id, double max, std::vector<double> dataReal, std::vector<double> dataImag) //constructor
		: Event(),
		id(id),
		max(max), 
		dataReal(dataReal),
		dataImag(dataImag)

	{
		if(dataReal.size() != dataImag.size())
			std::cout << "WARNING: real and imaginary vector have different sizes" << std::endl;
	}

	uint32_t getId() { return id; }
	double getMax() { return max; }
	std::vector<double> getReal() { return dataReal; }
	std::vector<double> getImag() { return dataImag; }

  private:
	ComplexEvent() {} // for serialization only

	uint32_t id;
	double max;
	std::vector<double> dataReal;
	std::vector<double> dataImag;

	ImplementSerializable(SST::BYOD::ComplexEvent);
};
} // namespace BYOD
} // namespace SST