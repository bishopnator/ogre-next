#pragma once
#include "OgreHlmsMaterialBufferPool.h"

// OGRE
#include <OgreDescriptorSetUav.h>
#include <OgreHlms.h>
#include <OgreResourceTransition.h>
#include <OgreRootLayout.h>
#include <OgreStd/vector.h>

// forwards
namespace Ogre
{
	struct DescBindingRange;
	class HlmsBatchDataPool;
	class HlmsBufferHandler;
	using HlmsBufferHandlerPtr = std::unique_ptr<HlmsBufferHandler>;
	class HlmsBufferPool;
	class HlmsBufferPoolInterface;
	class HlmsUavBufferPool;
}

namespace Ogre
{
	/// Base implementation of Hlms which uses HlmsBufferPool management.
	/// To make full implementation of Hlms using this class, it is necessary to implement the following virtual methods:
	/// * setupDescBindingRange
	/// * preparePassHash
	/// * registerRenderableTextures
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
	/// The registerRenderableTextures is for notifying the Ogre about texture bindings based on the properties. The implementation must be in sync with binding textures within fillBuffersForV1/fillBuffersForV2 implementation.
	/// 
	/// The setupDescBindingRange reflects the bindings setup in the constructor.
	/// 
	/// The fillBuffersForV1/fillBuffersForV2 fills the per instance data in the buffers.
	/// 
	/// The createDatablockImpl creates the custom datablock used by the hlms. The datablock contains parameters used by instances like
	/// colors, textures, transparency, etc. This is the closest to the "material". Multiple datablocks are stored in the same buffer so
	/// the multiple instances can be rendered together.
	class _OgreHlmsExtExport HlmsExt : public Hlms
	{
	public:
		using ResourceAccessMap = unordered_map<uint32_t, ResourceAccess::ResourceAccess>::type;

		struct TextureBinding
		{
			ShaderType mShaderType = PixelShader; ///< Shader type where the texture will be bound.
			IdString mName; ///< Name of the texture variable within the shader.
			TextureGpu* mTexture = nullptr; ///< Texture itself to bind.
			const HlmsSamplerblock* mSamplerBlock = nullptr; ///< Texture sample to bind.

			bool operator==(const TextureBinding& textureBinding) const
			{
				return mShaderType == textureBinding.mShaderType && mName == textureBinding.mName && mTexture == textureBinding.mTexture && mSamplerBlock == textureBinding.mSamplerBlock;
			}
		};
		using TextureBindings = vector<TextureBinding>::type;

	public:
		HlmsExt(HlmsTypes type, const String& typeName, Archive* dataFolder, ArchiveVec* pLibraryFolders);
		~HlmsExt() override;

		/// Setup the parameters for the primary buffers which hold the data from HlmsDatablocks. The primary buffer is always ConstBufferPacked.
		/// @see HlmsMaterialBufferPool::setPrimaryBufferParams
		void setDatablocksPrimaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize);

		/// Setup the parameters for the secondary buffers which hold the data from HlmsDatablocks. The secondary buffer is either ConstBufferPacked or ReadOnlyBufferPacked.
		/// @see HlmsMaterialBufferPool::setSecondaryBufferParams
		void setDatablocksSecondaryBufferParams(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t datablockSize, BufferType bufferType = BT_DEFAULT, bool useReadOnlyBuffers = true);

		/// Get capacity of datablocks in the primary buffer. Valid only after setDatablocksPrimaryBufferParams call and setting valid RenderSystem.
		size_t getDatablocksBufferCapacity() const;

		/// Request a slot for the datablock (only if it inherits from ConstBufferPoolUser). This should be called by the datablock constructor.
		void requestDatablockSlot(ConstBufferPoolUser& datablock, uint32_t hash, bool useSecondaryBuffer);

		/// Release a slot from the datablock (only if it inherits from ConstBufferPoolUser). This should be called by the datablock destructor.
		void releaseDatablockSlot(ConstBufferPoolUser& datablock);

		/// Notify the hlms that the datablock has been changed. It will schedule the update when preparePassHash is called.
		void invalidateDatablockSlot(ConstBufferPoolUser& datablock);

