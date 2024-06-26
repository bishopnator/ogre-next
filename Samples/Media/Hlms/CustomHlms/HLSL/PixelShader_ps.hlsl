@insertpiece( SetCrossPlatformSettings )

// outputs from vertex shader go here. can be interpolated to pixel shader
struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
	nointerpolation uint drawId : TEXCOORD0;
};

@property( hlms_shadowcaster )
	@insertpiece( DeclShadowCasterMacros )
@end

@insertpiece(DeclOutputType)

@insertpiece(output_type) main(vs_out input)
@property(!hlms_render_depth_only)
 : SV_TARGET
@end
{
	PS_OUTPUT outPs;
	
@property( hlms_shadowcaster )
	@insertpiece( DoShadowCastPS )
@end
@property( !hlms_render_depth_only )
  outPs.colour0 = float4( 1.0f, 0.0f, 1.0f, 1.0f ); // must return an RGBA colour
  return outPs;
@end
}
