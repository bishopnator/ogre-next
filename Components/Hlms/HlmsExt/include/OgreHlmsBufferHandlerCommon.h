#pragma once

// OGRE
#include <CommandBuffer/OgreCbShaderBuffer.h>
#include <CommandBuffer/OgreCommandBuffer.h>

/// Common declarations used by OgreHlmsXXXBufferHandler classes.
namespace Ogre
{
	namespace Details
	{
		/// Bind a buffer to the given slot to all selected shader types.
		template<typename BufferT>
		void bindBuffer(CommandBuffer& commandBuffer, uint8_t stages, uint16_t slot, BufferT& buffer, uint32_t offset)
		{
			static_assert(NumShaderTypes == 5, "ShaderType enum has been changed! Update the implementation.");

			if ((stages & (1 << VertexShader)) != 0)
				*commandBuffer.addCommand<CbShaderBuffer>() = CbShaderBuffer(VertexShader, slot, &buffer, offset, 0);
			if ((stages & (1 << PixelShader)) != 0)
				*commandBuffer.addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader, slot, &buffer, offset, 0);
			if ((stages & (1 << GeometryShader)) != 0)
				*commandBuffer.addCommand<CbShaderBuffer>() = CbShaderBuffer(GeometryShader, slot, &buffer, offset, 0);
			if ((stages & (1 << HullShader)) != 0)
				*commandBuffer.addCommand<CbShaderBuffer>() = CbShaderBuffer(HullShader, slot, &buffer, offset, 0);
			if ((stages & (1 << DomainShader)) != 0)
				*commandBuffer.addCommand<CbShaderBuffer>() = CbShaderBuffer(DomainShader, slot, &buffer, offset, 0);
		}

	} // namespace Details
} // namespace Ogre
