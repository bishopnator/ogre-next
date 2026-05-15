#pragma once
#include "OgreHlmsExtExport.h"

// OGRE
#include <OgreResourceTransition.h>
#include <OgreStd/vector.h>

// forwards
namespace Ogre
{
	class HlmsBufferHandler;
	using HlmsBufferHandlerPtr = std::unique_ptr<HlmsBufferHandler>;
}

namespace Ogre
{
	/// Base interface for the buffer pools used by HlmsExt.
	class _OgreHlmsExtExport HlmsBufferPoolInterface : public OgreAllocatedObj
	{
	public:
		explicit HlmsBufferPoolInterface(size_t defaultBufferSize, HlmsBufferHandlerPtr pBufferHandler);
		virtual ~HlmsBufferPoolInterface();

		/// not copyable
		HlmsBufferPoolInterface(const HlmsBufferPoolInterface&) = delete;
		HlmsBufferPoolInterface& operator=(const HlmsBufferPoolInterface&) = delete;

		/// Set binding for the buffers in the shaders.
		/// @param stages	Combination of ShaderType values.
		/// @param slot		Slot index to which the buffers will be bound.
		/// @throw Exception if a buffer is currently mapped.
		void setBinding(uint8_t stages, uint16_t slot);

		/// Set new render system to be used with the manager.
		void setRenderSystem(Ogre::RenderSystem* pRenderSystem);

		/// Destroys all the created buffers.
		virtual void destroyAllBuffers() = 0;

		/// Analyze the needed barriers according to the currently executed scene pass.
		virtual void analyzeBarriers(BarrierSolver& barrierSolver, ResourceTransitionArray& resourceTransitions, Camera& renderingCamera) = 0;

		/// This gets called right before executing the command buffer.
		virtual void preCommandBufferExecution(CommandBuffer* pCommandBuffer) = 0;

		/// This gets called after executing the command buffer.
		virtual void postCommandBufferExecution(CommandBuffer* pCommandBuffer) = 0;

		/// Called when the frame has fully ended (ALL passes have been executed to all RTTs).
		virtual void frameEnded() = 0;

	protected:
		virtual void onSetBinding(uint8_t stages, uint16_t slot) = 0;
		virtual void onSetVaoManager() = 0;

		BufferPacked* createBuffer(size_t bufferSize);
		void destroyBuffer(BufferPacked* pBuffer);
		size_t getBufferAlignment() const;
		size_t getMaxBufferSize() const;
		void bindBuffer(CommandBuffer& commandBuffer, BufferPacked& buffer, size_t bindOffset);

	protected:
		/// Buffer handler to handle underlying buffers.
		HlmsBufferHandlerPtr mBufferHandler;

		/// VertexArrayObject manager which is used by buffer handler.
		VaoManager* mVaoManager;

		/// Binding of the buffer.
		uint8_t mStages;
		uint16_t mSlot;

		/// Default buffer size used for newly created buffers.
		const size_t mDefaultBufferSize;
	};
} // namespace Ogre
