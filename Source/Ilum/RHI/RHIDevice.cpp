#include "RHIDevice.hpp"

#include "Backend/Vulkan/Device.hpp"
#include "Backend/DX12/Device.hpp"
#include "Backend/CUDA/Device.hpp"

namespace Ilum
{
RHIDevice::RHIDevice(RHIBackend backend):
    m_backend(backend)
{
}

std::unique_ptr<RHIDevice> RHIDevice::Create(RHIBackend backend)
{
	switch (backend)
	{
		case RHIBackend::Vulkan:
			return std::make_unique<Vulkan::Device>();
		case RHIBackend::DX12:
			return std::make_unique<DX12::Device>();
		case RHIBackend::CUDA:
			return std::make_unique<CUDA::Device>();
		default:
			break;
	}
	return nullptr;
}

RHIBackend RHIDevice::GetBackend() const
{
	return m_backend;
}
}        // namespace Ilum