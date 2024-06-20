#pragma once
#include "Hlms/OgreHlmsExt.h"

// OGRE
#include <OgreMatrix4.h>

namespace Demo
{
	/// Implementation of Ogre::HlmsExt.
	class CustomHlms : public Ogre::HlmsExt
	{
	public:
		struct PassBuffer final
		{
			Ogre::Matrix4 m_ViewProjMatrix;
		};

	public:
		explicit CustomHlms(Ogre::Archive* pDataFolder, Ogre::ArchiveVec* pLibraryFolders);
		~CustomHlms() override;

		/** Fill the provided string and string vector with all the sub-folder needed to instantiate
			an CustomHlms object with the default distribution of the HlmsResources.
			These paths are dependent of the current RenderSystem.

			This method can only be called after a valid RenderSystem has been chosen.

			All output parameter's content will be replaced with the new set of paths.
		@param outDataFolderPath
			Path (as a String) used for creating the "dataFolder" Archive the constructor will need
		@param outLibraryFoldersPaths
			Vector of String used for creating the ArchiveVector "libraryFolders" the constructor will
		need
		*/
		static void getDefaultPaths(Ogre::String& outDataFolderPath, Ogre::StringVector& outLibraryFoldersPaths);

	public: /// Ogre::HlmsExt interface
		Ogre::HlmsCache preparePassHash(const Ogre::CompositorShadowNode* pShadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* pSceneManager) override;
		Ogre::uint32 fillBuffersForV1(const Ogre::HlmsCache* pCache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::CommandBuffer* pCommandBuffer) override;
		Ogre::uint32 fillBuffersForV2(const Ogre::HlmsCache* pCache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::CommandBuffer* pCommandBuffer) override;

	protected:
		void setupDescBindingRanges(Ogre::DescBindingRange* descBindingRanges) override;
		Ogre::HlmsDatablock* createDatablockImpl(Ogre::IdString datablockName, const Ogre::HlmsMacroblock* pMacroblock, const Ogre::HlmsBlendblock* pBlendblock, const Ogre::HlmsParamVec& paramVec) override;

	private:
		Ogre::HlmsBufferPool* mPassBufferPool = nullptr;
		Ogre::HlmsBufferPool* mWorldMatricesBufferPool = nullptr;
		Ogre::HlmsBufferPool* mMaterialIndicesBufferPool = nullptr;
	};
} // namespace Demo
