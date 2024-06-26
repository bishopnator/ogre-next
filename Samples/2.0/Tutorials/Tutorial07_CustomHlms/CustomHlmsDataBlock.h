#pragma once

// OGRE
#include <OgreConstBufferPool.h>
#include <OgreHlmsDatablock.h>

// forwards
namespace Demo
{
	class CustomHlms;
}

namespace Demo
{
	/// A data block implementation for CustomHlms.
	class CustomHlmsDataBlock : public Ogre::HlmsDatablock, public Ogre::ConstBufferPoolUser
	{
	public:
		explicit CustomHlmsDataBlock(Ogre::IdString name, CustomHlms* pCreator, const Ogre::HlmsMacroblock* pMacroblock, const Ogre::HlmsBlendblock* pBlendblock, const Ogre::HlmsParamVec& params);
		~CustomHlmsDataBlock() override;

		static size_t getPrimaryBufferDatablockSize();
		static size_t getSecondaryBufferDatablockSize();

		/// Set new color to the datablock.
		void setColor(const Ogre::Vector4& color);

		/// Get the current color set to the datablock.
		const Ogre::Vector4& getColor() const;

	protected: // Ogre::ConstBufferPoolUser interface
		void uploadToConstBuffer(char* dstPtr, Ogre::uint8 dirtyFlags) override;

	private:
		Ogre::Vector4 mColor;
	};
} // namespace Demo
