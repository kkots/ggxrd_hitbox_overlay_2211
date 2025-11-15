
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

float4 colourdodge_ps(float4 pos : VPOS, float4 clr : COLOR0) : COLOR0
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

float4x4 matWorldViewProj : register(c0);

struct VS_INPUT
{

    float3 position : POSITION;
    float4 diffuse : COLOR;
};

struct VS_OUTPUT
{
	float4 position : POSITION;
	float4 diffuse : COLOR0;
};

// Thanks to Worse Than You for discovering the fix for AMD:
// without a vertex shader, if only a pixel shader was used, it would not draw anything
// while that pixel shader is in use. On AMD Direct3D 9, non-NULL pixel shader must be
// accompanied by a non-NULL vertex shader.
VS_OUTPUT main_vs(in VS_INPUT vertex)
{
	VS_OUTPUT output = ( VS_OUTPUT )0;
	
	output.position = mul( float4(vertex.position, 1.F), matWorldViewProj);
	output.diffuse = vertex.diffuse;
	
	return output;
}
