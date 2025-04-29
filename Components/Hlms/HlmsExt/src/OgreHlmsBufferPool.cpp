#include "OgreHlmsBufferPool.h"
#include "OgreHlmsBufferHandler.h"

// OGRE
#include <OgreCamera.h>
#include <OgreRenderSystem.h>
#include <CommandBuffer/OgreCbShaderBuffer.h>
#include <CommandBuffer/OgreCommandBuffer.h>
#include <Vao/OgreBufferPacked.h>
#include <Vao/OgreVaoManager.h>

// std
#include <array>

using namespace Ogre;

namespace
{
	//////////////////////////////////////////////////////////////////////////
	/// @return CommandBuffer::COMMAND_FIXED_SIZE, but the value is private to the class.
	size_t getCommandSize()
	{
		Ogre::CommandBuffer commandBuffer;
		commandBuffer.addCommand<Ogre::CbBase>();
		commandBuffer.addCommand<Ogre::CbBase>();
		return commandBuffer.getCommandOffset(commandBuffer.getLastCommand());
	}

	//////////////////////////////////////////////////////////////////////////
	const size_t cCommandFixedSize = getCommandSize();

	//////////////////////////////////////////////////////////////////////////
	constexpr size_t cInvalidCommandOffset = (size_t)~0;
}

//////////////////////////////////////////////////////////////////////////
HlmsBufferPool::HlmsBufferPool(MapMode mapMode, size_t defaultBufferSize, HlmsBufferHandlerPtr pBufferHandler)
: HlmsBufferPoolInterface(defaultBufferSize, std::move(pBufferHandler))
, mMapMode(mapMode)
, mBufferSize(0)
, mStartPtr(nullptr)
, mCurrentPtr(nullptr)
, mCapacity(0)
, mMappedPtr(0)
, mMappedOffset(0)
, mCurrentIndex(0)
, mShaderBufferCommandOffset(cInvalidCommandOffset)
, mBindOffset(-1)
, mBindBufferIndex(-1)
{
}

