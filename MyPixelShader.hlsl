
float4 g_ScreenSize : register(c0);

// old wording: suspected to require D3D10
//sampler2D g_MySampler : register(s0)
//{
//};
Texture2D g_BackbufferCopy : register (t0);
sampler g_MySampler =
sampler_state
{
    Texture = <g_BackbufferCopy>;
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

float4 main(float4 pos : VPOS, float4 clr : COLOR0) : COLOR0
{
	float2 texUV = { (pos.x + 0.5f) / g_ScreenSize.x, (pos.y + 0.5f) / g_ScreenSize.y };
	float4 framebufferColorUnderThePixel = tex2D(g_MySampler, texUV);
	
	float4 diffLinear = framebufferColorUnderThePixel - clr;
	float4 diffSquared = dot(diffLinear, diffLinear);
	float isLowRed = (float)(
		(clr.r + framebufferColorUnderThePixel.r) < 1.0
	);
	float4 diffConverted = diffSquared * float4(2.0f, 4.0f, 3.0f, 1.0f) * isLowRed
		+ diffSquared * float4(3.0f, 4.0f, 2.0f, 1.0f) * (1.0f - isLowRed);
	float sum = diffConverted.r + diffConverted.g + diffConverted.b;
	
	float mult = (float)(sum >= 0.54f);
	return float4(clr.r * mult, clr.g * mult, clr.b * mult, 1.0f);  // alpha changed from 0 to 1 because in OBS dodging mode outlines would be invisible

}
