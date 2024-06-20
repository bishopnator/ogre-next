#pragma once
#include "OgreComponentExport.h"

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
	/// To make full implementation of Hlms using this class, it is necessary to implement the following pure virtual methods:
	/// * setupDescBindingRange
	/// * fillBuffersForV1
	/// * fillBuffersForV2
	/// * createDatablockImpl
	class _OgreComponentExport HlmsExt : public Hlms
	{
	public:
		HlmsExt(HlmsTypes type, const String& typeName, Archive* dataFolder, ArchiveVec* pLibraryFolders);
		~HlmsExt() override;

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

	private:
		vector<HlmsBufferPool*>::type mBufferPools;
		size_t mReadOnlyBufferPoolsCount = 0; ///< cached due to HlmsListener::hlmsTypeChanged
	};

	/// Helper function to initialize DescBindingRange directly as it doesn't have a constructor accepting (start, end) and
	/// the default constructor is not exported from the OGRE.
	inline void setDescBindingRange(DescBindingRange& descBindingRange, uint16 startSlot, uint16 endSlot)
	{
		descBindingRange.start = startSlot;
		descBindingRange.end = endSlot;
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
        Ogre::String mainFolderPath;
        Ogre::StringVector libraryFoldersPaths;
        Ogre::StringVector::const_iterator libraryFolderPathIt;
        Ogre::StringVector::const_iterator libraryFolderPathEn;

        Ogre::ArchiveManager& archiveManager = Ogre::ArchiveManager::getSingleton();

		// Create & Register HlmsT
		// Get the path to all the subdirectories used by HlmsT
		HlmsT::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
		Archive *archiveUnlit = archiveManager.load( rootHlmsFolder + mainFolderPath, archiveType, true );
		ArchiveVec archiveLibraryFolders;
		libraryFolderPathIt = libraryFoldersPaths.begin();
		libraryFolderPathEn = libraryFoldersPaths.end();
		while( libraryFolderPathIt != libraryFolderPathEn )
		{
			Archive *archiveLibrary = archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, archiveType, true );
			archiveLibraryFolders.push_back( archiveLibrary );
			++libraryFolderPathIt;
		}

		// Create and register the HlmsT.
		HlmsT* pHlms = OGRE_NEW HlmsT( archiveUnlit, &archiveLibraryFolders);
		Root::getSingleton().getHlmsManager()->registerHlms( pHlms );
	}
} // namespace Ogre
