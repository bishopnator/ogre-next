#include "OgreHlmsUavBufferHandler.h"
#include "OgreHlmsBufferHandlerCommon.h"
#include "OgreHlmsExt.h"

// OGRE
#include <OgreSceneManager.h>
#include <CommandBuffer/OgreCbSetUavs.h>
#include <CommandBuffer/OgreCommandBuffer.h>
#include <Vao/OgreUavBufferPacked.h>
#include <vao/OgreVaoManager.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsUavBufferHandler::HlmsUavBufferHandler(HlmsExt& hlms, uint16_t writeSlot, uint16_t readSlot, size_t elementSize, const ResourceAccessMap& resourceAccessMap)
: mHlms(hlms)
, mElementSize(static_cast<uint32_t>(elementSize))
, mResourceAccessMap(resourceAccessMap)
, mCurrentResourceAccess(ResourceAccess::Undefined)
, mWriteSlot(writeSlot)
, mReadSlot(readSlot)
{
}

//////////////////////////////////////////////////////////////////////////
HlmsUavBufferHandler::~HlmsUavBufferHandler() = default;

//////////////////////////////////////////////////////////////////////////
size_t HlmsUavBufferHandler::getElementSize() const
{
	return static_cast<size_t>(mElementSize);
}

//////////////////////////////////////////////////////////////////////////
std::optional<ResourceAccess::ResourceAccess> HlmsUavBufferHandler::analyzeBarriers(SceneManager& sceneManager)
{
	if (sceneManager.getCurrentCompositorPass() == nullptr)
		return std::nullopt;

	const auto compositorPassId = sceneManager.getCurrentCompositorPass()->getDefinition()->mIdentifier;
	const auto found = mResourceAccessMap.find(compositorPassId);
	if (found == mResourceAccessMap.end())
	{
		mCurrentResourceAccess = ResourceAccess::Undefined;
		return std::nullopt;
	}

	mCurrentResourceAccess = found->second;
	return found->second;
}

//////////////////////////////////////////////////////////////////////////
BufferPacked* HlmsUavBufferHandler::createBuffer(VaoManager& vaoManager, size_t bufferSize)
{
	assert(bufferSize <= vaoManager.getUavBufferMaxSize());
	return vaoManager.createUavBuffer(bufferSize / mElementSize, mElementSize, BB_FLAG_UAV | BB_FLAG_READONLY, 0, false);
}

//////////////////////////////////////////////////////////////////////////
void HlmsUavBufferHandler::destroyBuffer(VaoManager& vaoManager, BufferPacked* pBuffer)
{
	vaoManager.destroyUavBuffer(static_cast<UavBufferPacked*>(pBuffer));
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsUavBufferHandler::getBufferAlignment(VaoManager& vaoManager) const
{
	return vaoManager.getUavBufferAlignment();
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsUavBufferHandler::getMaxBufferSize(VaoManager& vaoManager) const
{
	return vaoManager.getUavBufferMaxSize();
}

//////////////////////////////////////////////////////////////////////////
void HlmsUavBufferHandler::bindBuffer(CommandBuffer& commandBuffer, uint8_t /*stages*/, uint16_t /*slot*/, BufferPacked& buffer, size_t bindOffset)
{
	if (mCurrentResourceAccess == ResourceAccess::Write)
	{
		*commandBuffer.addCommand<CbSetUavs>() = CbSetUavs(1, mHlms.updateDescriptorUavSet(mWriteSlot - 1, static_cast<UavBufferPacked&>(buffer), bindOffset, 0));
	}
	else
	{
		Details::bindBuffer(commandBuffer, 1 << GeometryShader, mReadSlot, *static_cast<UavBufferPacked&>(buffer).getAsReadOnlyBufferView(), static_cast<uint32_t>(bindOffset));
	}
}
