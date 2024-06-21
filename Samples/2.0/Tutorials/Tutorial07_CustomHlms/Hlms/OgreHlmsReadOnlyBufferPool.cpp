#include "OgreHlmsReadOnlyBufferPool.h"
#include "OgreHlmsBufferPoolCommon.h"

// OGRE
#include <Vao/OgreReadOnlyBufferPacked.h>
#include <Vao/OgreVaoManager.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsReadOnlyBufferPool::HlmsReadOnlyBufferPool(size_t defaultBufferSize)
: HlmsBufferPool(MapMode::eSubRange, defaultBufferSize)
{
}

//////////////////////////////////////////////////////////////////////////
HlmsReadOnlyBufferPool::~HlmsReadOnlyBufferPool()
{
	destroyAllBuffers();
}

//////////////////////////////////////////////////////////////////////////
BufferPacked* HlmsReadOnlyBufferPool::createBuffer(size_t bufferSize)
{
	assert(bufferSize <= mVaoManager->getReadOnlyBufferMaxSize());
	return mVaoManager->createReadOnlyBuffer(PFG_RGBA32_FLOAT, bufferSize, BT_DYNAMIC_PERSISTENT, 0, false);
}

//////////////////////////////////////////////////////////////////////////
void HlmsReadOnlyBufferPool::destroyBuffer(BufferPacked* pBuffer)
{
	mVaoManager->destroyReadOnlyBuffer(static_cast<ReadOnlyBufferPacked*>(pBuffer));
}

//////////////////////////////////////////////////////////////////////////
void HlmsReadOnlyBufferPool::bindBuffer(CommandBuffer& commandBuffer, BufferPacked& buffer, size_t bindOffset)
{
	Details::bindBuffer(commandBuffer, mStages, mSlot, static_cast<ReadOnlyBufferPacked&>(buffer), static_cast<uint32_t>(bindOffset));
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsReadOnlyBufferPool::getBufferAlignment() const
{
	return mVaoManager->getTexBufferAlignment();
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsReadOnlyBufferPool::getMaxBufferSize() const
{
	return mVaoManager->getReadOnlyBufferMaxSize();
}
