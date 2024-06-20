#pragma once

// OGRE
#include <OgreHlmsDatablock.h>

// forwards
namespace Demo
{
	class CustomHlms;
}

namespace Demo
{
	/// A data block implementation for CustomHlms.
	class CustomHlmsDataBlock : public Ogre::HlmsDatablock
	{
	public:
		explicit CustomHlmsDataBlock(Ogre::IdString name, CustomHlms* pCreator, const Ogre::HlmsMacroblock* pMacroblock, const Ogre::HlmsBlendblock* pBlendblock, const Ogre::HlmsParamVec& params);
		~CustomHlmsDataBlock() override;
	};
} // namespace Demo
