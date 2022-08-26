#pragma once

#include "RHIDefinitions.hpp"

#include <memory>

namespace Ilum
{
class RHIDevice;

struct BufferDesc
{
	std::string name;

	RHIBufferUsage usage;
	RHIMemoryUsage memory;
	size_t         size;

	template<typename Archieve>
	inline void serialize(Archieve& ar)
	{

	}
	//INLINE_SERIALIZATION(name, usage, memory, size);
};

class RHIBuffer
{
  public:
	RHIBuffer(RHIDevice *device, const BufferDesc &desc);
	virtual ~RHIBuffer() = default;

	static std::unique_ptr<RHIBuffer> Create(RHIDevice *device, const BufferDesc &desc);

	const BufferDesc &GetDesc() const;

	virtual void *Map() = 0;
	virtual void  Unmap() = 0;
	virtual void  Flush(size_t offset, size_t size) = 0;

  protected:
	RHIDevice *p_device = nullptr;
	BufferDesc m_desc;
};

struct BufferStateTransition
{
	RHIBuffer *buffer;
	RHIResourceState src;
	RHIResourceState dst;
};
}        // namespace Ilum