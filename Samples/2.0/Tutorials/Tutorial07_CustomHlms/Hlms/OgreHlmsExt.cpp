#include "OgreHlmsExt.h"
#include "OgreHlmsConstBufferPool.h"
#include "OgreHlmsReadOnlyBufferPool.h"

// OGRE
#include <OgreHlmsListener.h>
#include <OgreRenderable.h>
#include <OgreRenderQueue.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsExt::HlmsExt(HlmsTypes type, const String& typeName, Archive* pDataFolder, ArchiveVec* pLibraryFolders)
: Hlms(type, typeName, pDataFolder, pLibraryFolders)
{
}

//////////////////////////////////////////////////////////////////////////
HlmsExt::~HlmsExt()
{
	for (auto* pBufferPool : mBufferPools)
		OGRE_DELETE pBufferPool;
}

//////////////////////////////////////////////////////////////////////////
HlmsBufferPool& HlmsExt::createConstBufferPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize)
{
	mBufferPools.push_back(OGRE_NEW HlmsConstBufferPool(bufferSize));

	uint8_t _stages = 0;
	for (const auto& shaderType : stages)
		_stages |= 1 << shaderType;

	if (_stages == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "No shader stage specified!", "HlmsExt::createConstBufferPool");

	mBufferPools.back()->setBinding(_stages, slot);
	return *mBufferPools.back();
}

//////////////////////////////////////////////////////////////////////////
HlmsBufferPool& HlmsExt::createReadOnlyBufferPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize)
{
	mBufferPools.push_back(OGRE_NEW HlmsReadOnlyBufferPool(bufferSize));

	uint8_t _stages = 0;
	for (const auto& shaderType : stages)
		_stages |= 1 << shaderType;

	if (_stages == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "No shader stage specified!", "HlmsExt::createReadOnlyBufferPool");

	mBufferPools.back()->setBinding(_stages, slot);
	return *mBufferPools.back();
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::_changeRenderSystem(RenderSystem* pNewRenderSystem)
{
	for (auto* pBufferPool : mBufferPools)
		pBufferPool->setRenderSystem(pNewRenderSystem);

	Hlms::_changeRenderSystem(pNewRenderSystem);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::preCommandBufferExecution(CommandBuffer* pCommandBuffer)
{
	for (auto* pBufferPool : mBufferPools)
		pBufferPool->preCommandBufferExecution(pCommandBuffer);

	Hlms::preCommandBufferExecution(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::postCommandBufferExecution(CommandBuffer* pCommandBuffer)
{
	for (auto* pBufferPool : mBufferPools)
		pBufferPool->postCommandBufferExecution(pCommandBuffer);

	Hlms::postCommandBufferExecution(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::frameEnded(void)
{
	for (auto* pBufferPool : mBufferPools)
		pBufferPool->frameEnded();

	Hlms::frameEnded();
}

//////////////////////////////////////////////////////////////////////////
uint32 HlmsExt::fillBuffersFor(const HlmsCache* /*pCache*/, const QueuedRenderable& /*queuedRenderable*/, bool /*casterPass*/, uint32 /*lastCacheHash*/, uint32 /*lastTextureHash*/)
{
	OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
		"Trying to use slow-path on a desktop implementation. "
		"Change the RenderQueue settings.",
		"HlmsExt::fillBuffersFor");
}

//////////////////////////////////////////////////////////////////////////
uint32 HlmsExt::fillBuffersForV1(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* pCommandBuffer)
{
	if (OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH(lastCacheHash) != static_cast<uint32_t>(mType))
	{
		const auto texUnit = onHlmsTypeChanged(casterPass, pCommandBuffer, queuedRenderable);
		mListener->hlmsTypeChanged(casterPass, pCommandBuffer, queuedRenderable.renderable->getDatablock(), texUnit);
	}

	return fillBuffersForV1(pCache, queuedRenderable, casterPass, pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
uint32 HlmsExt::fillBuffersForV2(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* pCommandBuffer)
{
	if (OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH(lastCacheHash) != static_cast<uint32_t>(mType))
	{
		const auto texUnit = onHlmsTypeChanged(casterPass, pCommandBuffer, queuedRenderable);
		mListener->hlmsTypeChanged(casterPass, pCommandBuffer, queuedRenderable.renderable->getDatablock(), texUnit);
	}

	return fillBuffersForV2(pCache, queuedRenderable, casterPass, pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::setupRootLayout(RootLayout& rootLayout)
{
	setupDescBindingRanges(rootLayout.mDescBindingRanges[0]);
	mListener->setupRootLayout(rootLayout, mSetProperties);
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsExt::onHlmsTypeChanged(bool /*casterPass*/, CommandBuffer* pCommandBuffer, const QueuedRenderable& /*queuedRenderable*/)
{
	for (auto* pBufferPool : mBufferPools)
		pBufferPool->bindBuffer(pCommandBuffer);

	return mReadOnlyBufferPoolsCount;
}
