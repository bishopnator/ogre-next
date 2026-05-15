#pragma once
#include "OgreHlmsBufferHandler.h"
#include "OgreHlmsUavBufferHandlerFwd.h"

// forwards
namespace Ogre
{
	class HlmsExt;
}

namespace Ogre
{
	/// Implementation of HlmsBufferHandler for uav buffers.
	/// * in write mode: RWStructuredBuffer<ElementT> buffer : register(write_register) --> write_register is u0, u1, u2, ....
	/// * in read mode: StructuredBuffer<ElementT> : register(read_register) --> read_register is t0, t1, t2, ...
	/// The r/w mode is determined by implementation of analyzeBarriers and the resourceAccessMap passed in the constructor.
	class _OgreHlmsExtExport HlmsUavBufferHandler : public HlmsBufferHandler
	{
	public:
		explicit HlmsUavBufferHandler(HlmsExt& hlms, uint16_t writeSlot, uint16_t readSlot, size_t elementSize, const ResourceAccessMap& resourceAccessMap);
		~HlmsUavBufferHandler() override;

		/// Get element size.
		size_t getElementSize() const;

	public: // HlmsBufferHandler interface
		std::optional<ResourceAccess::ResourceAccess> analyzeBarriers(SceneManager& sceneManager) override;
		BufferPacked* createBuffer(VaoManager& vaoManager, size_t bufferSize) override;
		void destroyBuffer(VaoManager& vaoManager, BufferPacked* pBuffer) override;
		size_t getBufferAlignment(VaoManager& vaoManager) const override;
		size_t getMaxBufferSize(VaoManager& vaoManager) const override;
		void bindBuffer(CommandBuffer& commandBuffer, uint8_t stages, uint16_t slot, BufferPacked& buffer, size_t bindOffset) override;

	private:
		HlmsExt& mHlms;
		const uint32_t mElementSize; ///< Size of the element stored in the buffer in bytes.
		const ResourceAccessMap mResourceAccessMap;

		ResourceAccess::ResourceAccess mCurrentResourceAccess;

		/// Shader slot bindings.
		const uint16_t mWriteSlot; ///< u0, u0, u2, ...
		const uint16_t mReadSlot; ///< t0, t0, t2, ...
	};
} // namespace Ogre
