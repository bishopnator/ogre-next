#pragma once
#include "OgreHlmsBufferPool.h"

namespace Ogre
{
	/// Implementation of HlmsBufferPool for ConstBufferPacked.
	class _OgreComponentExport HlmsConstBufferPool : public HlmsBufferPool
	{
	public:
		explicit HlmsConstBufferPool(size_t defaultBufferSize);
		~HlmsConstBufferPool() override;

	public: // HlmsBufferPool interface
		BufferPacked* createBuffer(size_t bufferSize) override;
		void destroyBuffer(BufferPacked* pBuffer) override;
		void bindBuffer(CommandBuffer& commandBuffer, BufferPacked& buffer, size_t bindOffset) override;
		size_t getBufferAlignment() const override;
		size_t getMaxBufferSize() const override;
	};
} // namespace Ogre
