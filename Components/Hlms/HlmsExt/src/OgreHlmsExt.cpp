#include "OgreHlmsExt.h"
#include "OgreHlmsBatchDataPool.h"
#include "OgreHlmsBufferPool.h"
#include "OgreHlmsConstBufferHandler.h"
#include "OgreHlmsReadOnlyBufferHandler.h"
#include "OgreHlmsUavBufferPool.h"
#include "OgreHlsl2Glsl.h"

// OGRE
#include <OgreCamera.h>
#include <OgreFileSystem.h>
#include <OgreHlmsDatablock.h>
#include <OgreHlmsListener.h>
#include <OgreHlmsManager.h>
#include <OgreLogManager.h>
#include <OgreRenderable.h>
#include <OgreRenderQueue.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>

// std
#include <fstream>

namespace
{
	/// List of supported shader conversions by the HlmsExt.
	/// The entries have format "target.source".
	const std::unordered_set<std::string> cSupportedConversion = { "glsl.hlsl" };
}

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
						mHeaderFiles[header] = pData->getAsString();
					}
					catch (const Ogre::Exception&)
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
	for (HlmsBufferPoolInterface* pBufferPool : mBufferPools)
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
size_t HlmsExt::getDatablocksBufferCapacity() const
{
	return mMaterialBufferPool.getSlotsPerPool();
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
HlmsBufferPool& HlmsExt::createBufferPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize, HlmsBufferHandlerPtr pBufferHandler)
{
	uint8_t _stages = 0;
	for (const ShaderType& shaderType : stages)
		_stages |= 1 << shaderType;

	if (_stages == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "No shader stage specified!", "HlmsExt::createBufferPool");

	if (dynamic_cast<const HlmsReadOnlyBufferHandler*>(pBufferHandler.get()) != nullptr)
		++mReadOnlyBufferPoolsCount;

	const auto bufferPoolMapMode = dynamic_cast<const HlmsConstBufferHandler*>(pBufferHandler.get()) != nullptr ? HlmsBufferPool::MapMode::eBulk : HlmsBufferPool::MapMode::eSubRange;
	mBufferPools.push_back(OGRE_NEW HlmsBufferPool(bufferPoolMapMode, bufferSize, std::move(pBufferHandler)));
	mBufferPools.back()->setBinding(_stages, slot);
	return static_cast<HlmsBufferPool&>(*mBufferPools.back());
}

//////////////////////////////////////////////////////////////////////////
HlmsUavBufferPool& HlmsExt::createUavBufferPool(uint16_t writeSlot, uint16_t readSlot, size_t elementSize, size_t numElements, const ResourceAccessMap& resourceAccessMap)
{
	mBufferPools.push_back(OGRE_NEW HlmsUavBufferPool(*this, writeSlot, readSlot, elementSize, numElements, resourceAccessMap));
	return static_cast<HlmsUavBufferPool&>(*mBufferPools.back());
}

//////////////////////////////////////////////////////////////////////////
const DescriptorSetUav* HlmsExt::updateDescriptorUavSet(uint16_t slot, UavBufferPacked& buffer, size_t bindOffset, size_t sizeBytes)
{
	if (slot >= mDescriptorSetUav.mUavs.size())
		mDescriptorSetUav.mUavs.resize(slot + 1);

	mDescriptorSetUav.mUavs[slot].slotType = DescriptorSetUav::SlotTypeBuffer;
	mDescriptorSetUav.mUavs[slot].getBuffer().makeEmpty();
	mDescriptorSetUav.mUavs[slot].getBuffer().buffer = &buffer;
	mDescriptorSetUav.mUavs[slot].getBuffer().offset = bindOffset;
	mDescriptorSetUav.mUavs[slot].getBuffer().sizeBytes = sizeBytes;
	mDescriptorSetUav.mUavs[slot].getBuffer().access = ResourceAccess::Write;
	return mHlmsManager->getDescriptorSetUav(mDescriptorSetUav);
}

