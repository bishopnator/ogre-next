#include "OgreHlmsBatchDataPool.h"
#include "OgreHlmsBufferHandler.h"

// OGRE
#include <OgreRenderSystem.h>
// #include <CommandBuffer/OgreCbShaderBuffer.h>
// #include <CommandBuffer/OgreCommandBuffer.h>
#include <Vao/OgreBufferPacked.h>
#include <Vao/OgreVaoManager.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
bool HlmsBatchDataPool::BatchDataBufferExt::hasFlag(BatchDataBufferFlags flag) const
{
	return (mFlags & (1 << static_cast<uint8_t>(flag))) != 0;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPool::BatchDataBufferExt::setFlag(BatchDataBufferFlags flag)
{
	mFlags |= (1 << static_cast<uint8_t>(flag));
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPool::BatchDataBufferExt::clearFlag(BatchDataBufferFlags flag)
{
	mFlags &= (~(1 << static_cast<uint8_t>(flag)));
}

//////////////////////////////////////////////////////////////////////////
HlmsBatchDataPool::HlmsBatchDataPool(size_t defaultBufferSize, HlmsBufferHandlerPtr pBufferHandler)
: HlmsBatchDataPoolInterface(defaultBufferSize, std::move(pBufferHandler))
{
}

//////////////////////////////////////////////////////////////////////////
HlmsBatchDataPool::~HlmsBatchDataPool()
{
	destroyAllBuffers();
}

//////////////////////////////////////////////////////////////////////////
HlmsBatchDataSlotId HlmsBatchDataPool::addData(const void* pData, size_t numBytes)
{
	const auto slotInfo = addSlot(numBytes);
	if (slotInfo.mId == cInvalidBatchDataSlotId)
		return cInvalidBatchDataSlotId;

	// Copy the data.
	auto& batchDataBufferExt = static_cast<BatchDataBufferExt&>(*slotInfo.mSlot.mOwner);
	memcpy(batchDataBufferExt.mBufferData.data() + slotInfo.mSlot.mOffset, pData, slotInfo.mSlot.mSize);

	// Update the flags.
	batchDataBufferExt.setFlag(BatchDataBufferFlags::eUpload);

	return slotInfo.mId;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPool::removeData(HlmsBatchDataSlotId id)
{
	removeSlot(id);
}

//////////////////////////////////////////////////////////////////////////
BatchDataBuffer* HlmsBatchDataPool::createBatchDataBuffer(size_t numBytes)
{
	auto* pBatchdataBuffer = new BatchDataBufferExt();
	pBatchdataBuffer->mBufferData.resize(numBytes);
	return pBatchdataBuffer;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPool::deleteBatchDataBuffer(BatchDataBuffer* pBatchDataBuffer)
{
	auto* pBatchDataBufferExt = static_cast<BatchDataBufferExt*>(pBatchDataBuffer);
	delete pBatchDataBufferExt;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPool::onBatchDataBufferPrepared(BatchDataBuffer& batchDataBuffer)
{
	auto& batchDataBufferExt = static_cast<BatchDataBufferExt&>(batchDataBuffer);
	if (!batchDataBufferExt.hasFlag(BatchDataBufferFlags::eUpdate))
		return;

	std::vector<uint8_t> updated(batchDataBufferExt.mBufferData.size());

	size_t offset = 0;
	for (const auto& id : batchDataBufferExt.mSlotIds)
	{
		auto* pSlot = findSlot(id);
		if (pSlot != nullptr)
		{
			memcpy(updated.data() + offset, batchDataBufferExt.mBufferData.data() + pSlot->mOffset, pSlot->mSize);
			pSlot->mOffset = offset;
			offset += pSlot->mSize;
		}
	}

	batchDataBufferExt.mBufferData = std::move(updated);

	batchDataBufferExt.clearFlag(BatchDataBufferFlags::eUpdate);
	batchDataBufferExt.setFlag(BatchDataBufferFlags::eUpload);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPool::onBatchDataBufferUpdated(BatchDataBuffer& batchDataBuffer)
{
	auto& batchDataBufferExt = static_cast<BatchDataBufferExt&>(batchDataBuffer);
	batchDataBufferExt.setFlag(BatchDataBufferFlags::eUpdate);
}

//////////////////////////////////////////////////////////////////////////
bool HlmsBatchDataPool::preBindBatchDataBuffer(BatchDataBuffer& batchDataBuffer)
{
	auto& batchDataBufferExt = static_cast<BatchDataBufferExt&>(batchDataBuffer);
	if (!batchDataBufferExt.hasFlag(BatchDataBufferFlags::eUpload))
		return false;

	auto* pData = batchDataBufferExt.mBuffer->map(0, batchDataBufferExt.mBufferData.size());
	memcpy(pData, batchDataBufferExt.mBufferData.data(), batchDataBufferExt.mBufferData.size());
	batchDataBufferExt.mBuffer->unmap(UO_UNMAP_ALL);
	batchDataBufferExt.clearFlag(BatchDataBufferFlags::eUpload);
	return true;
}
