texture image;
texture palette;
texture head;
texture highlight;
sampler s0 = sampler_state { texture=<image>; };
sampler s1 = sampler_state { texture=<palette>;   minFilter=none;   magFilter=none;   addressU=clamp; addressV=clamp; };
sampler s2 = sampler_state { texture=<head>;      minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };
sampler s3 = sampler_state { texture=<highlight>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };
float2 size;
float2 corner;
float2 sizehl;
float2 cornerhl;
int showhl;

// shader for displaying head textures
float4 P1(in float2 Tex : TEXCOORD0) : COLOR0
{
	float backdrop = tex2D(s0, Tex).a;
	float3 result;

	if (abs(backdrop - 1.0) < 0.001) { // (48.0 / 255.0) // 48 - key index color
		result = tex2D(s2, saturate((Tex - corner) / size));
	} else {
		result = tex1D(s1, backdrop).bgr * 4; // get color in palette
	}
	// blend highlights
	if (showhl) {
		float4 h = tex2D(s3, saturate((Tex - cornerhl) / sizehl));
		result = saturate(result + h); // saturate(result * (1 - h.a) * h.rgb * h.a)
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
	float3 result = tex1D(s1, tex2D(s0, Tex).a); // get color in palette
	return float4(result.bgr * 4, 1);
}

technique T0
{
	pass p0 { PixelShader = compile ps_2_0 P0(); }
}
