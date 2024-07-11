#pragma once
#include "OgreHlmsMaterialBufferPool.h"

// OGRE
#include <OgreHlms.h>
#include <OgreRootLayout.h>
#include <OgreStd/vector.h>

// forwards
namespace Ogre
{
	struct DescBindingRange;
	class HlmsBufferPool;
}

namespace Ogre
{
	/// Base implementation of Hlms which uses HlmsBufferPool management.
	/// To make full implementation of Hlms using this class, it is necessary to implement the following virtual methods:
	/// * setupDescBindingRange
	/// * preparePassHash
	/// * fillBuffersForV1
	/// * fillBuffersForV2
	/// * createDatablockImpl
	/// 
	/// In the constructor of inherited class, it is necessary to create buffers used by the hlms.
	/// * createConstBufferPool/createReadOnlyBufferPool
	/// * setup buffers which hold the data from datablocks: setDatablocksPrimaryBufferParams/setDatablocksSecondaryBufferParams
	/// 
	/// The implementation of preparePassHash fills the buffer(s) with pass data which are the same for all rendered instances:
	/// * view-projection matrix, render target resolution, etc.
	/// The buffers are filled, but not bound - there is no access to the CommandBuffer and hence the mapBuffer is called with nullptr.
	/// This implies that only const buffers are possible to use for the pass data.
	/// 
	/// The setupDescBindingRange reflects the bindings setup in the constructor.
	/// 
	/// The fillBuffersForV1/fillBuffersForV2 fills the per instance data in the buffers.
	/// 
	/// The createDatablockImpl creates the custom datablock used by the hlms. The datablock contains parameters used by instances like
	/// colors, textures, transparency, etc. This is the closest to the "material". Multiple datablocks are stored in the same buffer so
	/// the multiple instances can be rendered together.
	class _OgreComponentExport HlmsExt : public Hlms
	{
	public:
		HlmsExt(HlmsTypes type, const String& typeName, Archive* dataFolder, ArchiveVec* pLibraryFolders);
		~HlmsExt() override;

		/// Setup the parameters for the primary buffers which hold the data from HlmsDatablocks. The primary buffer is always ConstBufferPacked.
		/// @see HlmsMaterialBufferPool::setPrimaryBufferParams
		void setDatablocksPrimaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize);

