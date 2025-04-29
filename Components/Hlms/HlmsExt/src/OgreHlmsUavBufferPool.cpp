#include "OgreHlmsUavBufferPool.h"
#include "OgreHlmsUavBufferHandler.h"

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsUavBufferPool::HlmsUavBufferPool(HlmsExt& hlms, uint16_t writeSlot, uint16_t readSlot, size_t elementSize, size_t numElements, const ResourceAccessMap& resourceAccessMap)
: HlmsBatchDataPoolInterface(elementSize * numElements, std::make_unique<HlmsUavBufferHandler>(hlms, writeSlot, readSlot, elementSize, resourceAccessMap))
{
}

//////////////////////////////////////////////////////////////////////////
HlmsUavBufferPool::~HlmsUavBufferPool()
{
	destroyAllBuffers();
}

//////////////////////////////////////////////////////////////////////////
BatchDataBuffer* HlmsUavBufferPool::createBatchDataBuffer(size_t /*numBytes*/)
{
	return new BatchDataBuffer();
}

//////////////////////////////////////////////////////////////////////////
BatchDataSlotInfo HlmsUavBufferPool::addSlot(size_t numElements)
{
	const auto slotInfo = HlmsBatchDataPoolInterface::addSlot(numElements * getElementSize());

	// Convert bytes to elements.
	BatchDataSlotInfo out(slotInfo);
	out.mSlot.mOffset /= getElementSize();
	out.mSlot.mSize /= getElementSize();
	return out;
}

//////////////////////////////////////////////////////////////////////////
void HlmsUavBufferPool::removeAll()
{
	HlmsBatchDataPoolInterface::removeAll();
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsUavBufferPool::getElementSize() const
{
	return static_cast<const HlmsUavBufferHandler&>(*mBufferHandler).getElementSize();
}

//////////////////////////////////////////////////////////////////////////
void HlmsUavBufferPool::deleteBatchDataBuffer(BatchDataBuffer* pBatchDataBuffer)
{
	delete pBatchDataBuffer;
}

//////////////////////////////////////////////////////////////////////////
void HlmsUavBufferPool::onBatchDataBufferPrepared(BatchDataBuffer& /*batchDataBuffer*/)
{
}

//////////////////////////////////////////////////////////////////////////
void HlmsUavBufferPool::onBatchDataBufferUpdated(BatchDataBuffer& /*batchDataBuffer*/)
{
}

//////////////////////////////////////////////////////////////////////////
bool HlmsUavBufferPool::preBindBatchDataBuffer(BatchDataBuffer& /*batchDataBuffer*/)
{
	return false;
}
