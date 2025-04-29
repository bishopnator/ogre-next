#include "OgreHlmsBufferPoolInterface.h"
#include "OgreHlmsBufferHandler.h"

// OGRE
#include <OgreRenderSystem.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsBufferPoolInterface::HlmsBufferPoolInterface(size_t defaultBufferSize, HlmsBufferHandlerPtr pBufferHandler)
: mBufferHandler(std::move(pBufferHandler))
, mVaoManager(nullptr)
, mStages(0)
, mSlot(0)
, mDefaultBufferSize(defaultBufferSize)
{
}

//////////////////////////////////////////////////////////////////////////
HlmsBufferPoolInterface::~HlmsBufferPoolInterface() = default;

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPoolInterface::setBinding(uint8_t stages, uint16_t slot)
{
	onSetBinding(stages, slot);
	mStages = stages & c_allGraphicStagesMask;
	mSlot = slot;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPoolInterface::setRenderSystem(Ogre::RenderSystem* pRenderSystem)
{
	if (mVaoManager != nullptr)
	{
		destroyAllBuffers();
		mVaoManager = nullptr;
	}

	if (pRenderSystem != nullptr)
	{
		mVaoManager = pRenderSystem->getVaoManager();
		onSetVaoManager();
	}
}

//////////////////////////////////////////////////////////////////////////
BufferPacked* HlmsBufferPoolInterface::createBuffer(size_t bufferSize)
{
	return mBufferHandler->createBuffer(*mVaoManager, bufferSize);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPoolInterface::destroyBuffer(BufferPacked* pBuffer)
{
	mBufferHandler->destroyBuffer(*mVaoManager, pBuffer);
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsBufferPoolInterface::getBufferAlignment() const
{
	return mBufferHandler->getBufferAlignment(*mVaoManager);
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsBufferPoolInterface::getMaxBufferSize() const
{
	return mBufferHandler->getMaxBufferSize(*mVaoManager);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPoolInterface::bindBuffer(CommandBuffer& commandBuffer, BufferPacked& buffer, size_t bindOffset)
{
	mBufferHandler->bindBuffer(commandBuffer, mStages, mSlot, buffer, bindOffset);
}
