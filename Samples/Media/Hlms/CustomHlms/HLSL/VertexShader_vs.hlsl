@insertpiece( SetCrossPlatformSettings )
@insertpiece( define_n4096 )

// vertex attributes go here to input to the vertex shader
struct vs_in
{
    float4 position_local : POSITION;
	uint drawId : DRAWID;
};

// outputs from vertex shader go here. can be interpolated to pixel shader
struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
	nointerpolation uint drawId : TEXCOORD0;
};

@insertpiece(Common_Matrix_DeclUnpackMatrix4x4)
@insertpiece(Common_Matrix_DeclUnpackMatrix4x3)

#include "PassData.hlsl"

// pass buffer
CONST_BUFFER(PassBuffer, 0)
{
	PassData passData;
}

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
	InstanceData instanceData[n4096];
};

// Automatic instancing - collected world transformations
ReadOnlyBuffer(0, float4, worldMatricesBuf);

@piece(shadow_caster_update_pos)
  //float shadowConstantBias = -uintBitsToFloat(instanceData[input.drawId].worldMaterialIdx.y) * passData.depthRange.y;
  float shadowConstantBias = -instanceData[input.drawId].worldMaterialIdx.y * passData.depthRange.y;
  output.position_clip.z = output.position_clip.z + shadowConstantBias;
@end

vs_out main(vs_in input)
{
  vs_out output = (vs_out)0; // zero the memory first

  // either following code with 4x3 matrix:
  ogre_float4x3 worldMat = UNPACK_MAT4x3(worldMatricesBuf, input.drawId);
  float4 worldPos = float4(mul(input.position_local, worldMat).xyz, 1.0f);  
  // or using 4x4 matrix:
  //float4x4 worldMat = UNPACK_MAT4(worldMatricesBuf, input.drawId);
  //float4 worldPos = mul(input.position_local, worldMat);
  
  output.position_clip = mul(worldPos, passData.viewProj);
  output.drawId = input.drawId;
  
@property( hlms_shadowcaster )
  @insertpiece(shadow_caster_update_pos)
@end
  
  return output;
}