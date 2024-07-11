@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(std140) uniform;

vulkan_layout( OGRE_POSITION ) in vec4 position_local;
@property( GL_ARB_base_instance )
	vulkan_layout( OGRE_DRAWID ) in uint drawId;
@end

vulkan_layout( location = 0 ) out block
{
	FLAT_INTERPOLANT( uint drawId, 0 );
} outVs;

@insertpiece(Common_Matrix_DeclUnpackMatrix4x4)
@insertpiece(Common_Matrix_DeclUnpackMatrix4x3)

@insertpiece(PassData.h)

// pass buffer
CONST_BUFFER(PassBuffer, 0)
{
	PassData passData;
};

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

// Automatic instancing - collected world transformations
ReadOnlyBufferF(0, float4, worldMatricesBuf);

void main()
{
@property( !GL_ARB_base_instance )
	const uint drawId = baseInstance + uint( gl_InstanceID );
@end

	// either following code with 4x3 matrix:
	ogre_float4x3 worldMat = UNPACK_MAT4x3(worldMatricesBuf, drawId);
	float4 worldPos = float4(mul(position_local, worldMat).xyz, 1.0f);  
	// or using 4x4 matrix:
	//float4x4 worldMat = UNPACK_MAT4(worldMatricesBuf, drawId);
	//float4 worldPos = mul(position_local, worldMat);
	gl_Position = mul(worldPos, passData.viewProj);
	outVs.drawId = drawId;
  
@property( hlms_shadowcaster )
  float shadowConstantBias = -instanceData[drawId].worldMaterialIdx.y * passData.depthRange.y;
  gl_Position.z = gl_Position.z + shadowConstantBias;
@end
}
