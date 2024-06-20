#pragma once
#include "OgreHlmsBufferPool.h"

namespace Ogre
{
	/// Implementation of HlmsBufferPool for ReadOnlyBufferPacked.
	class _OgreComponentExport HlmsReadOnlyBufferPool : public HlmsBufferPool
	{
	public:
		explicit HlmsReadOnlyBufferPool(size_t defaultBufferSize);
		~HlmsReadOnlyBufferPool() override;

	public: // HlmsBufferPool interface
		BufferPacked* createBuffer(size_t bufferSize) override;
		void destroyBuffer(BufferPacked* pBuffer) override;
		void bindBuffer(CommandBuffer& commandBuffer, BufferPacked& buffer, size_t bindOffset) override;
		size_t getBufferAlignment() const override;
		size_t getMaxBufferSize() const override;
	};
} // namespace Ogre
