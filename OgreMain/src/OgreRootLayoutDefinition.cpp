/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "OgreStableHeaders.h"
#include "OgreRootLayoutDefinition.h"

namespace Ogre
{
    //-------------------------------------------------------------------------
    RootLayoutDefinition::RootLayoutDefinition() {}
    //-------------------------------------------------------------------------
    uint16 RootLayoutDefinition::getBindingSlot( DescBindingTypes::DescBindingTypes bindingType,
                                                 uint16 slot ) const
    {
        OGRE_ASSERT_LOW( slot < mTranslatedSlots[bindingType].size() );
        return mTranslatedSlots[bindingType][slot];
    }
    //-------------------------------------------------------------------------
    void RootLayoutDefinition::setBinding( const size_t setIdx,
                                           DescBindingTypes::DescBindingTypes bindingType,
                                           const DescBindingRange &descBindingRange )
    {
        mDefinition.mDescBindingRanges[setIdx][bindingType] = descBindingRange;
    }
    //-------------------------------------------------------------------------
    const DescBindingRange &RootLayoutDefinition::getBinding(
        const size_t setIdx, DescBindingTypes::DescBindingTypes bindingType ) const
    {
        return mDefinition.mDescBindingRanges[setIdx][bindingType];
    }
    //-------------------------------------------------------------------------
    void RootLayoutDefinition::addArrayBinding( DescBindingTypes::DescBindingTypes bindingType,
                                                ArrayDesc arrayDesc )
    {
        mDefinition.addArrayBinding( bindingType, arrayDesc );
    }
    //-------------------------------------------------------------------------
    void RootLayoutDefinition::setCompute( bool value ) { mDefinition.mCompute = value; }
    //-------------------------------------------------------------------------
    bool RootLayoutDefinition::isCompute() const { return mDefinition.mCompute; }
    //-------------------------------------------------------------------------
    void RootLayoutDefinition::setParamsBuffStages( uint8 value )
    {
        mDefinition.mParamsBuffStages = value;
    }
    //-------------------------------------------------------------------------
    uint8 RootLayoutDefinition::getParamsBuffStages() const { return mDefinition.mParamsBuffStages; }
    //-------------------------------------------------------------------------
    void RootLayoutDefinition::setBaked( const size_t setIdx, bool value )
    {
        mDefinition.mBaked[setIdx] = value;
    }
    //-------------------------------------------------------------------------
    bool RootLayoutDefinition::isBaked( const size_t setIdx ) const
    {
        return mDefinition.mBaked[setIdx];
    }
    //-----------------------------------------------------------------------------------
    void RootLayoutDefinition::copyCommonAttributes( RootLayout &rootLayout )
    {
        rootLayout.mCompute = mDefinition.mCompute;
        rootLayout.mParamsBuffStages = mDefinition.mParamsBuffStages;
        for( size_t k = 0; k < OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++k )
            rootLayout.mBaked[k] = mDefinition.mBaked[k];
    }
    //-----------------------------------------------------------------------------------
    void RootLayoutDefinition::translateBindings( RootLayout &rootLayout, uint16 &slot,
                                                  DescBindingTypes::DescBindingTypes bindingType )
    {
        // prepare translated slots
        size_t maxSlot = 0;
        for( size_t k = 0; k < OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++k )
        {
            const DescBindingRange &bindingRange = mDefinition.mDescBindingRanges[k][bindingType];
            if( maxSlot < bindingRange.end )
                maxSlot = bindingRange.end;
        }
        mTranslatedSlots[bindingType].resize( maxSlot, 0xffff );

        for( size_t k = 0; k < OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++k )
        {
            const DescBindingRange &bindingRange = mDefinition.mDescBindingRanges[k][bindingType];
            if( bindingRange.isInUse() )
            {
                // add binding range
                const uint16 count = bindingRange.end - bindingRange.start;
                rootLayout.mDescBindingRanges[k][bindingType] = DescBindingRange( slot, slot + count );

                for( uint16 i = bindingRange.start; i < bindingRange.end; ++i )
                    mTranslatedSlots[bindingType][i] = slot + i - bindingRange.start;

                // add ParamBuffer array bindings
                for( size_t k = 0; k < mDefinition.getNumArrayBindings( bindingType ); ++k )
                {
                    const ArrayDesc arrayDesc = mDefinition.getArrayBinding( bindingType, k );
                    if( arrayDesc.bindingIdx >= bindingRange.start &&
                        arrayDesc.bindingIdx + arrayDesc.arraySize <= bindingRange.end )
                    {
                        rootLayout.addArrayBinding(
                            bindingType, ArrayDesc( slot + arrayDesc.bindingIdx - bindingRange.start,
                                                    arrayDesc.arraySize ) );
                    }
                }

                slot += count;
            }
        }
    }
}  // namespace Ogre