//////////////////////////////////////////////////////////////////////////
HlmsBatchDataPool& HlmsExt::createBatchDataPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize, HlmsBufferHandlerPtr pBufferHandler)
{
	uint8_t _stages = 0;
	for (const ShaderType& shaderType : stages)
		_stages |= 1 << shaderType;

	if (_stages == 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "No shader stage specified!", "HlmsExt::createBatchDataPool");

	if (dynamic_cast<const HlmsReadOnlyBufferHandler*>(pBufferHandler.get()) != nullptr)
		++mReadOnlyBufferPoolsCount;

	mBufferPools.push_back(OGRE_NEW HlmsBatchDataPool(bufferSize, std::move(pBufferHandler)));
	mBufferPools.back()->setBinding(_stages, slot);
	return static_cast<HlmsBatchDataPool&>(*mBufferPools.back());
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::_changeRenderSystem(RenderSystem* pNewRenderSystem)
{
	for (HlmsBufferPoolInterface* pBufferPool : mBufferPools)
		pBufferPool->setRenderSystem(pNewRenderSystem);

	mMaterialBufferPool._changeRenderSystem(pNewRenderSystem);
	Hlms::_changeRenderSystem(pNewRenderSystem);
	
	// Run auto-detection for the shader files.
	if (pNewRenderSystem != nullptr)
		autodetectShaderFiles();
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::preCommandBufferExecution(CommandBuffer* pCommandBuffer)
{
	for (HlmsBufferPoolInterface* pBufferPool : mBufferPools)
		pBufferPool->preCommandBufferExecution(pCommandBuffer);

	Hlms::preCommandBufferExecution(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::postCommandBufferExecution(CommandBuffer* pCommandBuffer)
{
	for (HlmsBufferPoolInterface* pBufferPool : mBufferPools)
		pBufferPool->postCommandBufferExecution(pCommandBuffer);

	Hlms::postCommandBufferExecution(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::frameEnded(void)
{
	for (HlmsBufferPoolInterface* pBufferPool : mBufferPools)
		pBufferPool->frameEnded();

	Hlms::frameEnded();
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::calculateHashForPreCreate(Renderable* pRenderable, PiecesMap* pInOutPieces)
{
	Hlms::calculateHashForPreCreate(pRenderable, pInOutPieces);

	// inject header files
	for (int k = 0; k < NumShaderTypes; ++k)
		pInOutPieces[k].insert(mHeaderFiles.begin(), mHeaderFiles.end());
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::calculateHashForPreCaster(Renderable* pRenderable, PiecesMap* pInOutPieces, const PiecesMap* pNormalPassPieces)
{
	Hlms::calculateHashForPreCaster(pRenderable, pInOutPieces, pNormalPassPieces);

	// inject header files
	for( int k = 0; k < NumShaderTypes; ++k )
		pInOutPieces[k].insert(mHeaderFiles.begin(), mHeaderFiles.end());
}

//////////////////////////////////////////////////////////////////////////
HlmsCache HlmsExt::preparePassHash(const CompositorShadowNode* pShadowNode, bool casterPass, bool dualParaboloid, SceneManager* pSceneManager)
{
	CamerasInProgress cameras = pSceneManager->getCamerasInProgress();
	mConstantBiasScale = cameras.renderingCamera != nullptr ? cameras.renderingCamera->_getConstantBiasScale() : 0.1f;

	mT[kNoTid].setProperties.clear();
	onPreparePassHash(pShadowNode, casterPass, dualParaboloid, pSceneManager);
	HlmsCache hlmsCache = Hlms::preparePassHashBase(pShadowNode, casterPass, dualParaboloid, pSceneManager);

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
void HlmsExt::analyzeBarriers(BarrierSolver& barrierSolver, ResourceTransitionArray& resourceTransitions, Camera* renderingCamera, const bool bCasterPass)
{
	Hlms::analyzeBarriers(barrierSolver, resourceTransitions, renderingCamera, bCasterPass);

	for (HlmsBufferPoolInterface* pBufferPool : mBufferPools)
		pBufferPool->analyzeBarriers(barrierSolver, resourceTransitions, *renderingCamera);
}

//////////////////////////////////////////////////////////////////////////
HighLevelGpuProgramPtr HlmsExt::compileShaderCode(const String& source, const String& debugFilenameOutput, uint32 finalHash, ShaderType shaderType, size_t tid)
{
	// If source already begins with #version, it has GLSL format. If mShadeFileExt is .hlsl, it means,
	// that the shader is loaded from cache and hence it was already converted to GLSL.
	if (mShaderFileExt == ".hlsl" && mShaderProfile == "glsl" && source.find("#version") == String::npos)
	{
		// Convert HLSL to GLSL.
		RootLayout rootLayout;
		setupRootLayout(rootLayout, tid);
		const auto glsl = convertHlsl2Glsl(source, debugFilenameOutput, shaderType, rootLayout, mDataFolder);
		
		String debugFilenameOutputGlsl = debugFilenameOutput;
		if (mDebugOutput && !debugFilenameOutput.empty())
		{
			// Update debug filename and replace extension.
			debugFilenameOutputGlsl = debugFilenameOutput.substr(0, debugFilenameOutput.size() - 5) + "_converted.glsl";

			std::ofstream debugDumpFile;
			debugDumpFile.open(debugFilenameOutputGlsl.c_str(), std::ios::out | std::ios::binary);

			// Write properties which generated the shader.
			if (mDebugOutputProperties)
				dumpProperties(debugDumpFile, tid);

			// Write GLSL shader to the debug file.
			debugDumpFile.write(glsl.c_str(), glsl.size() );
		}

		return Hlms::compileShaderCode(glsl, debugFilenameOutputGlsl, finalHash, shaderType, tid);
	}
	
	// Compile the shader.
	return Hlms::compileShaderCode(source, debugFilenameOutput, finalHash, shaderType, tid);
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::setupRootLayout(RootLayout& rootLayout, size_t tid)
{
	// Setup non-baked binding ranges.
	rootLayout.mBaked[0] = false;
	setupDescBindingRangesNonBaked(rootLayout.mDescBindingRanges[0]);
	
	// Setup baked binding ranges.
	rootLayout.mBaked[1] = true;
	setupDescBindingRangesBaked(rootLayout.mDescBindingRanges[1]);

	// Notify also listener about the root layout.
	mListener->setupRootLayout(rootLayout, mT[tid].setProperties, tid);
}

//////////////////////////////////////////////////////////////////////////
bool HlmsExt::autodetectShaderFiles()
{
	// Get the file(s) for the vertex shaders - it is sufficient to check only one.
	const auto pFiles = mDataFolder->find("*_vs.*", false, false);
	if (pFiles == nullptr || pFiles->empty())
	{
		LogManager::getSingleton().logMessage(LML_NORMAL, "HLMS data folder doesn't contain *_vs.* file(s).");
		return false;
	}

	// Simple helper functor to extract file's extension.
	const auto getFileExtension = [](const String& file) {
		const auto ix = file.rfind('.');
		return ix != String::npos ? file.substr(ix) : String();
	};

	String shaderFileExt;
	for (const auto& file : *pFiles)
	{
		// Get the extension.
		const auto ext = getFileExtension(file);
		if (ext == mShaderFileExt)
			return true; // The file extension matches the predefined file extension by parent class.
		
		// Check if it is possible to convert the shader files to mShaderProfile.
		if (cSupportedConversion.find(mShaderProfile + ext) != cSupportedConversion.end())
		{
			// Shader profile is supported by the RenderSystem.
			shaderFileExt = ext;

			// Don't break the loop to allow to select the shaders chosen by parent class (mShaderFileExt is already set).
		}
	}

	if (shaderFileExt.empty())
	{
		LogManager::getSingleton().logMessage(LML_NORMAL, "HLMS data folder doesn't contain shaders supported by current RenderSystem.");
		return false;
	}

	// Overwrite shader file extension.
	mShaderFileExt = shaderFileExt;
	return true;
}

//////////////////////////////////////////////////////////////////////////
void HlmsExt::onPreparePassHash(const CompositorShadowNode* /*pShadowNode*/, bool /*casterPass*/, bool /*dualParaboloid*/, SceneManager* /*pSceneManager*/)
{
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsExt::onHlmsTypeChanged(bool /*casterPass*/, CommandBuffer* pCommandBuffer, const QueuedRenderable& /*queuedRenderable*/)
{
	for (HlmsBufferPoolInterface* pBufferPoolInterface : mBufferPools)
	{
		if (auto* pBufferPool = dynamic_cast<HlmsBufferPool*>(pBufferPoolInterface); pBufferPool != nullptr)
			pBufferPool->bindBuffer(pCommandBuffer);
	}

	mMaterialBufferPool.onHlmsTypeChanged();

	return mReadOnlyBufferPoolsCount;
}
