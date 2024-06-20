#include "CustomHlmsDataBlock.h"
#include "CustomHlms.h"

using namespace Demo;

//////////////////////////////////////////////////////////////////////////
CustomHlmsDataBlock::CustomHlmsDataBlock(Ogre::IdString name, CustomHlms* pCreator, const Ogre::HlmsMacroblock* pMacroblock, const Ogre::HlmsBlendblock* pBlendblock, const Ogre::HlmsParamVec& params)
: Ogre::HlmsDatablock(name, pCreator, pMacroblock, pBlendblock, params)
{
}

//////////////////////////////////////////////////////////////////////////
CustomHlmsDataBlock::~CustomHlmsDataBlock() = default;
