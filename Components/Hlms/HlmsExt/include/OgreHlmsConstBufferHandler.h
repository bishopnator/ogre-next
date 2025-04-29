#pragma once
#include "OgreHlmsBufferHandler.h"

namespace Ogre
{
	/// Implementation of HlmsBufferHandler for const buffers.
	class _OgreHlmsExtExport HlmsConstBufferHandler : public HlmsBufferHandler
	{
	public:
		HlmsConstBufferHandler();
		~HlmsConstBufferHandler() override;

	public: // HlmsBufferHandler interface
		std::optional<ResourceAccess::ResourceAccess> analyzeBarriers(SceneManager& sceneManager) override;
		BufferPacked* createBuffer(VaoManager& vaoManager, size_t bufferSize) override;
		void destroyBuffer(VaoManager& vaoManager, BufferPacked* pBuffer) override;
		size_t getBufferAlignment(VaoManager& vaoManager) const override;
		size_t getMaxBufferSize(VaoManager& vaoManager) const override;
		void bindBuffer(CommandBuffer& commandBuffer, uint8_t stages, uint16_t slot, BufferPacked& buffer, size_t bindOffset) override;
	};
} // namespace Ogre
