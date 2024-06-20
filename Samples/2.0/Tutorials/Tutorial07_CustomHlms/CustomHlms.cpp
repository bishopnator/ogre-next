#include "CustomHlms.h"
#include "CustomHlmsDataBlock.h"
#include "Hlms/OgreHlmsBufferPool.h"

// OGRE
#include <OgreArchiveManager.h>
#include <OgreCamera.h>
#include <OgreHlmsManager.h>
#include <OgreProfiler.h>
#include <OgreRenderQueue.h>
#include <OgreRoot.h>
#include <OgreRenderSystem.h>

// std
#include <array>

using namespace Demo;

//////////////////////////////////////////////////////////////////////////
CustomHlms::CustomHlms(Ogre::Archive* pDataFolder, Ogre::ArchiveVec* pLibraryFolders)
: Ogre::HlmsExt(Ogre::HLMS_USER0, "CustomHlms", pDataFolder, pLibraryFolders)
{
	mPassBufferPool = &createConstBufferPool({ Ogre::VertexShader }, 0, sizeof(PassBuffer));

	// max. space for 4096 instances
	mMaterialIndicesBufferPool = &createConstBufferPool({ Ogre::PixelShader }, 1, 4096 * sizeof(uint32_t) * 4); // buffer in shader is mapped to uint4[]
	mWorldMatricesBufferPool = &createReadOnlyBufferPool({ Ogre::VertexShader }, 0, 4096 * sizeof(Ogre::Matrix4));
}

//////////////////////////////////////////////////////////////////////////
CustomHlms::~CustomHlms() = default;

//////////////////////////////////////////////////////////////////////////
void CustomHlms::getDefaultPaths(Ogre::String& outDataFolderPath, Ogre::StringVector& outLibraryFoldersPaths)
{
	// We need to know what RenderSystem is currently in use, as the
	// name of the compatible shading language is part of the path.
	Ogre::RenderSystem *renderSystem = Ogre::Root::getSingleton().getRenderSystem();
	Ogre::String shaderSyntax = "GLSL";
	if( renderSystem->getName() == "OpenGL ES 2.x Rendering Subsystem" )
		shaderSyntax = "GLSLES";
	else if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
		shaderSyntax = "HLSL";
	else if( renderSystem->getName() == "Metal Rendering Subsystem" )
		shaderSyntax = "Metal";

	// Fill the library folder paths with the relevant folders.
	outLibraryFoldersPaths.clear();
    outLibraryFoldersPaths.push_back( "Hlms/Common/" + shaderSyntax );

	// Fill the data folder path
	outDataFolderPath = "Hlms/CustomHlms/" + shaderSyntax;
}

//////////////////////////////////////////////////////////////////////////
Ogre::HlmsCache CustomHlms::preparePassHash(const Ogre::CompositorShadowNode* pShadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* pSceneManager)
{
	OgreProfileExhaustive("GpuMaterialManager::preparePassHash");
	mSetProperties.clear();

	Ogre::HlmsCache retVal = Ogre::HlmsExt::preparePassHashBase(pShadowNode, casterPass, dualParaboloid, pSceneManager);
	retVal.setProperties = mSetProperties;

	// fill matrices to pass buffer
	Ogre::CamerasInProgress cameras = pSceneManager->getCamerasInProgress();
	Ogre::Matrix4 viewMatrix = cameras.renderingCamera->getVrViewMatrix(0);

	Ogre::Matrix4 projectionMatrix = cameras.renderingCamera->getProjectionMatrixWithRSDepth();
	Ogre::RenderPassDescriptor* pRenderPassDesc = mRenderSystem->getCurrentPassDescriptor();

	if (pRenderPassDesc->requiresTextureFlipping())
	{
		projectionMatrix[1][0] = -projectionMatrix[1][0];
		projectionMatrix[1][1] = -projectionMatrix[1][1];
		projectionMatrix[1][2] = -projectionMatrix[1][2];
		projectionMatrix[1][3] = -projectionMatrix[1][3];
	}

	// Buffer will be bound later within fillBuffersForV2 so there is no problem to skip CommandBuffer now.
	mPassBufferPool->mapBuffer(nullptr);

	// Fill pass buffer.
	PassBuffer passBuffer;
	passBuffer.m_ViewProjMatrix = projectionMatrix * viewMatrix;
	mPassBufferPool->writeData(&passBuffer, sizeof(PassBuffer));

	// Pass buffer is filled - it is possible to unmap it now.
	mPassBufferPool->unmapBuffer(nullptr);

	return retVal;
}

//////////////////////////////////////////////////////////////////////////
Ogre::uint32 CustomHlms::fillBuffersForV1(const Ogre::HlmsCache* /*pCache*/, const Ogre::QueuedRenderable& /*queuedRenderable*/, bool /*casterPass*/, Ogre::CommandBuffer* /*pCommandBuffer*/)
{
	OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, "Not implemented yet!", "CustomHlms::fillBuffersForV1");
}

//////////////////////////////////////////////////////////////////////////
Ogre::uint32 CustomHlms::fillBuffersForV2(const Ogre::HlmsCache* /*pCache*/, const Ogre::QueuedRenderable& queuedRenderable, bool /*casterPass*/, Ogre::CommandBuffer* pCommandBuffer)
{
	// Check if there is enough space for the new instance.
	if (!mWorldMatricesBufferPool->canWrite(sizeof(Ogre::Matrix4)) || !mMaterialIndicesBufferPool->canWrite(sizeof(uint32_t) * 4))
	{
		mWorldMatricesBufferPool->mapBuffer(pCommandBuffer, sizeof(Ogre::Matrix4));
		mMaterialIndicesBufferPool->mapBuffer(pCommandBuffer, sizeof(uint32_t) * 4);
	}

	// write the data.
	static_assert(sizeof(Ogre::Matrix4) == sizeof(float) * 16, "Ogre::Matrix4 has not expected size!");
	const Ogre::Matrix4& worldMat = queuedRenderable.movableObject->_getParentNodeFullTransform();
	mWorldMatricesBufferPool->writeData(&worldMat, sizeof(Ogre::Matrix4));

	std::array<uint32_t, 4> materialIndex{0, 0, 0, 0};
	mMaterialIndicesBufferPool->writeData(materialIndex.data(), sizeof(uint32_t) * 4);

	// Get the instance index.
	return static_cast<uint32_t>(mMaterialIndicesBufferPool->getWrittenSize()) / (sizeof(uint32_t) * 4) - 1;
}

//////////////////////////////////////////////////////////////////////////
void CustomHlms::setupDescBindingRanges(Ogre::DescBindingRange* descBindingRanges)
{
	Ogre::setDescBindingRange(descBindingRanges[Ogre::DescBindingTypes::ConstBuffer], 0, 2);
	Ogre::setDescBindingRange(descBindingRanges[Ogre::DescBindingTypes::ReadOnlyBuffer], 0, 1);
}

//////////////////////////////////////////////////////////////////////////
Ogre::HlmsDatablock* CustomHlms::createDatablockImpl(Ogre::IdString datablockName, const Ogre::HlmsMacroblock* pMacroblock, const Ogre::HlmsBlendblock* pBlendblock, const Ogre::HlmsParamVec& paramVec)
{
	return OGRE_NEW CustomHlmsDataBlock(datablockName, this, pMacroblock, pBlendblock, paramVec);
}
