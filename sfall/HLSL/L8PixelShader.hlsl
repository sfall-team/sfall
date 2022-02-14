texture image;
texture palette;
texture head;
sampler s0 = sampler_state { texture=<image>; };
sampler s1 = sampler_state { texture=<palette>; minFilter=none;   magFilter=none;   addressU=clamp; addressV=clamp; };
sampler s2 = sampler_state { texture=<head>;    minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };
float2 size;
float2 corner;

// shader for displaying head textures
float4 P1(in float2 Tex : TEXCOORD0) : COLOR0
{
	float backdrop = tex2D(s0, Tex).r;
	float3 result;

	if (abs(backdrop - 1.0) < 0.001) {
		result = tex2D(s2, saturate((Tex - corner) / size));
	} else {
		result = tex1D(s1, backdrop).bgr * 4;
	}
	return float4(result, 1);
}

technique T1
{
	pass p1 { PixelShader = compile ps_2_0 P1(); }
}

// main shader
float4 P0(in float2 Tex : TEXCOORD0) : COLOR0
{
	float3 result = tex1D(s1, tex2D(s0, Tex).r);
	return float4(result.bgr * 4, 1);
}

technique T0
{
	pass p0 { PixelShader = compile ps_2_0 P0(); }
}