//////////////////////////////////////////////////////////////////////////
HlmsBufferPool::~HlmsBufferPool()
{
	destroyAllBuffers();
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::onSetBinding(uint8_t /*stages*/, uint16_t /*slot*/)
{
	if (mStartPtr != nullptr)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot change binding in the middle of rendering.", "HlmsBufferPool::onSetBinding");
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::onSetVaoManager()
{
	// Update buffer size for newly created buffers.
	mBufferSize = alignToNextMultiple(mDefaultBufferSize, getBufferAlignment());
	mBufferSize = std::min(mBufferSize, getMaxBufferSize());
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::bindBuffer(CommandBuffer* pCommandBuffer)
{
	if (pCommandBuffer != nullptr && mBindOffset != size_t(-1) && mBindBufferIndex < mBuffers.size())
	{
		bindBuffer(*pCommandBuffer, *mBuffers[mBindBufferIndex], static_cast<uint32_t>(mBindOffset));
		mShaderBufferCommandOffset = pCommandBuffer->getCommandOffset(pCommandBuffer->getLastCommand());
	}
	else if (mCurrentIndex < mBuffers.size())
	{
		const auto bindOffset = (mStartPtr - mMappedPtr) * sizeof(uint8_t);

		if (pCommandBuffer == nullptr)
		{
			// save values for later binding
			mBindOffset = mMappedOffset + bindOffset;
			mBindBufferIndex = mCurrentIndex;
			return;
		}

		bindBuffer(*pCommandBuffer, *mBuffers[mCurrentIndex], static_cast<uint32_t>(mMappedOffset + bindOffset));
		mShaderBufferCommandOffset = pCommandBuffer->getCommandOffset(pCommandBuffer->getLastCommand());
	}

	mBindOffset = size_t(-1);
	mBindBufferIndex = size_t(-1);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::destroyAllBuffers()
{
	auto itor = mBuffers.begin();
	auto end = mBuffers.end();

	while (itor != end)
	{
		if ((*itor)->getMappingState() != Ogre::MS_UNMAPPED)
			(*itor)->unmap(Ogre::UO_UNMAP_ALL);

		destroyBuffer(*itor);
		++itor;
	}

	mBuffers.clear();

	mStartPtr = nullptr;
	mCurrentPtr = nullptr;
	mCapacity = 0;
	mMappedPtr = nullptr;
	mMappedOffset = 0;
	mCurrentIndex = 0;
	mShaderBufferCommandOffset = cInvalidCommandOffset;
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::mapBuffer(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes)
{
	if (mMapMode == MapMode::eBulk)
		mapBufferBulk(pCommandBuffer, minimumSizeBytes);
	else
		mapBufferSubRange(pCommandBuffer, minimumSizeBytes);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::unmapBuffer(CommandBuffer* pCommandBuffer)
{
	if (mMapMode == MapMode::eBulk)
		unmapBufferBulk();
	else
		unmapBufferSubRange(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::mapBufferBulk(CommandBuffer* pCommandBuffer, size_t /*minimumSizeBytes*/)
{
	// Ensure that currently mapped const buffer is not mapped anymore.
	unmapBufferBulk();

	if (mStages == 0)
		OGRE_EXCEPT(Exception::ERR_INVALID_STATE, "SetBinding was not called.", "HlmsBufferPool::MapNextBuffer");

	if (mCurrentIndex >= mBuffers.size())
	{
		auto* pNewBuffer = createBuffer(mBufferSize);
		mBuffers.push_back(pNewBuffer);
	}

	auto* pBuffer = mBuffers[mCurrentIndex];

	mMappedOffset = 0;
	mMappedPtr = reinterpret_cast<uint8_t*>(pBuffer->map(mMappedOffset, pBuffer->getTotalSizeBytes() - mMappedOffset));
	mStartPtr = mCurrentPtr = mMappedPtr;
	mCapacity = pBuffer->getTotalSizeBytes();

	bindBuffer(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::unmapBufferBulk()
{
	if (mStartPtr != nullptr)
	{
		const auto writtenSize = mCurrentPtr - mStartPtr;

		// Unmap the current buffer.
		auto* pBuffer = mBuffers[mCurrentIndex];
		pBuffer->unmap(UO_KEEP_PERSISTENT, mMappedOffset, writtenSize);

		++mCurrentIndex;

		mMappedPtr = nullptr;
		mStartPtr = nullptr;
		mCurrentPtr = nullptr;
		mCapacity = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::mapBufferSubRange(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes)
{
	updateShaderBufferCommand(pCommandBuffer);

	size_t currentOffset = (mCurrentPtr - mStartPtr) * sizeof(uint8_t);
	currentOffset = alignToNextMultiple(currentOffset, getBufferAlignment());
	currentOffset = std::min(mCapacity, currentOffset);
	const size_t remainingSize = mCapacity - currentOffset;
	if (remainingSize < minimumSizeBytes)
	{
		mapNextBufferSubRange(pCommandBuffer, minimumSizeBytes);
	}
	else
	{
		size_t bindOffset = (mStartPtr - mMappedPtr) * sizeof(uint8_t);
		mStartPtr += currentOffset;
		mCurrentPtr = mStartPtr;
		mCapacity -= currentOffset;

		bindOffset = (mCurrentPtr - mMappedPtr) * sizeof(uint8_t);

		if (mMappedOffset + bindOffset >= mBuffers[mCurrentIndex]->getTotalSizeBytes())
		{
			mapNextBufferSubRange(pCommandBuffer, minimumSizeBytes);
		}
		else
		{
			// Add a new binding command.
			bindBuffer(pCommandBuffer);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::mapNextBufferSubRange(CommandBuffer* pCommandBuffer, size_t minimumSizeBytes)
{
	unmapBufferSubRange(pCommandBuffer);

	auto* pBuffer = mCurrentIndex < mBuffers.size() ? mBuffers[mCurrentIndex] : nullptr;
	auto bufferSize = pBuffer != nullptr ? pBuffer->getTotalSizeBytes() : 0;

	mMappedOffset = alignToNextMultiple(mMappedOffset, getBufferAlignment());

	// We'll go out of bounds. This buffer is full. Get a new one and remap from 0.
	if (pBuffer == nullptr || mMappedOffset + minimumSizeBytes >= bufferSize)
	{
		mMappedOffset = 0;

		if (pBuffer != nullptr)
			++mCurrentIndex; // Use next buffer.

		if (mCurrentIndex >= mBuffers.size())
		{
			auto* newBuffer = createBuffer(mBufferSize);
			mBuffers.push_back(newBuffer);
		}

		pBuffer = mBuffers[mCurrentIndex];
	}

	mMappedPtr = reinterpret_cast<uint8_t*>(pBuffer->map(mMappedOffset, pBuffer->getTotalSizeBytes() - mMappedOffset, false));
	mStartPtr = mCurrentPtr = mMappedPtr;
	mCapacity = pBuffer->getTotalSizeBytes() - mMappedOffset;

	bindBuffer(pCommandBuffer);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::unmapBufferSubRange(CommandBuffer* pCommandBuffer)
{
	// Save our progress.
	const size_t bytesWritten = (mCurrentPtr - mMappedPtr) * sizeof(uint8_t);
	mMappedOffset += bytesWritten;

	if (mMappedPtr != nullptr)
	{
		// Unmap the current buffer.
		auto* pBuffer = mBuffers[mCurrentIndex];
		pBuffer->unmap(UO_KEEP_PERSISTENT, 0, bytesWritten);
		updateShaderBufferCommand(pCommandBuffer);
	}

	mMappedPtr = nullptr;
	mStartPtr = nullptr;
	mCurrentPtr = nullptr;
	mCapacity = 0;

	// Ensure the proper alignment.
	mMappedOffset = alignToNextMultiple(mMappedOffset, getBufferAlignment());
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::writeData(const void* pData, size_t numBytes)
{
	if (mStartPtr == nullptr)
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Buffer is not mapped.", "HlmsBufferPool::writeData");

	const size_t written = (mCurrentPtr - mStartPtr) * sizeof(uint8_t);
	if (written + numBytes > mCapacity)
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Buffer overflow.", "HlmsBufferPool::writeData");

	memcpy(mCurrentPtr, pData, numBytes);
	mCurrentPtr += numBytes;
}

//////////////////////////////////////////////////////////////////////////
bool HlmsBufferPool::canWrite(size_t numBytes) const
{
	// Note that if buffer is not mapped, both mCurrentPtr and mStartPtr are nullptr and mCapacity is 0. In this case for any numBytes > 0, the return value is false.
	const size_t written = (mCurrentPtr - mStartPtr) * sizeof(uint8_t);
	return written + numBytes <= mCapacity;
}

//////////////////////////////////////////////////////////////////////////
size_t HlmsBufferPool::getWrittenSize() const
{
	return (mCurrentPtr - mStartPtr) * sizeof(uint8_t);
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::analyzeBarriers(BarrierSolver& barrierSolver, ResourceTransitionArray& resourceTransitions, Camera& renderingCamera)
{
	const auto resourceAccess = mBufferHandler->analyzeBarriers(*renderingCamera.getSceneManager());
	if (resourceAccess != std::nullopt)
	{
		const auto value = resourceAccess.value();
		for (auto* pBuffer : mBuffers)
			barrierSolver.resolveTransition(resourceTransitions, pBuffer, value, mStages);
	}
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::preCommandBufferExecution(CommandBuffer* pCommandBuffer)
{
	unmapBuffer(pCommandBuffer);

	if (mMapMode == MapMode::eSubRange)
	{
		for (auto* pBuffer : mBuffers)
			pBuffer->advanceFrame();
	}
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::postCommandBufferExecution(CommandBuffer* pCommandBuffer)
{
	if (mMapMode == MapMode::eSubRange)
	{
		for (auto* pBuffer : mBuffers)
			pBuffer->regressFrame();
	}
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::frameEnded()
{
	mCurrentIndex = 0;
	mMappedOffset = 0;

	if (mMapMode == MapMode::eSubRange)
	{
		for (auto* pBuffer : mBuffers)
			pBuffer->advanceFrame();
	}
}

//////////////////////////////////////////////////////////////////////////
void HlmsBufferPool::updateShaderBufferCommand(CommandBuffer* pCommandBuffer)
{
	if (mShaderBufferCommandOffset != cInvalidCommandOffset)
	{
		static_assert(NumShaderTypes == 5, "ShaderType enum has been changed! Update the implementation.");
		constexpr std::array<uint8_t, 32> cNumBits = {
			0, //  0 = b00000 = 0 bits
			1, //  1 = b00001 = 1 bit
			1, //  2 = b00010 = 1 bit
			2, //  3 = b00011 = 2 bits
			1, //  4 = b00100 = 1 bit
			2, //  5 = b00101 = 2 bits
			2, //  6 = b00110 = 2 bits
			3, //  7 = b00111 = 3 bits
			1, //  8 = b01000 = 1 bit
			2, //  9 = b01001 = 2 bits
			2, // 10 = b01010 = 2 bits
			3, // 11 = b01011 = 3 bits
			2, // 12 = b01100 = 2 bits
			3, // 13 = b01101 = 3 bits
			3, // 14 = b01110 = 3 bits
			4, // 15 = b01111 = 4 bits
			1, // 16 = b10000 = 1 bit
			2, // 17 = b10001 = 2 bits
			2, // 18 = b10010 = 2 bits
			3, // 19 = b10011 = 3 bits
			2, // 20 = b10100 = 2 bits
			3, // 21 = b10101 = 3 bits
			3, // 22 = b10110 = 3 bits
			4, // 23 = b10111 = 4 bits
			2, // 24 = b11000 = 2 bits
			3, // 25 = b11001 = 3 bits
			3, // 26 = b11010 = 3 bits
			4, // 27 = b11011 = 4 bits
			3, // 28 = b11100 = 3 bits
			4, // 29 = b11101 = 4 bits
			4, // 30 = b11110 = 4 bits
			5  // 31 = b11111 = 5 bits
		};

		// Update all commands added by BindBuffers according to the bits set in m_Stages.
		const size_t numShaderBufferCommands = cNumBits[mStages];
		size_t shaderBufferCommandOffset = mShaderBufferCommandOffset;
		for (size_t k = 0; k < numShaderBufferCommands; ++k)
		{
			auto* pShaderBufferCommand = reinterpret_cast<CbShaderBuffer*>(pCommandBuffer->getCommandFromOffset(shaderBufferCommandOffset));
            shaderBufferCommandOffset -= cCommandFixedSize;

			assert(pShaderBufferCommand != nullptr);
			if (pShaderBufferCommand != nullptr)
			{
				assert(pShaderBufferCommand->bufferPacked == mBuffers[mCurrentIndex]);
				pShaderBufferCommand->bindSizeBytes = static_cast<uint32_t>(mMappedOffset) - pShaderBufferCommand->bindOffset;
			}
		}

		mShaderBufferCommandOffset = cInvalidCommandOffset;
	}
}
