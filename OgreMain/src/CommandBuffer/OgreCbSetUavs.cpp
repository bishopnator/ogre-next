/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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

#include "CommandBuffer/OgreCbSetUavs.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
    void CommandBuffer::execute_setUavsPS( CommandBuffer *_this, const CbBase *RESTRICT_ALIAS _cmd )
    {
        const CbSetUavs *cmd = static_cast<const CbSetUavs *>( _cmd );
        _this->mRenderSystem->_setUavPS( cmd->slotStart, cmd->set );
    }

    CbSetUavs::CbSetUavs( uint32 _slotStart, const DescriptorSetUav *_set ) :
        CbBase( CB_SET_UAVS_PS ),
        slotStart( _slotStart ),
        set( _set )
    {
    }
}  // namespace Ogre
