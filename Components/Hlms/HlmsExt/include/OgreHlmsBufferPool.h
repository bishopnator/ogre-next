#pragma once
#include "OgreHlmsBufferPoolInterface.h"

namespace Ogre
{
	/// Buffer pool for providing access to the buffers needed for binding to the shaders. A buffer pool can fill data in 2 mapping modes: bulk and sub-range.
	///
	/// The bulk map mode means that mapBuffer method always takes a new buffer and data are filled from the beginning. It is suitable for the buffers with smaller
	///    capacities like ConstBufferPacked.
	/// 
	/// The sub-range map mode means that mapBuffer just inserts a command to CommandBuffer with the offset where last write ended. The command is updated when
	///    new mapBuffer is called which means that the size of written data is known. This mode allows to reuse same buffer which can be optimized by the GPU
	///    drivers as updating just the offset/size of already bound buffer is faster than binding new buffer completely. It is suitable for buffers with larger
	///    capacities like ReadOnlyBufferPacked.
	class _OgreHlmsExtExport HlmsBufferPool final : public HlmsBufferPoolInterface
	{
	public:
		/// Mapping mode. The buffer can work in 2 different modes:
		///  bulk mapping mode:      The buffer is bound to the shaders completely - from 0 to its whole capacity.
		///  sub-range mapping mode: Only the part of the buffer can be bound to the shaders.
		enum class MapMode : uint8_t
		{
			eBulk = 0,		///< Allows to map always from the beginning of the buffer and shaders have access to the whole buffer.
			eSubRange		///< Allows to map in the middle of the buffer and shaders don't necessarily have access to the whole buffer.
		};

	public:
		/// @param mapMode				Mapping mode in which this buffer will work.
		/// @param defaultBufferSize	Default buffer size used for newly created buffers within this pool.
		explicit HlmsBufferPool(MapMode mapMode, size_t defaultBufferSize, HlmsBufferHandlerPtr pBufferHandler);
		~HlmsBufferPool() override;

		/// Bind the buffer to the shaders.
		void bindBuffer(CommandBuffer* pCommandBuffer);

		/// Maps a buffer and prepares it for filling the data.
		/// if m_MapMode == MapMode::eBulk:     Maps always next buffer and data will be written at the begining of the buffer. minimumSizeBytes is ignored
		/// if m_MapMode == MapMode::eSubRange: Tries to map region of minimumSizeBytes (or till the end) within the current buffer. If there is not enough space (minimumSizeBytes > 0), maps next buffer.
		/// 
		/// note: pCommandBuffer can be nullptr only if map mode is set to MapMode::eBulk - in this case the buffer is not bound immediately, but caller
		///       is responsible to call bindBuffer manually later.
		void mapBuffer(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes = 0);

		/// Unmaps the buffer.
		void unmapBuffer(CommandBuffer* pCommandBuffer);

		/// Write data to the buffer - must be mapped by previous call mapNextBuffer.
        /// @throw Exception if a buffer is not mapped or if there is not enough space for the numBytes.
		void writeData(const void* pData, size_t numBytes);

		/// Get the pointer for writing a value in the buffer. Must be mapped by previous call mapNextBuffer.
		template<typename T>
		T* insertValue()
		{
			if (mStartPtr == nullptr)
				OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Buffer is not mapped.", "HlmsBufferPool::insertValue");

			const size_t written = (mCurrentPtr - mStartPtr) * sizeof(uint8_t);
			if (written + sizeof(T) > mCapacity)
				OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Buffer overflow.", "HlmsBufferPool::insertValue");

			T* pValue = reinterpret_cast<T*>(mCurrentPtr);
			mCurrentPtr += sizeof(T);
			return pValue;
		}

		/// Get the pointer for writing multiple values in the buffer. Must be mapped by previous call mapNextBuffer.
		template<typename T>
		T* insertValues(size_t numValues)
		{
			if (mStartPtr == nullptr)
				OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Buffer is not mapped.", "HlmsBufferPool::insertValues");

			const size_t written = (mCurrentPtr - mStartPtr) * sizeof(uint8_t);
			if (written + numValues * sizeof(T) > mCapacity)
				OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Buffer overflow.", "HlmsBufferPool::insertValues");

			T* pValue = reinterpret_cast<T*>(mCurrentPtr);
			mCurrentPtr += numValues * sizeof(T);
			return pValue;
		}

		/// Check whether there is enough space in the currently mapped range to write give size of data.
		bool canWrite(size_t numBytes) const;

		/// Get the currently written data since last MapBuffer call.
		size_t getWrittenSize() const;

	public: // HlmsBufferPoolInterface implementation
		void onSetBinding(uint8_t stages, uint16_t slot) override final;
		void onSetVaoManager() override final;
		void destroyAllBuffers() override final;
		void analyzeBarriers(BarrierSolver& barrierSolver, ResourceTransitionArray& resourceTransitions, Camera& renderingCamera) override;
		void preCommandBufferExecution(CommandBuffer* pCommandBuffer) override final;
		void postCommandBufferExecution(CommandBuffer* pCommandBuffer) override final;
		void frameEnded() override final;

	protected:
		using HlmsBufferPoolInterface::bindBuffer;

	private:
		void updateShaderBufferCommand(CommandBuffer* pCommandBuffer);

		/// Implementation of MapMode::eBulk
		void mapBufferBulk(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes);
		void unmapBufferBulk();

		/// Implementation of MapMode::eIndirect
		void mapBufferSubRange(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes);
		void mapNextBufferSubRange(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes);
		void unmapBufferSubRange(CommandBuffer* pCommandBuffer);

	private:
		/// Mapping mode for the buffer. Set through the constructor and cannot be changed later.
		const MapMode mMapMode;

		/// The buffer size used for creating new buffers.
		size_t mBufferSize;

		uint8_t* mStartPtr; ///< Start of the writing to the buffer.
		uint8_t* mCurrentPtr; ///< Current offset where next data can be written (mStartPtr <= mCurrentPtr). Number of written bytes = mCurrentPtr - mStartPtr
		size_t mCapacity; ///< Number of bytes allowed to write to the buffer at mStartPtr.

		uint8_t* mMappedPtr; ///< Pointer returned by BufferPacked::map call.
		size_t mMappedOffset; ///< Offset from the beginning of the buffer where the buffer was mapped.

		size_t mCurrentIndex; ///< Resets to zero every new frame. Index to mBuffers which buffer is currently used for writing.
		vector<BufferPacked*>::type mBuffers; ///< Buffer pool.

		/// Offset in CommandBuffer where CbShaderBuffer was created. If written data size is not known at the beginning of mapping of
		/// the buffer, the command offset of the CbShaderBuffer in CommandBuffer is saved, and later when new mapping is requested,
		/// the written data size is updated in that CbShaderBuffer.
		size_t mShaderBufferCommandOffset;

		/// Delayed binding - supported only for MapMode::eBulk when mapBuffer was called with nullptr CommandBuffer.
		size_t mBindOffset;
		size_t mBindBufferIndex;
	};
} // namespace Ogre
