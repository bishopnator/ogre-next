#include "OgreHlmsMaterialBufferPool.h"
#include "OgreHlmsBufferHandlerCommon.h"

// OGRE
#include <OgreException.h>
#include <Vao/OgreConstBufferPacked.h>
#include <Vao/OgreReadOnlyBufferPacked.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsMaterialBufferPool::HlmsMaterialBufferPool()
: ConstBufferPool(0, ExtraBufferParams())
, mPrimaryBufferStages(0)
, mPrimaryBufferSlot(0)
, mSecondaryBufferStages(0)
, mSecondaryBufferSlot(0)
, mLastBoundPool(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
HlmsMaterialBufferPool::~HlmsMaterialBufferPool()
{
}

//////////////////////////////////////////////////////////////////////////
bool HlmsMaterialBufferPool::IsValid() const
{
	return mBytesPerSlot > 0;
}

//////////////////////////////////////////////////////////////////////////
void HlmsMaterialBufferPool::setPrimaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize)
{
	if (mSecondaryBufferStages != 0 && mSecondaryBufferSlot == slot)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Primary buffer cannot be bound to the same slot as secondary buffer.", "HlmsMaterialBufferPool::setPrimaryBufferParams");
	if (datablockSize == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Datablock is expected to write some data to a primary buffer.", "HlmsMaterialBufferPool::setPrimaryBufferParams");

	uint8_t _stages = 0;
	for (const auto& shaderType : stages)
		_stages |= 1 << shaderType;

	if (_stages == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "No shader stage specified!", "HlmsMaterialBufferPool::setPrimaryBufferParams");

	mPrimaryBufferStages = _stages;
	mPrimaryBufferSlot = slot;
	mBytesPerSlot = static_cast<uint32_t>(datablockSize);
}

//////////////////////////////////////////////////////////////////////////
void HlmsMaterialBufferPool::setSecondaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize, BufferType bufferType, bool useReadOnlyBuffers)
{
	if (mPrimaryBufferStages != 0 && mPrimaryBufferSlot == slot)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Secondary buffer cannot be bound to the same slot as primary buffer.", "HlmsMaterialBufferPool::setSecondaryBufferParams");
	if (datablockSize == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Datablock is expected to write some data to a primary buffer.", "HlmsMaterialBufferPool::setSecondaryBufferParams");

	uint8_t _stages = 0;
	for (const auto& shaderType : stages)
		_stages |= 1 << shaderType;

	if (_stages == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "No shader stage specified!", "HlmsMaterialBufferPool::setSecondaryBufferParams");

	mSecondaryBufferStages = _stages;
	mSecondaryBufferSlot = slot;
	mExtraBufferParams = ExtraBufferParams(datablockSize, bufferType, useReadOnlyBuffers);
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsMaterialBufferPool::getSlotsPerPool() const
{
	return mSlotsPerPool;
}

//////////////////////////////////////////////////////////////////////////
void HlmsMaterialBufferPool::onHlmsTypeChanged()
{
	mLastBoundPool = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void HlmsMaterialBufferPool::bindBuffers(const BufferPool& bufferPool, CommandBuffer& commandBuffer)
{
	if (!IsValid())
		return;

	if (mLastBoundPool != &bufferPool)
	{
		// Bind primary buffer.
		Details::bindBuffer(commandBuffer, mPrimaryBufferStages, mPrimaryBufferSlot, static_cast<ConstBufferPacked&>(*bufferPool.materialBuffer), static_cast<uint32_t>(0));

		// Bind secondary buffer.
		if (bufferPool.extraBuffer != nullptr)
		{
			if (mExtraBufferParams.useReadOnlyBuffers)
				Details::bindBuffer(commandBuffer, mSecondaryBufferStages, mSecondaryBufferSlot, static_cast<ReadOnlyBufferPacked&>(*bufferPool.extraBuffer), static_cast<uint32_t>(0));
			else
				Details::bindBuffer(commandBuffer, mSecondaryBufferStages, mSecondaryBufferSlot, static_cast<ConstBufferPacked&>(*bufferPool.extraBuffer), static_cast<uint32_t>(0));
		}
		mLastBoundPool = &bufferPool;
	}
}

//////////////////////////////////////////////////////////////////////////
void HlmsMaterialBufferPool::updateBuffers()
{
	uploadDirtyDatablocks();
}

//////////////////////////////////////////////////////////////////////////
void HlmsMaterialBufferPool::_changeRenderSystem(RenderSystem* pRenderSystem)
{
	if (!IsValid())
		return;

	mLastBoundPool = nullptr;
	ConstBufferPool::_changeRenderSystem(pRenderSystem);
}
