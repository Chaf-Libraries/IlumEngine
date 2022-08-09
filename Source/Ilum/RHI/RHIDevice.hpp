#pragma once

#include <Core/Window.hpp>

#include "RHIDefinitions.hpp"

namespace Ilum
{
class RHIDevice
{
  public:
	RHIDevice()          = default;
	virtual ~RHIDevice() = default;

	static std::unique_ptr<RHIDevice> Create();

	virtual void WaitIdle() = 0;

	virtual bool IsFeatureSupport(RHIFeature feature) = 0;
};
}        // namespace Ilum