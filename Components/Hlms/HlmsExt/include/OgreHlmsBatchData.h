#pragma once

// STL
#include <limits>
#include <unordered_set>

// forwards
namespace Ogre
{
	class BufferPacked;
}

namespace Ogre
{
	/// Slot's id to identify a slot within the HlmsBatchDataPool.
	using HlmsBatchDataSlotId = size_t;

	/// Slot id to indicate that it is not allocated/assigned by HlmsBatchDataPool.
	constexpr HlmsBatchDataSlotId cInvalidBatchDataSlotId = std::numeric_limits<size_t>::max();

	/// Created buffer with slots.
	struct BatchDataBuffer
	{
		BufferPacked* mBuffer = nullptr;

		/// Slots added to the buffer identified by the IDs.
		std::unordered_set<HlmsBatchDataSlotId> mSlotIds;

		/// Number of used bytes from the buffer.
		size_t mUsed = 0;
	};

	/// Slot within a buffer.
	struct BatchDataSlot
	{
		/// The owner of this slot.
		BatchDataBuffer* mOwner = nullptr;

		/// Offset of the slot from the beginning of the buffer.
		size_t mOffset = 0;

		/// Size of the data used by the slot.
		size_t mSize = 0;
	};

	/// Slot within a buffer with its ID. It is used as returned value from some functions.
	struct BatchDataSlotInfo
	{
		HlmsBatchDataSlotId mId = cInvalidBatchDataSlotId;
		BatchDataSlot mSlot;
	};
} // namespace Ogre
