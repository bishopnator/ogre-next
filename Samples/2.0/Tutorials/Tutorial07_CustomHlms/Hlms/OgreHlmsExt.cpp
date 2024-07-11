#include "OgreHlmsExt.h"
#include "OgreHlmsConstBufferPool.h"
#include "OgreHlmsReadOnlyBufferPool.h"

// OGRE
#include <OgreCamera.h>
#include <OgreHlmsDatablock.h>
#include <OgreHlmsListener.h>
#include <OgreLogManager.h>
#include <OgreRenderable.h>
#include <OgreRenderQueue.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>

using namespace Ogre;

//////////////////////////////////////////////////////////////////////////
HlmsExt::HlmsExt(HlmsTypes type, const String& typeName, Archive* pDataFolder, ArchiveVec* pLibraryFolders)
: Hlms(type, typeName, pDataFolder, pLibraryFolders)
, mReadOnlyBufferPoolsCount(0)
, mConstantBiasScale(0.1f)
{
	// Add folders also to resource manager so the #include directives in the shaders works properly.
	if (pDataFolder != nullptr)
		Root::getSingleton().addResourceLocation(pDataFolder->getName(), "FileSystem");

	if (pLibraryFolders != nullptr)
	{
		for (auto* pLibraryFolder : *pLibraryFolders)
		{
			Root::getSingleton().addResourceLocation(pLibraryFolder->getName(), "FileSystem");

			// load the header files
			auto pHeaders = pLibraryFolder->find("*.h");
			if (pHeaders != nullptr)
			{
				for (const auto& header : *pHeaders)
				{
					try
					{
						auto pData = pLibraryFolder->open(header);
						m_HeaderFiles[header] = pData->getAsString();
					}
					catch (const Ogre::Exception& e)
					{
						LogManager::getSingleton().logMessage("The header file '" + header + "' from '" + pLibraryFolder->getName() + "' cannot be loaded.");
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
HlmsExt::~HlmsExt()
{
	for (HlmsBufferPool* pBufferPool : mBufferPools)
		OGRE_DELETE pBufferPool;
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::setDatablocksPrimaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize)
{
	mMaterialBufferPool.setPrimaryBufferParams(stages, slot, datablockSize);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::setDatablocksSecondaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize, BufferType bufferType, bool useReadOnlyBuffers)
{
	mMaterialBufferPool.setSecondaryBufferParams(stages, slot, datablockSize, bufferType, useReadOnlyBuffers);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::requestDatablockSlot(ConstBufferPoolUser& datablock, uint32_t hash, bool useSecondaryBuffer)
{
	mMaterialBufferPool.requestSlot(hash, &datablock, useSecondaryBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::releaseDatablockSlot(ConstBufferPoolUser& datablock)
{
	if (datablock.getAssignedPool() != nullptr)
		mMaterialBufferPool.releaseSlot(&datablock);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::invalidateDatablockSlot(ConstBufferPoolUser& datablock)
{
	assert(datablock.getAssignedPool() != nullptr);
	mMaterialBufferPool.scheduleForUpdate(&datablock);
}

//////////////////////////////////////////////////////////////////////////
HlmsBufferPool& HlmsExt::createConstBufferPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize)
{
	mBufferPools.push_back(OGRE_NEW HlmsConstBufferPool(bufferSize));

	uint8_t _stages = 0;
	for (const ShaderType& shaderType : stages)
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
	for (const ShaderType& shaderType : stages)
		_stages |= 1 << shaderType;

	if (_stages == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "No shader stage specified!", "HlmsExt::createReadOnlyBufferPool");

	mBufferPools.back()->setBinding(_stages, slot);
	return *mBufferPools.back();
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::_changeRenderSystem(RenderSystem* pNewRenderSystem)
{
	for (HlmsBufferPool* pBufferPool : mBufferPools)
		pBufferPool->setRenderSystem(pNewRenderSystem);

	mMaterialBufferPool._changeRenderSystem(pNewRenderSystem);
	Hlms::_changeRenderSystem(pNewRenderSystem);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::preCommandBufferExecution(CommandBuffer* pCommandBuffer)
{
	for (HlmsBufferPool* pBufferPool : mBufferPools)
		pBufferPool->preCommandBufferExecution(pCommandBuffer);

	Hlms::preCommandBufferExecution(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::postCommandBufferExecution(CommandBuffer* pCommandBuffer)
{
	for (HlmsBufferPool* pBufferPool : mBufferPools)
		pBufferPool->postCommandBufferExecution(pCommandBuffer);

	Hlms::postCommandBufferExecution(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::frameEnded(void)
{
	for (HlmsBufferPool* pBufferPool : mBufferPools)
		pBufferPool->frameEnded();

	Hlms::frameEnded();
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::calculateHashForPreCreate(Renderable* pRenderable, PiecesMap* pInOutPieces)
{
	Hlms::calculateHashForPreCreate(pRenderable, pInOutPieces);

	// inject header files
	for (int k = 0; k < NumShaderTypes; ++k)
		pInOutPieces[k].insert(m_HeaderFiles.begin(), m_HeaderFiles.end());
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::calculateHashForPreCaster(Renderable* pRenderable, PiecesMap* pInOutPieces)
{
	Hlms::calculateHashForPreCaster(pRenderable, pInOutPieces);

	// inject header files
	for( int k = 0; k < NumShaderTypes; ++k )
		pInOutPieces[k].insert(m_HeaderFiles.begin(), m_HeaderFiles.end());
}

//////////////////////////////////////////////////////////////////////////
HlmsCache HlmsExt::preparePassHash(const CompositorShadowNode* pShadowNode, bool casterPass, bool dualParaboloid, SceneManager* pSceneManager)
{
	CamerasInProgress cameras = pSceneManager->getCamerasInProgress();
	mConstantBiasScale = cameras.renderingCamera != nullptr ? cameras.renderingCamera->_getConstantBiasScale() : 0.1f;

	HlmsCache hlmsCache = Hlms::preparePassHash(pShadowNode, casterPass, dualParaboloid, pSceneManager);

	// Upload all dirty datablocks to the buffers.
	mMaterialBufferPool.updateBuffers();

	return hlmsCache;
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
		const size_t texUnit = onHlmsTypeChanged(casterPass, pCommandBuffer, queuedRenderable);
		mListener->hlmsTypeChanged(casterPass, pCommandBuffer, queuedRenderable.renderable->getDatablock(), texUnit);
	}

	// Don't bind the material buffer on caster passes (important to keep MDI & auto-instancing running on shadow map passes).
	auto* pDatablock = queuedRenderable.renderable->getDatablock();
	if (!casterPass || pDatablock->getAlphaTest() != CMPF_ALWAYS_PASS)
	{
		const ConstBufferPoolUser* pBufferPoolUser = dynamic_cast<const ConstBufferPoolUser*>(pDatablock);
		if (pBufferPoolUser != nullptr && pBufferPoolUser->getAssignedPool() != nullptr)
			mMaterialBufferPool.bindBuffers(*pBufferPoolUser->getAssignedPool(), *pCommandBuffer);
	}

	return fillBuffersForV1(pCache, queuedRenderable, casterPass, pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
uint32 HlmsExt::fillBuffersForV2(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* pCommandBuffer)
{
	if (OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH(lastCacheHash) != static_cast<uint32_t>(mType))
	{
		const size_t texUnit = onHlmsTypeChanged(casterPass, pCommandBuffer, queuedRenderable);
		mListener->hlmsTypeChanged(casterPass, pCommandBuffer, queuedRenderable.renderable->getDatablock(), texUnit);
	}

	// Don't bind the material buffer on caster passes (important to keep MDI & auto-instancing running on shadow map passes).
	auto* pDatablock = queuedRenderable.renderable->getDatablock();
	if (!casterPass || pDatablock->getAlphaTest() != CMPF_ALWAYS_PASS)
	{
		const ConstBufferPoolUser* pBufferPoolUser = dynamic_cast<const ConstBufferPoolUser*>(pDatablock);
		if (pBufferPoolUser != nullptr && pBufferPoolUser->getAssignedPool() != nullptr)
			mMaterialBufferPool.bindBuffers(*pBufferPoolUser->getAssignedPool(), *pCommandBuffer);
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
	for (HlmsBufferPool* pBufferPool : mBufferPools)
		pBufferPool->bindBuffer(pCommandBuffer);

	mMaterialBufferPool.onHlmsTypeChanged();

	return mReadOnlyBufferPoolsCount;
}
