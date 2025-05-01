#pragma once
#include "OgreHlmsExtPrerequisites.h"

// OGRE
#include <OgreResourceTransition.h>
#include <OgreStd/vector.h>

// std
#include <optional>
#include <stdint.h>

// forwards
namespace Ogre
{
	class BufferPacked;
	class CommandBuffer;
	class SceneManager;
	class VaoManager;
}

namespace Ogre
{
	/// Helper class to create/destroy/bind a buffer. The inherited classes handle certain type of the buffer like const, read-only, tex, uav, etc.
	class _OgreHlmsExtExport HlmsBufferHandler
	{
	public:
		HlmsBufferHandler();
		virtual ~HlmsBufferHandler();

		/// Analyze the memory barriers for the buffers created by this handler and returns a desired access to it.
		virtual std::optional<ResourceAccess::ResourceAccess> analyzeBarriers(SceneManager& sceneManager) = 0;

		/// Create new buffer.
		virtual BufferPacked* createBuffer(VaoManager& vaoManager, size_t bufferSize) = 0;

		/// Destroy buffer.
		virtual void destroyBuffer(VaoManager& vaoManager, BufferPacked* pBuffer) = 0;

		/// Get the buffer alignment to which the size must be rounded.
		virtual size_t getBufferAlignment(VaoManager& vaoManager) const = 0;

		/// Get maximum buffer size.
		virtual size_t getMaxBufferSize(VaoManager& vaoManager) const = 0;

		/// Bind a buffer to the shaders.
		virtual void bindBuffer(CommandBuffer& commandBuffer, uint8_t stages, uint16_t slot, BufferPacked& buffer, size_t bindOffset) = 0;
	};
} // namespace Ogre
