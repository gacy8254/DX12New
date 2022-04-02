#pragma once
#include "BasePSO.h"


#include <memory>
#include <vector>

class IntegrateBRDFPSO : public BasePSO
{
public:
	IntegrateBRDFPSO(std::shared_ptr<Device> _device);
	virtual ~IntegrateBRDFPSO();

	void Apply(CommandList& _commandList);
};

