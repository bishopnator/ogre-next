#pragma once
#include "OgreHlmsExtPrerequisites.h"

// OGRE
#include <OgreCommon.h>

namespace Ogre
{
	// Convert s shader written in HLSL to GLSL.
	_OgreHlmsExtExport String convertHlsl2Glsl(const String& hlsl, const String& debugFilenameOutput, ShaderType shaderType, const RootLayout& rootLayout, Archive* pDataFolder = nullptr);
} // namespace Ogre
