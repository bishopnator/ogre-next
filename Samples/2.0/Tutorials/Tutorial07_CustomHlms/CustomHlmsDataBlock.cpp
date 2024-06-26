#include "CustomHlmsDataBlock.h"
#include "CustomHlms.h"

using namespace Demo;

//////////////////////////////////////////////////////////////////////////
CustomHlmsDataBlock::CustomHlmsDataBlock(Ogre::IdString name, CustomHlms* pCreator, const Ogre::HlmsMacroblock* pMacroblock, const Ogre::HlmsBlendblock* pBlendblock, const Ogre::HlmsParamVec& params)
: Ogre::HlmsDatablock(name, pCreator, pMacroblock, pBlendblock, params)
, mColor(1, 0, 0, 1)
{
	pCreator->requestDatablockSlot(*this, 0, false);
}

//////////////////////////////////////////////////////////////////////////
CustomHlmsDataBlock::~CustomHlmsDataBlock()
{
	static_cast<CustomHlms*>(mCreator)->releaseDatablockSlot(*this);
}

//////////////////////////////////////////////////////////////////////////
void CustomHlmsDataBlock::uploadToConstBuffer(char* dstPtr, Ogre::uint8 /*dirtyFlags*/)
{
	memcpy(dstPtr, &mColor, sizeof(Ogre::Vector4));
}

//////////////////////////////////////////////////////////////////////////
size_t CustomHlmsDataBlock::getPrimaryBufferDatablockSize()
{
	return sizeof(Ogre::Vector4);
}

//////////////////////////////////////////////////////////////////////////
size_t CustomHlmsDataBlock::getSecondaryBufferDatablockSize()
{
	return 0; // secondary buffer is not used by this data block
}

//////////////////////////////////////////////////////////////////////////
void CustomHlmsDataBlock::setColor(const Ogre::Vector4& color)
{
	mColor = color;
    static_cast<CustomHlms*>(mCreator)->invalidateDatablockSlot(*this);
}

//////////////////////////////////////////////////////////////////////////
const Ogre::Vector4& CustomHlmsDataBlock::getColor() const
{
	return mColor;
}
