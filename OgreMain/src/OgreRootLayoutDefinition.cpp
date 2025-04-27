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
}  // namespace Ogre
