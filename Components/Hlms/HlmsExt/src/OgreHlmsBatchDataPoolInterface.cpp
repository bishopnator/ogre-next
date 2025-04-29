#include "OgreHlmsBatchDataPoolInterface.h"
#include "OgreHlmsBufferHandler.h"

// OGRE
#include <OgreCamera.h>
#include <OgreRenderSystem.h>
#include <Vao/OgreBufferPacked.h>
#include <Vao/OgreVaoManager.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsBatchDataPoolInterface::HlmsBatchDataPoolInterface(size_t defaultBufferSize, HlmsBufferHandlerPtr pBufferHandler)
: HlmsBufferPoolInterface(defaultBufferSize, std::move(pBufferHandler))
, mNextId(0)
, mLastBoundBuffer(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
HlmsBatchDataPoolInterface::~HlmsBatchDataPoolInterface()
{
	assert(mBuffers.empty()); // Forgot to call destroyAllBuffers()?
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::onSetBinding(uint8_t /*stages*/, uint16_t /*slot*/)
{
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::onSetVaoManager()
{
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::destroyAllBuffers()
{
	auto itor = mBuffers.begin();
	auto end = mBuffers.end();

	while (itor != end)
	{
		destroyBuffer((*itor)->mBuffer);
		deleteBatchDataBuffer(*itor);
		++itor;
	}

	mBuffers.clear();
	mSlots.clear();
	mNextId = 0;
	mUnusedIds.clear();
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::analyzeBarriers(BarrierSolver& barrierSolver, ResourceTransitionArray& resourceTransitions, Camera& renderingCamera)
{
	const auto resourceAccess = mBufferHandler->analyzeBarriers(*renderingCamera.getSceneManager());
	if (resourceAccess != std::nullopt)
	{
		const auto value = resourceAccess.value();
		for (auto* pBuffer : mBuffers)
			barrierSolver.resolveTransition(resourceTransitions, pBuffer->mBuffer, value, mStages);
	}
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::preCommandBufferExecution(CommandBuffer* /*pCommandBuffer*/)
{
	mLastBoundBuffer = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::postCommandBufferExecution(CommandBuffer* /*pCommandBuffer*/)
{
	mLastBoundBuffer = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::frameEnded()
{
}

//////////////////////////////////////////////////////////////////////////
bool HlmsBatchDataPoolInterface::bindBuffer(HlmsBatchDataSlotId id, CommandBuffer* pCommandBuffer)
{
	const auto found = mSlots.find(id);
	if (found == mSlots.end())
		return false;

	if (preBindBatchDataBuffer(*found->second.mOwner))
		mLastBoundBuffer = nullptr; // force bind

	if (mLastBoundBuffer == found->second.mOwner)
		return false;

	bindBuffer(*pCommandBuffer, *found->second.mOwner->mBuffer, uint32_t(0));
	mLastBoundBuffer = found->second.mOwner;
	return true;
}

//////////////////////////////////////////////////////////////////////////
BatchDataSlotInfo HlmsBatchDataPoolInterface::addSlot(size_t numBytes)
{
	// Find a slot with enough free space.
	BatchDataBuffer* pBatchDataBuffer = nullptr;
	for (auto* pCreatedBatchDataBuffer : mBuffers)
	{
		if (pCreatedBatchDataBuffer->mBuffer->getTotalSizeBytes() - pCreatedBatchDataBuffer->mUsed >= numBytes)
		{
			pBatchDataBuffer = pCreatedBatchDataBuffer;
			break;
		}
	}

	if (pBatchDataBuffer == nullptr)
	{
		// Get the required buffer size.
		const auto bufferSize = alignToNextMultiple(std::max(numBytes, mDefaultBufferSize), getBufferAlignment());
		if (bufferSize > getMaxBufferSize())
			return BatchDataSlotInfo{};

		auto* pBuffer = createBuffer(bufferSize);
		if (pBuffer == nullptr)
			return BatchDataSlotInfo{};

		// Create new buffer.
		pBatchDataBuffer = createBatchDataBuffer(bufferSize);
		pBatchDataBuffer->mBuffer = pBuffer;
		mBuffers.insert(pBatchDataBuffer);
	}

	// Notify implementation about prepared BatchDataBuffer for adding new slot into it.
	onBatchDataBufferPrepared(*pBatchDataBuffer);

	// Initialize the slot.
	BatchDataSlotInfo out;
	out.mSlot.mOwner = pBatchDataBuffer;
	out.mSlot.mSize = numBytes;
	out.mSlot.mOffset = pBatchDataBuffer->mUsed;
	pBatchDataBuffer->mUsed += numBytes;

	// Store the slot.
	out.mId = createId();
	mSlots.try_emplace(out.mId, out.mSlot);
	pBatchDataBuffer->mSlotIds.insert(out.mId);

	return out;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::removeSlot(HlmsBatchDataSlotId id)
{
	const auto found = mSlots.find(id);
	if (found == mSlots.end())
		return;

	{
		const auto& slot = found->second;
		slot.mOwner->mSlotIds.erase(id);
		if (slot.mOwner->mSlotIds.empty())
		{
			mBuffers.erase(slot.mOwner);
			destroyBuffer(slot.mOwner->mBuffer);
			delete slot.mOwner;
		}
		else
		{
			onBatchDataBufferUpdated(*slot.mOwner);
			slot.mOwner->mUsed -= slot.mSize;
		}
	}

	mSlots.erase(found);
	mUnusedIds.push_back(id);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBatchDataPoolInterface::removeAll()
{
	mLastBoundBuffer = nullptr;
	mNextId = 0;
	mSlots.clear();
	mUnusedIds.clear();

	for (auto* pBuffer : mBuffers)
	{
		pBuffer->mSlotIds.clear();
		pBuffer->mUsed = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsBatchDataPoolInterface::getDataOffset(HlmsBatchDataSlotId id) const
{
	const auto found = mSlots.find(id);
	return (found != mSlots.end() ? found->second.mOffset : 0);
}

//////////////////////////////////////////////////////////////////////////
HlmsBatchDataSlotId HlmsBatchDataPoolInterface::createId()
{
	if (!mUnusedIds.empty())
	{
		const auto id = mUnusedIds.back();
		mUnusedIds.pop_back();
		return id;
	}

	const auto id = mNextId;
	++mNextId;
	return id;
}

//////////////////////////////////////////////////////////////////////////
BatchDataSlot* HlmsBatchDataPoolInterface::findSlot(HlmsBatchDataSlotId id)
{
	const auto found = mSlots.find(id);
	return found != mSlots.end() ? &found->second : nullptr;
}
