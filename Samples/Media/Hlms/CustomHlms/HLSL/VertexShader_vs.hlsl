@insertpiece( SetCrossPlatformSettings )

// vertex attributes go here to input to the vertex shader
struct vs_in
{
    float3 position_local : POSITION;
	uint drawId : DRAWID;
};

// outputs from vertex shader go here. can be interpolated to pixel shader
struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
};

@insertpiece(Common_Matrix_DeclUnpackMatrix4x4)
@insertpiece(Common_Matrix_DeclUnpackMatrix4x3)

// pass buffer
CONST_BUFFER_STRUCT_BEGIN(PassBuffer, 0)
{
	float4x4 viewProj;
}
CONST_BUFFER_STRUCT_END(passBuf);

// Automatic instancing - collected world transformations
ReadOnlyBuffer(0, float4, worldMatricesBuf);

vs_out main(vs_in input)
{
  vs_out output = (vs_out)0; // zero the memory first

  //ogre_float4x3 worldMat = UNPACK_MAT4x3(worldMatricesBuf, input.drawId);
  //float4 worldPos = float4(mul(input.position_local, worldMat).xyz, 1.0f);
  float4x4 worldMat = UNPACK_MAT4(worldMatricesBuf, input.drawId);
  float4 worldPos = mul(float4(input.position_local, 1.0), worldMat);
  
  output.position_clip = mul(worldPos, passBuf.viewProj);
  return output;
}
