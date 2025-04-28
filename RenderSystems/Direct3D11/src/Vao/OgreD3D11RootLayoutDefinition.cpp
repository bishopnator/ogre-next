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

#include "Vao/OgreD3D11RootLayoutDefinition.h"

#include "OgreLwString.h"
#include "OgreRoot.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------------
    D3D11RootLayoutDefinition::D3D11RootLayoutDefinition() {}
    //-----------------------------------------------------------------------------------
    RootLayout D3D11RootLayoutDefinition::createRootLayout()
    {
        // bx: ParamBuffer, ConstBuffer
        // tx: ReadOnlyBuffer, TexBuffer, Texture
        // sx: Sampler
        // ux: UavBuffer, UavTexture
        RootLayout rootLayout;
        copyCommonAttributes( rootLayout );

        // add ParamBuffer, ConstBuffer bindings
        uint16 bcounter = 0;
        translateBindings( rootLayout, bcounter, DescBindingTypes::ParamBuffer );
        translateBindings( rootLayout, bcounter, DescBindingTypes::ConstBuffer );

        // add ReadOnlyBuffer, TexBuffer, Texture bindings
        uint16 tcounter = 0;
        translateBindings( rootLayout, tcounter, DescBindingTypes::ReadOnlyBuffer );
        translateBindings( rootLayout, tcounter, DescBindingTypes::TexBuffer );
        translateBindings( rootLayout, tcounter, DescBindingTypes::Texture );

        // add Sampler bindings
        uint16 scounter = 0;
        translateBindings( rootLayout, scounter, DescBindingTypes::Sampler );

        // add UavBuffer, UavTexture bindings
        uint16 ucounter = 0;  // u-register is shared with color binding in pixel shader and ogre updates
                              // the actual start index when it binds the UAV buffers
        translateBindings( rootLayout, ucounter, DescBindingTypes::UavBuffer );
        translateBindings( rootLayout, ucounter, DescBindingTypes::UavTexture );

        return rootLayout;
    }
    //-----------------------------------------------------------------------------------
    String D3D11RootLayoutDefinition::createShaderPreprocessorDefinitions() const
    {
        char buffer[1024];
        LwString out( buffer, 1024 );

        const auto appendPreprocessor = [this, &out]( DescBindingTypes::DescBindingTypes bindingType,
                                                      const char *baseName, const char *registerName,
                                                      uint16 offset = 0 ) {
            for( size_t k = 0; k < mTranslatedSlots[bindingType].size(); ++k )
            {
                if( mTranslatedSlots[bindingType][k] != 0xffff )
                    out.a( "#define ", baseName, k, registerName,
                           offset + mTranslatedSlots[bindingType][k], "\n" );
            }
        };

        appendPreprocessor( DescBindingTypes::ParamBuffer, "ogre_param_buffer_", " b" );
        appendPreprocessor( DescBindingTypes::ConstBuffer, "ogre_const_buffer_", " b" );
        appendPreprocessor( DescBindingTypes::ReadOnlyBuffer, "ogre_read_only_buffer_", " t" );
        appendPreprocessor( DescBindingTypes::TexBuffer, "ogre_tex_buffer_", " t" );
        appendPreprocessor( DescBindingTypes::Texture, "ogre_texture_", " t" );
        appendPreprocessor( DescBindingTypes::Sampler, "ogre_sampler_", " s" );

        // uav buffers are bound after the color targets - e.g. if shader outputs 2 colors, the first uav buffer is bound to u2 (u0 and u1 are used by colors)
        const uint8 numColorEntries = Root::getSingleton().getRenderSystem()->getCurrentPassDescriptor()->getNumColourEntries();
        appendPreprocessor( DescBindingTypes::UavBuffer, "ogre_uav_buffer_", " u", numColorEntries );
        appendPreprocessor( DescBindingTypes::UavTexture, "ogre_uav_texture_", " u", numColorEntries );

        return buffer;
    }
}  // namespace Ogre
