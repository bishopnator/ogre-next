#pragma once

// OGRE
#include <OgreResourceTransition.h>
#include <ogrestd/unordered_map.h>

namespace Ogre
{
	/// Maps CompositorPassScene's identifier to resource access. This tells HlmsExt about the usage of the buffers created by this handler
	/// within the currently executed scene pass so it can properly issue the memory barriers.
	using ResourceAccessMap = unordered_map<uint32_t, ResourceAccess::ResourceAccess>::type;
} // namespace Ogre
