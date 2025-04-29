#pragma once
#include "OgreHlmsBatchData.h"
#include "OgreHlmsBufferPoolInterface.h"

// std
#include <unordered_set>

namespace Ogre
{
	/// Base class for batched data pools. It creates and maintain layouts of the created buffers. Data from multiple meshes can be stored in a single buffer
	/// which reduces bindings. The pool has a predefined capacity (constructor) which allows to batch multiple data together. It however allows to store even
	/// larger data if needed. In this cases a special buffer with desired capacity is created.
	class _OgreHlmsExtExport HlmsBatchDataPoolInterface : public HlmsBufferPoolInterface
	{
	public:
		explicit HlmsBatchDataPoolInterface(size_t defaultBufferSize, HlmsBufferHandlerPtr pBufferHandler);
		~HlmsBatchDataPoolInterface() override;

		/// Bind the buffer to the shaders.
		/// @return true if new buffer was bound.
		bool bindBuffer(HlmsBatchDataSlotId id, CommandBuffer* pCommandBuffer);

		/// Get the data offset within the buffer where the slot is stored.
		size_t getDataOffset(HlmsBatchDataSlotId id) const;

	public: // HlmsBufferPoolInterface implementation
		void onSetBinding(uint8_t stages, uint16_t slot) override;
		void onSetVaoManager() override;
		void destroyAllBuffers() override;
		void analyzeBarriers(BarrierSolver& barrierSolver, ResourceTransitionArray& resourceTransitions, Camera& renderingCamera) override;
		void preCommandBufferExecution(CommandBuffer* pCommandBuffer) override;
		void postCommandBufferExecution(CommandBuffer* pCommandBuffer) override;
		void frameEnded() override final;

	protected:	
		/// Create a slot with requested size.
		/// note: The implementation must provide helper function which calls protected addSlot.
		BatchDataSlotInfo addSlot(size_t numBytes);

		/// Remove slot from the pool.
		/// note: The implementation must provide helper function which calls protected removeSlot.
		void removeSlot(HlmsBatchDataSlotId id);

		/// Remove all slots from the pool.
		/// note: The implementation must provide helper function which calls protected removeAll.
		void removeAll();

		using HlmsBufferPoolInterface::bindBuffer;
		HlmsBatchDataSlotId createId();
		BatchDataSlot* findSlot(HlmsBatchDataSlotId id);

	private:
		/// Allows the implementation to customize the BatchDataBuffer by adding some additional data.
		virtual BatchDataBuffer* createBatchDataBuffer(size_t numBytes) = 0;
		virtual void deleteBatchDataBuffer(BatchDataBuffer* pBatchDataBuffer) = 0;

		/// Notifications.
		virtual void onBatchDataBufferPrepared(BatchDataBuffer& batchDataBuffer) = 0;
		virtual void onBatchDataBufferUpdated(BatchDataBuffer& batchDataBuffer) = 0;
		virtual bool preBindBatchDataBuffer(BatchDataBuffer& batchDataBuffer) = 0;

	private:
		std::unordered_set<BatchDataBuffer*> mBuffers;
		std::unordered_map<HlmsBatchDataSlotId, BatchDataSlot> mSlots;

		/// Last bound buffer to avoid useless rebinding.
		BatchDataBuffer* mLastBoundBuffer;

		/// Next available ID.
		HlmsBatchDataSlotId mNextId;

		/// List of IDs from removed slots.
		std::vector<HlmsBatchDataSlotId> mUnusedIds;
	};
} // namespace Ogre