		/// Setup the parameters for the secondary buffers which hold the data from HlmsDatablocks. The secondary buffer is either ConstBufferPacked or ReadOnlyBufferPacked.
		/// @see HlmsMaterialBufferPool::setSecondaryBufferParams
		void setDatablocksSecondaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize, BufferType bufferType = BT_DEFAULT, bool useReadOnlyBuffers = true);

		/// Request a slot for the datablock (only if it inherits from ConstBufferPoolUser). This should be called by the datablock constructor.
		void requestDatablockSlot(ConstBufferPoolUser& datablock, uint32_t hash, bool useSecondaryBuffer);

		/// Release a slot from the datablock (only if it inherits from ConstBufferPoolUser). This should be called by the datablock destructor.
		void releaseDatablockSlot(ConstBufferPoolUser& datablock);

		/// Notify the hlms that the datablock has been changed. It will schedule the update when preparePassHash is called.
		void invalidateDatablockSlot(ConstBufferPoolUser& datablock);

		/// Create a const buffer pool with the requested bindings.
		/// @param stages		Combination of ShaderType values.
		/// @param slot			Slot index to which the buffers will be bound.
		/// @param bufferSize	Size of the created buffers within the buffer pool. The size is internally limited to 65536b or the VaoManager::getConstBufferMaxSize()
		HlmsBufferPool& createConstBufferPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize);

		/// Create a read-only buffer pool with the requested bindings.
		/// @param stages		Combination of ShaderType values.
		/// @param slot			Slot index to which the buffers will be bound.
		/// @param bufferSize	Size of the created buffers within the buffer pool. The size is internally limited to the VaoManager::getReadOnlyBufferMaxSize()
		HlmsBufferPool& createReadOnlyBufferPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize);

	public: // Hlms interface
		void _changeRenderSystem(RenderSystem* pNewRenderSystem) override;
		void preCommandBufferExecution(CommandBuffer* pCommandBuffer) override;
		void postCommandBufferExecution(CommandBuffer* pCommandBuffer) override;
		void frameEnded(void) override;
		void calculateHashForPreCreate(Renderable* pRenderable, PiecesMap* pInOutPieces) override;
		void calculateHashForPreCaster(Renderable* pRenderable, PiecesMap* pInOutPieces) override;
		HlmsCache preparePassHash(const CompositorShadowNode* pShadowNode, bool casterPass, bool dualParaboloid, SceneManager* pSceneManager) override;
		uint32 fillBuffersFor(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash) override;
		uint32 fillBuffersForV1(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* pCommandBuffer) override;
		uint32 fillBuffersForV2(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* pCommandBuffer) override;

	protected: // Hlms interface
		void setupRootLayout(RootLayout& rootLayout) override;

		/// Called from setupRootLayout to initialize the DescBindingRange to tell OGRE which slots are used.
		/// To access a DescBindingRange, use descBindingRanges[DescBindingTypes::xxx]
		virtual void setupDescBindingRanges(DescBindingRange* descBindingRanges) = 0;

	protected:
		/// Called from fillBuffersForV1/fillBuffersForV2 when Hlms types was changed since last renderable.
		/// @return Number of used texture buffers.
		virtual size_t onHlmsTypeChanged(bool casterPass, CommandBuffer* pCommandBuffer, const QueuedRenderable& queuedRenderable);

		/// Called from public fillBuffersForV1 to fill the buffers for the queued renderable.
		virtual uint32 fillBuffersForV1(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, CommandBuffer* pCommandBuffer) = 0;

		/// Called from public fillBuffersForV2 to fill the buffers for the queued renderable.
		virtual uint32 fillBuffersForV2(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, CommandBuffer* pCommandBuffer) = 0;

	protected:
        float mConstantBiasScale;

	private:
		/// Buffer pool to store the properties from datablocks. It is used only if requested by implementation. It is necessary that
		/// the implementation creates datablocks inherited from ConstBufferPoolUser.
		HlmsMaterialBufferPool mMaterialBufferPool;

		/// Buffer pools used by shaders to store additional data to render the objects.
		vector<HlmsBufferPool*>::type mBufferPools;

		size_t mReadOnlyBufferPoolsCount; ///< cached due to HlmsListener::hlmsTypeChanged

		/// Header files which can be included in the shaders using @insertpiece
		PiecesMap m_HeaderFiles;
	};

	/// Helper function to initialize DescBindingRange directly as it doesn't have a constructor accepting (start, end) and
	/// the default constructor is not exported from the OGRE.
	inline void setDescBindingRange(DescBindingRange& descBindingRange, uint16 startSlot, uint16 endSlot)
	{
		descBindingRange.start = startSlot;
		descBindingRange.end = endSlot;
	}

	/// Helper function to convert float value to uint32_t which can be accessed in shader using uintBitsToFloat.
	inline uint32_t FloatBitsToUint(float value)
	{
		return *reinterpret_cast<uint32_t *>(&value);
	}

    /// Helper function to convert uint32_t value to float which can be accessed in shader using floatBitsToUint.
    inline float UintBitsToFloat(uint32_t value)
	{
		return *reinterpret_cast<float*>(&value);
	}

	/// Helper template to implement registration of the HLMS.
	/// The code is copied from GraphicsSystem::registerHlms().
	template<typename HlmsT>
	void registerHlms(const String& resourcePath, const Ogre::String& archiveType)
	{
		ConfigFile cf;
		cf.load( resourcePath + "resources2.cfg" );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
		String rootHlmsFolder =
			macBundlePath() + '/' + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
		String rootHlmsFolder = resourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif

		if( rootHlmsFolder.empty() )
			rootHlmsFolder = "./";
		else if( *( rootHlmsFolder.end() - 1 ) != '/' )
			rootHlmsFolder += "/";

		// At this point rootHlmsFolder should be a valid path to the Hlms data folder.
		// For retrieval of the paths to the different folders needed.
		String mainFolderPath;
		StringVector libraryFoldersPaths;
		StringVector::const_iterator libraryFolderPathIt;
		StringVector::const_iterator libraryFolderPathEn;

		ArchiveManager& archiveManager = ArchiveManager::getSingleton();

		// Create & Register HlmsT
		// Get the path to all the subdirectories used by HlmsT
		HlmsT::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
		Archive* pRootArchive = archiveManager.load(rootHlmsFolder + mainFolderPath, archiveType, true);
		ArchiveVec archiveLibraryFolders;
		libraryFolderPathIt = libraryFoldersPaths.begin();
		libraryFolderPathEn = libraryFoldersPaths.end();
		while( libraryFolderPathIt != libraryFolderPathEn )
		{
			Archive *archiveLibrary = archiveManager.load(rootHlmsFolder + *libraryFolderPathIt, archiveType, true);
			archiveLibraryFolders.push_back( archiveLibrary );
			++libraryFolderPathIt;
		}

		// Create and register the HlmsT.
		HlmsT* pHlms = OGRE_NEW HlmsT(pRootArchive, &archiveLibraryFolders);
		Root::getSingleton().getHlmsManager()->registerHlms(pHlms);
	}
} // namespace Ogre
