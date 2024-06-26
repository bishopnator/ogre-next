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
#include <Compositor/OgreCompositorShadowNode.h>

// std
#include <array>

using namespace Demo;

//////////////////////////////////////////////////////////////////////////
CustomHlms::CustomHlms(Ogre::Archive* pDataFolder, Ogre::ArchiveVec* pLibraryFolders)
: Ogre::HlmsExt(Ogre::HLMS_USER0, "CustomHlms", pDataFolder, pLibraryFolders)
{
	mPassBufferPool = &createConstBufferPool({ Ogre::VertexShader }, 0, sizeof(PassBuffer));

	// max. space for 4096 instances
	constexpr size_t cMaxNumInstances = 4096;
	mInstanceBuffer = &createConstBufferPool({ Ogre::VertexShader, Ogre::PixelShader }, 1, cMaxNumInstances * sizeof(Ogre::Vector4));
	mWorldMatricesBufferPool = &createReadOnlyBufferPool({ Ogre::VertexShader }, 0, cMaxNumInstances * sizeof(Ogre::Matrix4));

	// datablocks
	setDatablocksPrimaryBufferParams({ Ogre::PixelShader }, 2, CustomHlmsDataBlock::getPrimaryBufferDatablockSize());
	if (CustomHlmsDataBlock::getSecondaryBufferDatablockSize() != 0)
		setDatablocksSecondaryBufferParams({ Ogre::PixelShader }, 2, CustomHlmsDataBlock::getSecondaryBufferDatablockSize());
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
    outLibraryFoldersPaths.push_back( "Hlms/Common/Any" );

	// Fill the data folder path
	outDataFolderPath = "Hlms/CustomHlms/" + shaderSyntax;
}

//////////////////////////////////////////////////////////////////////////
Ogre::HlmsCache CustomHlms::preparePassHash(const Ogre::CompositorShadowNode* pShadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* pSceneManager)
{
	OgreProfileExhaustive("CustomHlms::preparePassHash");
	Ogre::HlmsCache retVal = Ogre::HlmsExt::preparePassHash(pShadowNode, casterPass, dualParaboloid, pSceneManager);

	// fill matrices to the pass buffer
	Ogre::CamerasInProgress cameras = pSceneManager->getCamerasInProgress();
Ogre::Matrix4 viewMatrix = cameras.renderingCamera->getViewMatrix(true);

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
	PassBuffer* pPassBuffer = mPassBufferPool->insertValue<PassBuffer>();
	if (casterPass && pShadowNode != nullptr)
	{
		Ogre::Real fNear, fFar;
		pShadowNode->getMinMaxDepthRange(cameras.renderingCamera, fNear, fFar);
		const Ogre::Real depthRange = fFar - fNear;
		pPassBuffer->mDepthRange.x = fNear;
		pPassBuffer->mDepthRange.y = 1.0f / depthRange;
		pPassBuffer->mDepthRange.z = 0.0;
		pPassBuffer->mDepthRange.w = 0.0;
	}
	pPassBuffer->mViewProjMatrix = projectionMatrix * viewMatrix;

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
Ogre::uint32 CustomHlms::fillBuffersForV2(const Ogre::HlmsCache* pCache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::CommandBuffer* pCommandBuffer)
{
	assert(dynamic_cast<const CustomHlmsDataBlock*>(queuedRenderable.renderable->getDatablock()) != nullptr);
    const CustomHlmsDataBlock* pDatablock = static_cast<const CustomHlmsDataBlock*>(queuedRenderable.renderable->getDatablock());

	// Check if there is enough space for the new instance.
	if (!mWorldMatricesBufferPool->canWrite(sizeof(Ogre::Matrix4)) || !mInstanceBuffer->canWrite(sizeof(Ogre::Vector4)))
	{
		mWorldMatricesBufferPool->mapBuffer(pCommandBuffer, sizeof(Ogre::Matrix4));
		mInstanceBuffer->mapBuffer(pCommandBuffer, sizeof(Ogre::Vector4));
	}

	// Write a world transformation for the renderable.
	static_assert(sizeof(Ogre::Matrix4) == sizeof(float) * 16, "Ogre::Matrix4 has not expected size!");
	const Ogre::Matrix4& worldMat = queuedRenderable.movableObject->_getParentNodeFullTransform();
	mWorldMatricesBufferPool->writeData(&worldMat, sizeof(Ogre::Matrix4));

	// Write instance data.
	static_assert(sizeof(Ogre::Vector4) == sizeof(float) * 4, "Ogre::Vector4 has not expected size!");
	Ogre::Vector4* pInstancedData = mInstanceBuffer->insertValue<Ogre::Vector4>();
	pInstancedData->x = static_cast<float>(pDatablock->getAssignedSlot());
	pInstancedData->y = pDatablock->mShadowConstantBias * mConstantBiasScale;

	// Get the instance index.
	return static_cast<uint32_t>(mInstanceBuffer->getWrittenSize()) / sizeof(Ogre::Vector4) - 1;
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
