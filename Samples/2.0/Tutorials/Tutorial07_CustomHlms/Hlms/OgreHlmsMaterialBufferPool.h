#pragma once
#include "OgreComponentExport.h"

// OGRE
#include <OgreCommon.h>
#include <OgreConstBufferPool.h>

namespace Ogre
{
	/// Implementation of ConstBufferPool to store properties from HlmsDatablock instances. In this case the data block class must inherit from ConstBufferPoolUser.
	/// A single buffer created by the HlmsMaterialBufferPool can hold multiple data blocks. If a buffer is full, new buffer is created to store further data blocks.
	/// 
	/// A data block get split its properties between primary and secondary buffers. The primary buffer is always ConstBufferPacked. The secondary buffer can be either
	/// ConstBufferPacked or ReadOnlyBufferPackedr. This will influence how the secondary buffer is bound to the shaders.
	class _OgreComponentExport HlmsMaterialBufferPool : public ConstBufferPool
	{
	public:
		HlmsMaterialBufferPool();
		~HlmsMaterialBufferPool() override;

		/// Check whether the pool is valid and can be used. It is valid if setPrimaryBufferParams was called.
		bool IsValid() const;

		/// Set the information about the primary buffer.
		/// @param stages				List of stages to which the primary buffer will be bound.
        /// @param slot					Slot index to which the buffer will be bound.
		/// @param datablockSize		Size of the data written by a data block. This will also define how many data blocks can be written in a single buffer.
		void setPrimaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize);

		/// Set the information about the secondary buffer.
		/// @param stages				List of stages to which the secondary buffer will be bound.
		/// @param slot					Slot index to which the buffer will be bound.
		/// @param datablockSize		Size of the data written by a data block. The number of available data blocks in the buffer are defined by the primary buffer as they must has same number of available slots.
		/// @param bufferType			Buffer type for the secondary buffer.
		/// @param useReadOnlyBuffers	Set the value to true to use ReadOnlyBufferPacked or false to use ConstBufferPacked for the secondary buffer.
		void setSecondaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize, BufferType bufferType = BT_DEFAULT, bool useReadOnlyBuffers = true);

		/// Called by Hlms implementation when a Hlms type changed.
        void onHlmsTypeChanged();

		/// Bind the buffers to the shaders if needed.
		void bindBuffers(const BufferPool& bufferPool, CommandBuffer& commandBuffer);

	public: // ConstBufferPool interface
		void _changeRenderSystem(RenderSystem* pRenderSystem) override;

	private:
		uint8_t mPrimaryBufferStages;
		uint16_t mPrimaryBufferSlot;

		uint8_t mSecondaryBufferStages;
        uint16_t mSecondaryBufferSlot;

		/// Last bound buffers
		const BufferPool* mLastBoundPool;
	};
} // namespace Ogre
