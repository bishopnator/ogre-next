#include "OgreHlmsConstBufferHandler.h"
#include "OgreHlmsBufferHandlerCommon.h"

// OGRE
#include <vao/OgreConstBufferPacked.h>
#include <vao/OgreVaoManager.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsConstBufferHandler::HlmsConstBufferHandler() = default;
HlmsConstBufferHandler::~HlmsConstBufferHandler() = default;

//////////////////////////////////////////////////////////////////////////
std::optional<ResourceAccess::ResourceAccess> HlmsConstBufferHandler::analyzeBarriers(SceneManager& sceneManager)
{
	return ResourceAccess::Read;
}

//////////////////////////////////////////////////////////////////////////
BufferPacked* HlmsConstBufferHandler::createBuffer(VaoManager& vaoManager, size_t bufferSize)
{
	assert(bufferSize <= vaoManager.getConstBufferMaxSize());
	return vaoManager.createConstBuffer(bufferSize, BT_DYNAMIC_PERSISTENT, 0, false);
}

//////////////////////////////////////////////////////////////////////////
void HlmsConstBufferHandler::destroyBuffer(VaoManager& vaoManager, BufferPacked* pBuffer)
{
	vaoManager.destroyConstBuffer(static_cast<ConstBufferPacked*>(pBuffer));
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsConstBufferHandler::getBufferAlignment(VaoManager& vaoManager) const
{
	return vaoManager.getConstBufferAlignment();
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsConstBufferHandler::getMaxBufferSize(VaoManager& vaoManager) const
{
	return std::min<size_t>(65536, vaoManager.getConstBufferMaxSize()); // limit the size to maximum 65536 bytes
}

//////////////////////////////////////////////////////////////////////////
void HlmsConstBufferHandler::bindBuffer(CommandBuffer& commandBuffer, uint8_t stages, uint16_t slot, BufferPacked& buffer, size_t bindOffset)
{
	Details::bindBuffer(commandBuffer, stages, slot, static_cast<ConstBufferPacked&>(buffer), static_cast<uint32_t>(bindOffset));
}
