#pragma once
#include "OgreComponentExport.h"

// OGRE
#include <OgreStd/vector.h>

namespace Ogre
{
	/// Buffer pool for providing access to the buffers needed for binding to the shaders. A buffer pool can fill data in 2 mapping modes: direct and indirect.
	/// The direct map mode means that MapBuffer method always takes a new buffer and data are filled from the beginning.
	/// The indirect map mode fills that until the buffer is full and only then takes new buffer.
	/// 
	/// The advantage of indirect map mode is that the same buffer is reused by multiple shaders, only new binding offsets are provided. It is suitable for buffers
	/// with higher capacity like ReadOnlyBufferPacked. The direct map mode is suitable for smaller buffers like ConstBufferPacked.
	class _OgreComponentExport HlmsBufferPool : public OgreAllocatedObj
	{
	public:
		/// Mapping mode. The buffer can work in 2 different modes:
		///  direct mapping mode:   The buffer is bound to the shaders completely - from 0 to its whole capacity.
		///  indirect mapping mode: Only the part of the buffer can be bound to the shaders.
		enum class MapMode : uint8_t
		{
			eDirect = 0,	///< Allows to map always from the beginning of the buffer and shaders have access to whole buffer.
			eIndirect		///< Allows to map in the middle of the buffer and shaders don't necessarily have access to the whole buffer.
		};

	public:
		/// @param mapMode				Mapping mode in which this buffer will work.
		/// @param defaultBufferSize	Default buffer size used for newly created buffers within this pool.
		explicit HlmsBufferPool(MapMode mapMode, size_t defaultBufferSize);
		virtual ~HlmsBufferPool();

		/// Set binding for the buffers in the shaders.
		/// @param stages	Combination of ShaderType values.
		/// @param slot		Slot index to which the buffers will be bound.
		void setBinding(uint8_t stages, uint16_t slot);

		/// Bind the buffers to the shaders.
		void bindBuffer(CommandBuffer* pCommandBuffer);

		/// Set new render system to be used with the manager.
		void setRenderSystem(Ogre::RenderSystem* pRenderSystem);

		/// Destroys all the created buffers.
		void destroyAllBuffers();

		/// Maps a buffer and prepares it for filling the data.
		/// if m_MapMode == MapMode::eDirect:   Maps always next buffer and data will be written at the begining of the buffer. minimumSizeBytes is ignored
		/// if m_MapMode == MapMode::eIndirect: Tries to map region of minimumSizeBytes (or till the end) within the current buffer. If there is not enough space (minimumSizeBytes > 0), maps next buffer.
		/// 
		/// note: pCommandBuffer can be nullptr only if map mode is set to MapMode::eDirect - in this case the buffer is not bound immediately, but caller
		///       is responsible to call BindBiffer manually later.
		void mapBuffer(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes = 0);

		/// Unmaps the buffer.
		void unmapBuffer(CommandBuffer* pCommandBuffer);

		/// Write data to the buffer - must be mapped by previous call MapNextBuffer.
		void writeData(const void* pData, size_t numBytes);

		/// Check whether there is enough space in the currently mapped range to write give size of data.
		bool canWrite(size_t numBytes) const;

		/// Get the currently written data since last MapBuffer call.
		size_t getWrittenSize() const;

		/// This gets called right before executing the command buffer.
		void preCommandBufferExecution(CommandBuffer* pCommandBuffer);

		/// This gets called after executing the command buffer.
		void postCommandBufferExecution(CommandBuffer* pCommandBuffer);

		/// Called when the frame has fully ended (ALL passes have been executed to all RTTs).
		void frameEnded();

	protected:
		virtual BufferPacked* createBuffer(size_t bufferSize) = 0;
		virtual void destroyBuffer(BufferPacked* pBuffer) = 0;
		virtual void bindBuffer(CommandBuffer& commandBuffer, BufferPacked& buffer, size_t bindOffset) = 0;
		virtual size_t getBufferAlignment() const = 0;
		virtual size_t getMaxBufferSize() const = 0;

	private:
		void updateShaderBufferCommand(CommandBuffer* pCommandBuffer);

		/// Implementation of MapMode::eDirect
		void mapBufferDirect(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes);
		void unmapBufferDirect();

		/// Implementation of MapMode::eIndirect
		void mapBufferIndirect(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes);
		void mapNextBufferIndirect(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes);
		void unmapBufferIndirect(CommandBuffer* pCommandBuffer);

	protected:
		/// VertexArrayObject manager which is used for creating/destroying the buffers.
		VaoManager* mVaoManager = nullptr;

		/// Binding of the buffer.
		uint8_t mStages = 0;
		uint16_t mSlot = 0;

	private:
		/// Mapping mode for the buffer. Set through the constructor and cannot be changed later.
		const MapMode mMapMode;

		/// Default buffer size used for newly created buffers.
		const size_t mDefaultBufferSize;
		size_t mBufferSize;

		uint8_t* mStartPtr = nullptr; ///< Start of the writing to the buffer.
		uint8_t* mCurrentPtr = nullptr; ///< Current offset where next data can be written (m_pStartOffset <= m_pCurrentOffset). Number of written bytes = m_pCurrent - m_pStart
		size_t mCapacity = 0; ///< Number of bytes allowed to write to the buffer at m_pStart.

		uint8_t* mMappedPtr = nullptr; ///< Pointer returned by BufferPacked::map call.
		size_t mMappedOffset = 0; ///< Offset from the beginning of the buffer where the buffer was mapped.

		size_t mCurrentIndex = 0; ///< Resets to zero every new frame. Index to m_Buffers which buffer is currently used for writing.
		vector<BufferPacked*>::type mBuffers; ///< Buffer pool.

		/// Offset in CommandBuffer where CbShaderBuffer was created. If written data size is not known at the beginning of mapping of
		/// the buffer, the command offset of the CbShaderBuffer in CommandBuffer is saved, and later when new mapping is requested,
		/// the written data size is updated in that CbShaderBuffer.
		size_t mShaderBufferCommandOffset = (size_t)~0;
	};
} // namespace Ogre
