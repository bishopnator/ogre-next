#pragma once
#include "OgreHlmsBatchDataPoolInterface.h"
#include "OgreHlmsUavBufferHandlerFwd.h"

// std
#include <unordered_set>

// forwards
namespace Ogre
{
	class HlmsExt;
}

namespace Ogre
{
	/// Data pool for providing support for accessing UAV buffers and preparing layout of them during processing the renderables (HlmsExt::fillBuffersForV2). The CompositorPassScene
	/// should write data to the buffers and another CompositorPassScene should use them for rendering. The layout is specified by reserving space in the buffer - there is no ability
	/// to write data to the buffer from CPU. InstanceData should hold the offset in the buffer where one pass will write and another will read.
	class _OgreHlmsExtExport HlmsUavBufferPool final : public HlmsBatchDataPoolInterface
	{
	public:
		explicit HlmsUavBufferPool(HlmsExt& hlms, uint16_t writeSlot, uint16_t readSlot, size_t elementSize, size_t numElements, const ResourceAccessMap& resourceAccessMap);
		~HlmsUavBufferPool() override;

		/// Add a slot by specifying its size in number of elements (in contrast to parent HlmsBatchDataPoolInterface::addSlot which expects size in bytes).
		BatchDataSlotInfo addSlot(size_t numElements);

		/// Remove all slots.
		void removeAll();

		/// Get element size.
		size_t getElementSize() const;

	private: // HlmsBatchDataPoolInterface implementation
		BatchDataBuffer* createBatchDataBuffer(size_t numBytes) override;
		void deleteBatchDataBuffer(BatchDataBuffer* pBatchDataBuffer) override;
		void onBatchDataBufferPrepared(BatchDataBuffer& batchDataBuffer) override;
		void onBatchDataBufferUpdated(BatchDataBuffer& batchDataBuffer) override;
		bool preBindBatchDataBuffer(BatchDataBuffer& batchDataBuffer) override;
	};
} // namespace Ogre
