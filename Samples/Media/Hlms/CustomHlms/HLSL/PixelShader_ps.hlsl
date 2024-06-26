@insertpiece( SetCrossPlatformSettings )

// outputs from vertex shader go here. can be interpolated to pixel shader
struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
	nointerpolation uint drawId : TEXCOORD0;
};

@property( hlms_shadowcaster )
	@insertpiece( DeclShadowCasterMacros )
@end

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
@property( fast_shader_build_hack )
	float4 worldMaterialIdx[2];
@else
	float4 worldMaterialIdx[4096];
@end
};

// Materials - data collected from HlmsDatablock instances
CONST_BUFFER(MaterialData, 2)
{
@property(fast_shader_build_hack)
	float4 color[2];
@else
	float4 color[4096];
@end
};


@insertpiece(DeclOutputType)

@insertpiece(output_type) main(vs_out input)
@property(!hlms_render_depth_only)
 : SV_TARGET
@end
{
	PS_OUTPUT outPs;
	
@property(hlms_shadowcaster)
	@insertpiece(DoShadowCastPS)
@end
@property(!hlms_render_depth_only)
  uint materialIndex = uint(worldMaterialIdx[input.drawId].x);
  outPs.colour0 = color[materialIndex]; // must return an RGBA colour
  return outPs;
@end
}
