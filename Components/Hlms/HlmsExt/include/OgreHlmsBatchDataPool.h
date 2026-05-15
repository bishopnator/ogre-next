#pragma once
#include "OgreHlmsBatchDataPoolInterface.h"

namespace Ogre
{
	/// Data pool for providing support for uploading a mesh data to a buffer. Data from multiple meshes can be stored in a single buffer which reduces bindings.
	/// The pool has a predefined capacity which allows to batch multiple data together. It however allows to store even larger data if needed. In this cases
	/// a special buffer with desired capacity is created.
	class _OgreHlmsExtExport HlmsBatchDataPool final : public HlmsBatchDataPoolInterface
	{
	public:
		enum class BatchDataBufferFlags : uint8_t
		{
			eUpdate = 0, ///< The slots are not well organized withing the buffer - there are gaps between them.
			eUpload, ///< It is necessary to upload data to GPU.

			Count
		};

		/// Created buffer with slots.
		struct BatchDataBufferExt : public BatchDataBuffer
		{
			/// Buffer data on CPU prepared for upload.
			std::vector<uint8_t> mBufferData;

			/// Flags to indicate a state of the buffer.
			uint8_t mFlags = 0;

			bool hasFlag(BatchDataBufferFlags flag) const;
			void setFlag(BatchDataBufferFlags flag);
			void clearFlag(BatchDataBufferFlags flag);
		};

	public:
		explicit HlmsBatchDataPool(size_t defaultBufferSize, HlmsBufferHandlerPtr pBufferHandler);
		~HlmsBatchDataPool() override;

		/// Add data to the pool.
		HlmsBatchDataSlotId addData(const void* pData, size_t numBytes);

		/// Remove data from the pool.
		void removeData(HlmsBatchDataSlotId id);

	private: // HlmsBatchDataPoolInterface implementation
		BatchDataBuffer* createBatchDataBuffer(size_t numBytes) override;
		void deleteBatchDataBuffer(BatchDataBuffer* pBatchDataBuffer) override;
		void onBatchDataBufferPrepared(BatchDataBuffer& batchDataBuffer) override;
		void onBatchDataBufferUpdated(BatchDataBuffer& batchDataBuffer) override;
		bool preBindBatchDataBuffer(BatchDataBuffer& batchDataBuffer) override;
	};
} // namespace Ogre
