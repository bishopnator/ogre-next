#include "OgreHlmsConstBufferPool.h"
#include "OgreHlmsBufferPoolCommon.h"

// OGRE
#include <Vao/OgreConstBufferPacked.h>
#include <Vao/OgreVaoManager.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsConstBufferPool::HlmsConstBufferPool(size_t defaultBufferSize)
: HlmsBufferPool(MapMode::eBulk, defaultBufferSize)
{
}

//////////////////////////////////////////////////////////////////////////
HlmsConstBufferPool::~HlmsConstBufferPool()
{
	destroyAllBuffers();
}

//////////////////////////////////////////////////////////////////////////
BufferPacked* HlmsConstBufferPool::createBuffer(size_t bufferSize)
{
	assert(bufferSize <= mVaoManager->getConstBufferMaxSize());
	return mVaoManager->createConstBuffer(bufferSize, BT_DYNAMIC_PERSISTENT, 0, false);
}

//////////////////////////////////////////////////////////////////////////
void HlmsConstBufferPool::destroyBuffer(BufferPacked* pBuffer)
{
	mVaoManager->destroyConstBuffer(static_cast<ConstBufferPacked*>(pBuffer));
}

//////////////////////////////////////////////////////////////////////////
void HlmsConstBufferPool::bindBuffer(CommandBuffer& commandBuffer, BufferPacked& buffer, size_t bindOffset)
{
	Details::bindBuffer(commandBuffer, mStages, mSlot, static_cast<ConstBufferPacked&>(buffer), static_cast<uint32_t>(bindOffset));
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsConstBufferPool::getBufferAlignment() const
{
	return mVaoManager->getConstBufferAlignment();
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsConstBufferPool::getMaxBufferSize() const
{
	return std::min<size_t>(65536, mVaoManager->getConstBufferMaxSize()); // limit the size to maximum 65536 bytes
}
