@insertpiece( SetCrossPlatformSettings )

// outputs from vertex shader go here. can be interpolated to pixel shader
struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
	nointerpolation uint drawId : TEXCOORD0;
};

@property( hlms_shadowcaster )
	@insertpiece( DeclShadowCasterMacros )
@end

#include "InstanceData.hlsl"

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
	InstanceData instanceData[2];
@else
	InstanceData instanceData[4096];
@end
};

#include "MaterialData.hlsl"

// Materials - data collected from HlmsDatablock instances
CONST_BUFFER(MaterialBuffer, 2)
{
@property(fast_shader_build_hack)
	MaterialData materialData[2];
@else
	MaterialData materialData[4096];
@end
};


@insertpiece(DeclOutputType)

@insertpiece(output_type) main(vs_out input)
{
	PS_OUTPUT outPs;
	
@property(hlms_shadowcaster)
	@insertpiece(DoShadowCastPS)
@end
@property(!hlms_render_depth_only)
  uint materialIndex = uint(instanceData[input.drawId].worldMaterialIdx.x);
  outPs.colour0 = materialData[materialIndex].color; // must return an RGBA colour
  return outPs;
@end
}
