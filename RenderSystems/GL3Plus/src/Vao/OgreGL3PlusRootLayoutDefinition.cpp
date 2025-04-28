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

#include "Vao/OgreGL3PlusRootLayoutDefinition.h"

#include "OgreLwString.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------------
    GL3PlusRootLayoutDefinition::GL3PlusRootLayoutDefinition( bool readOnlyIsTexBuffer,
                                                              bool emulateTexBuffers ) :
        mReadOnlyIsTexBuffer( readOnlyIsTexBuffer ),
        mEmulateTexBuffers( emulateTexBuffers )
    {
    }
    //-----------------------------------------------------------------------------------
    RootLayout GL3PlusRootLayoutDefinition::createRootLayout()
    {
        // GL_UNIFORM_BUFFER: ParamBuffer, ConstBuffer
        // GL_TEXTURE_BUFFER/GL_TEXTURE/GL_SHADER_STORAGE_BUFFER : ReadOnlyBuffer
        // GL_TEXTURE_BUFFER/GL_TEXTURE : TexBuffer
        // GL_SHADER_STORAGE_BUFFER: UavBuffer, UavTexture
        // GL_TEXTURE: Texture
        // GL_SAMPLER: Sampler
        uint16 ssboCounter = 0;
        uint16 uniformBufferCounter = 0;
        uint16 textureBufferCounter = 0;
        uint16 textureCounter = 0;
        uint16 samplerCounter = 0;

        RootLayout rootLayout;
        copyCommonAttributes( rootLayout );

        // add ParamBuffer, ConstBuffer bindings
        translateBindings( rootLayout, uniformBufferCounter, DescBindingTypes::ParamBuffer );
        translateBindings( rootLayout, uniformBufferCounter, DescBindingTypes::ConstBuffer );

        // add ReadOnlyBuffer bindings
        if (!mReadOnlyIsTexBuffer)
        {
            // ReadOnlyBuffer as GL3PlusReadOnlyUavBufferPacked --> GL_SHADER_STORAGE_BUFFER
            translateBindings( rootLayout, ssboCounter, DescBindingTypes::ReadOnlyBuffer );
        }
        else if (!mEmulateTexBuffers)
        {
            // ReadOnlyBuffer as GL3PlusReadOnlyTexBufferPacked --> GL_TEXTURE_BUFFER
            translateBindings( rootLayout, textureBufferCounter, DescBindingTypes::ReadOnlyBuffer );
        }
        else
        {
            // ReadOnlyBuffer as GL3PlusReadOnlyBufferEmulatedPacked --> (GL_TEXTURE0 + slot)
            translateBindings( rootLayout, textureCounter, DescBindingTypes::ReadOnlyBuffer );
        }

        // add TexBuffer bindings
        if (!mEmulateTexBuffers)
        {
            // TexBuffer as GL3PlusTexBufferPacked --> GL_TEXTURE_BUFFER
            translateBindings( rootLayout, textureBufferCounter, DescBindingTypes::TexBuffer );
        }
        else
        {
            // TexBuffer as GL3PlusTexBufferEmulatedPacked --> (GL_TEXTURE0 + slot)
            translateBindings( rootLayout, textureCounter, DescBindingTypes::TexBuffer );
        }

        // add UavBuffer, UavTexture bindings
        translateBindings( rootLayout, ssboCounter, DescBindingTypes::TexBuffer );
        translateBindings( rootLayout, ssboCounter, DescBindingTypes::UavBuffer );
        translateBindings( rootLayout, ssboCounter, DescBindingTypes::UavTexture );

        // add Texture bindings
        translateBindings( rootLayout, textureCounter, DescBindingTypes::Texture );

        // add Sampler bindings
        translateBindings( rootLayout, samplerCounter, DescBindingTypes::Sampler );

        return rootLayout;
    }
    //-----------------------------------------------------------------------------------
    String GL3PlusRootLayoutDefinition::createShaderPreprocessorDefinitions() const
    {
        char buffer[1024];
        LwString out( buffer, 1024 );

        const auto appendPreprocessor = [this, &out]( DescBindingTypes::DescBindingTypes bindingType,
                                                      const char *baseName ) {
            for( size_t k = 0; k < mTranslatedSlots[bindingType].size(); ++k )
            {
                if( mTranslatedSlots[bindingType][k] != 0xffff )
                    out.a( "#define ", baseName, k, mTranslatedSlots[bindingType][k],
                           "\n" );
            }
        };

        appendPreprocessor( DescBindingTypes::ParamBuffer, "ogre_param_buffer_" );
        appendPreprocessor( DescBindingTypes::ConstBuffer, "ogre_const_buffer_" );
        appendPreprocessor( DescBindingTypes::ReadOnlyBuffer, "ogre_read_only_buffer_" );
        appendPreprocessor( DescBindingTypes::TexBuffer, "ogre_tex_buffer_" );
        appendPreprocessor( DescBindingTypes::Texture, "ogre_texture_" );
        appendPreprocessor( DescBindingTypes::Sampler, "ogre_sampler_" );
        appendPreprocessor( DescBindingTypes::UavBuffer, "ogre_uav_buffer_" );
        appendPreprocessor( DescBindingTypes::UavTexture, "ogre_uav_texture_" );

        return buffer;
    }
}  // namespace Ogre
