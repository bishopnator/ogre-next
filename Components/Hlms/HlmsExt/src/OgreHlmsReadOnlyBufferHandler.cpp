#include "OgreHlmsReadOnlyBufferHandler.h"
#include "OgreHlmsBufferHandlerCommon.h"

// OGRE
#include <Vao/OgreReadOnlyBufferPacked.h>
#include <vao/OgreVaoManager.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsReadOnlyBufferHandler::HlmsReadOnlyBufferHandler() = default;
HlmsReadOnlyBufferHandler::~HlmsReadOnlyBufferHandler() = default;

//////////////////////////////////////////////////////////////////////////
std::optional<ResourceAccess::ResourceAccess> HlmsReadOnlyBufferHandler::analyzeBarriers(SceneManager& /*sceneManager*/)
{
	return ResourceAccess::Read;
}

//////////////////////////////////////////////////////////////////////////
BufferPacked* HlmsReadOnlyBufferHandler::createBuffer(VaoManager& vaoManager, size_t bufferSize)
{
	assert(bufferSize <= vaoManager.getReadOnlyBufferMaxSize());
	return vaoManager.createReadOnlyBuffer(PFG_RGBA32_FLOAT, bufferSize, BT_DYNAMIC_PERSISTENT, 0, false);
}

//////////////////////////////////////////////////////////////////////////
void HlmsReadOnlyBufferHandler::destroyBuffer(VaoManager& vaoManager, BufferPacked* pBuffer)
{
	vaoManager.destroyReadOnlyBuffer(static_cast<ReadOnlyBufferPacked*>(pBuffer));
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsReadOnlyBufferHandler::getBufferAlignment(VaoManager& vaoManager) const
{
	return vaoManager.getTexBufferAlignment();
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsReadOnlyBufferHandler::getMaxBufferSize(VaoManager& vaoManager) const
{
	return vaoManager.getReadOnlyBufferMaxSize();
}

//////////////////////////////////////////////////////////////////////////
void HlmsReadOnlyBufferHandler::bindBuffer(CommandBuffer& commandBuffer, uint8_t stages, uint16_t slot, BufferPacked& buffer, size_t bindOffset)
{
	Details::bindBuffer(commandBuffer, stages, slot, static_cast<ReadOnlyBufferPacked&>(buffer), static_cast<uint32_t>(bindOffset));
}
