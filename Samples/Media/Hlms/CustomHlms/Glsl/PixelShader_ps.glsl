@insertpiece( SetCrossPlatformSettings )

layout(std140) uniform;
#define FRAG_COLOR		0

@property( !hlms_shadowcaster )
layout(location = FRAG_COLOR, index = 0) out vec4 outColour;
@end @property( hlms_shadowcaster )
layout(location = FRAG_COLOR, index = 0) out float outColour;
@end

@insertpiece(InstanceData.h)

// Uniforms that change per Item/Entity
CONST_BUFFER(InstanceBuffer, 1)
{
	// .x =
	// Contains the material's start index.
	//
	// .y =
	// shadowConstantBias. Send the bias directly to avoid an
	// unnecessary indirection during the shadow mapping pass.
	// Must be loaded with uintBitsToFloat
	InstanceData instanceData[4096];
};

@insertpiece(MaterialData.h)

// Materials - data collected from HlmsDatablock instances
CONST_BUFFER(MaterialBuffer, 2)
{
	MaterialData materialData[4096];
};

@property( !hlms_shadowcaster || !hlms_shadow_uses_depth_texture || exponential_shadow_maps )
	vulkan_layout( location = 0 ) in block
	{
		FLAT_INTERPOLANT( uint drawId, 0 );
	} input;
@end

void main()
{
@property(!hlms_render_depth_only)
  uint materialIndex = uint(instanceData[input.drawId].worldMaterialIdx.x);
  outColour = materialData[materialIndex].color; // must return an RGBA colour
@end
@property( hlms_shadowcaster )
	@insertpiece( DoShadowCastPS )
@end
}