		/// Create a buffer pool with the requested bindings.
		/// @param stages			Combination of ShaderType values.
		/// @param slot				Slot index to which the buffers will be bound.
		/// @param bufferSize		Size of the created buffers within the buffer pool.
		/// @param pBufferHandler	Buffer handler to handler creation/deletion/binding of the buffers.
		HlmsBufferPool& createBufferPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize, HlmsBufferHandlerPtr pBufferHandler);

		/// Create a UAV pool with the requested bindings.
		/// @param stages			Combination of ShaderType values where the buffer will be bound for reading.
		/// @param writeSlot		Slot index to which the buffers will be bound in the write mode (u0, u1, u2, ..., but it is relative number to number of currently bound color buffers --> 0 means just after last color buffer).
		/// @param readSlot			Slot index to which the buffers will be bound in the read mode (t0, t1, t2, ...).
		/// @param elementSize		Size of the element stored in a buffer.
		/// @param numElements		Number of elements which will fit by default in the buffer.
		/// @param resourceAccessMap Defines the access of the UAVs per compositor pass identified by its identifier. @see CompositorPassDef::mIdentifier.
		HlmsUavBufferPool& createUavBufferPool(const std::initializer_list<ShaderType>& stages, uint16_t writeSlot, uint16_t readSlot, size_t elementSize, size_t numElements, const ResourceAccessMap& resourceAccessMap);

		/// Create a buffer pool for data batching. The buffers are not automatically bound to the shaders like HlmsBufferPool, but rather the implementation of HlmsExt
		/// is responsible to bind it correctly. The pool provides the support for storing data which are registered under BatchDataSlotId. If e.g. the data are extracted from
		/// Renderable, it is up to the implementation to access corresponding BatchDataSlotId for the Renderable and bind a buffer which contains those data. The batched data
		/// ensures that there will be a limited amount of bindings.
		/// @param stages			Combination of ShaderType values.
		/// @param slot				Slot index to which the buffers will be bound.
		/// @param bufferSize		Size of the created buffers within the buffer pool.
		/// @param pBufferHandler	Buffer handler to handler creation/deletion/binding of the buffers.
		HlmsBatchDataPool& createBatchDataPool(const std::initializer_list<ShaderType>& stages, uint16_t slot, size_t bufferSize, HlmsBufferHandlerPtr pBufferHandler);

	public: // Hlms interface
		void _changeRenderSystem(RenderSystem* pNewRenderSystem) override;
		void preCommandBufferExecution(CommandBuffer* pCommandBuffer) override;
		void postCommandBufferExecution(CommandBuffer* pCommandBuffer) override;
		void frameEnded(void) override;
		void calculateHashForPreCreate(Renderable* pRenderable, PiecesMap* pInOutPieces) override;
		void calculateHashForPreCaster(Renderable* pRenderable, PiecesMap* pInOutPieces, const PiecesMap* pNormalPassPieces) override;
		HlmsCache preparePassHash(const CompositorShadowNode* pShadowNode, bool casterPass, bool dualParaboloid, SceneManager* pSceneManager) override final;
		uint32 fillBuffersFor(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash) override;
		uint32 fillBuffersForV1(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* pCommandBuffer) override;
		uint32 fillBuffersForV2(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* pCommandBuffer) override;
		void analyzeBarriers(BarrierSolver& barrierSolver, ResourceTransitionArray& resourceTransitions, Camera* renderingCamera, const bool bCasterPass) override;

	protected: // Hlms interface
		/// Allow to convert source code if needed to the supported shader language.
		HighLevelGpuProgramPtr compileShaderCode(const String& source, const String& debugFilenameOutput, uint32 finalHash, ShaderType shaderType, size_t tid) override;

		void setupRootLayout(RootLayout& rootLayout, size_t tid) override;

		/// Called from setupRootLayout to initialize the non-baked DescBindingRange to tell OGRE which slots are used. The non-baked ranges cannot contain UavBuffer and UabTexture.
		/// To access a DescBindingRange, use descBindingRanges[DescBindingTypes::xxx]
		virtual void setupDescBindingRangesNonBaked(DescBindingRange* descBindingRanges) = 0;

		/// Called from setupRootLayout to initialize the baked DescBindingRange to tell OGRE which slots are used. The baked ranges cannot contain ParamBuffer and ConstBuffer slots.
		/// To access a DescBindingRange, use descBindingRanges[DescBindingTypes::xxx]
		virtual void setupDescBindingRangesBaked(DescBindingRange* descBindingRanges) = 0;

	protected:
		/// Do automatic detection of shader files based on the files stored on the main folder.
		bool autodetectShaderFiles();

		/// Called from preparePassHash which allows to do further pass initialization.
		virtual void onPreparePassHash(const CompositorShadowNode* pShadowNode, bool casterPass, bool dualParaboloid, SceneManager* pSceneManager);

		/// Called from fillBuffersForV1/fillBuffersForV2 when Hlms types was changed since last renderable.
		/// @return Number of used texture buffers.
		virtual size_t onHlmsTypeChanged(bool casterPass, CommandBuffer* pCommandBuffer, const QueuedRenderable& queuedRenderable);

		/// Called from public fillBuffersForV1 to fill the buffers for the queued renderable.
		virtual uint32 fillBuffersForV1(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, CommandBuffer* pCommandBuffer) = 0;

		/// Called from public fillBuffersForV2 to fill the buffers for the queued renderable.
		virtual uint32 fillBuffersForV2(const HlmsCache* pCache, const QueuedRenderable& queuedRenderable, bool casterPass, CommandBuffer* pCommandBuffer) = 0;

		/// Add a pass texture. The pass textures are global textures used by all renderables withing the currently rendered pass. It should be called from onPreparePassHash.
		/// It sets the property with the value which corresponds to the binding slot. It is possible to declare then the binding in the templates as t@value(name).
		void addPassTexture(ShaderType shaderType, const IdString& name, TextureGpu& texture, const HlmsSamplerblock& samplerBlock);

		/// The renderable textures must be added from 2 calls which must be in sync.
		/// * Calling addRenderableTexture(shaderType, name) from the calculateHashForPreCreate/calculateHashForPreCaster which registers the texture with proper slot and set the property with the value which corresponds to the binding slot.
		/// * Calling addRenderableTexture(shaderType, name, texture, sampler, pCommandBuffer) from the fillBuffersForV1/fillBuffersForV2 which issues the binding command.
		/// 
		/// If the above calls are not in sync, the CbTexture command will be called with wrong slot opposed to the value stored in the property.
		void addRenderableTexture(ShaderType shaderType, const IdString& name);
		void addRenderableTexture(ShaderType shaderType, const IdString& name, TextureGpu& texture, const HlmsSamplerblock& samplerBlock, CommandBuffer* pCommandBuffer);

		/// Invalidate texture binding at given slot. This must be called if direct texture binding through mRenderSystem is called e.g. from fillBuffersFor.
		void invalidateTextureBinding(uint16_t slot);

	private:
		/// Check whether the binding is needed. If the requested texture is not currently bound at the specified slot, binds it and updates the mActiveTextures.
		void bindTexture(uint16_t slot, const TextureBinding& textureBinding, CommandBuffer* pCommandBuffer);

		/// Bind the pass textures if needed.
		void bindPassTextures(CommandBuffer* pCommandBuffer);

		/// Bind UAV buffers.
		void bindUavBuffers(CommandBuffer* pCommandBuffer);

	protected:
        float mConstantBiasScale;

	private:
		/// Buffer pool to store the properties from datablocks. It is used only if requested by implementation. It is necessary that
		/// the implementation creates datablocks inherited from ConstBufferPoolUser.
		HlmsMaterialBufferPool mMaterialBufferPool;

		/// Buffer pools used by shaders to store additional data to render the objects.
		vector<HlmsBufferPoolInterface*>::type mBufferPools;

		/// Header files which can be included in the shaders using @insertpiece
		PiecesMap mHeaderFiles;

		/// Set of bound UAVs. This tracks the bindings initiated through pools created by createUavBufferPool.
		/// The mDescriptorSetUav.mRefCount indicates whether the UAVs are bound in CommandBuffer. This is the hack
		/// to avoid addition of the new member.
		DescriptorSetUav mDescriptorSetUav;

		/// Reserved the texture buffer slots. This is updated when a texbuffer or read-only buffer is created. Usually done in the constructor of the inherited class from HlmsExt.
		/// It includes also read-only buffers and uav buffers which are bound to the same binding slots.
		uint8_t mReservedTexBufferSlots;
		
		/// Counter for the textures used by renderables.
		uint8_t mRenderableTexturesCounter;

		/// Track the bound textures to avoid useless binding of a texture which is already bound.
		TextureBindings mActiveTextures;

		/// Pass textures. The binding begins at mReservedTexBufferSlots.
		TextureBindings mPassTextures;
		bool mPassTexturesDirty; ///< True if the pass textures are changed and must be rebound.
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

	/// Get root HLMS folder from the configuration file.
	_OgreHlmsExtExport String getHlmsRootFolder(const String& resourcePath, const String& configFileName = "resources2.cfg", const String& key = "DoNotUseAsResource", const String& section = "Hlms");

	/// Helper template to implement registration of the HLMS.
	/// The code is copied from GraphicsSystem::registerHlms().
	template<typename HlmsT>
	HlmsT* registerHlms(String rootHlmsFolder, const Ogre::String& archiveType)
	{
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
		return pHlms;
	}
} // namespace Ogre
