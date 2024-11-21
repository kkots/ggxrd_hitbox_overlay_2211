
float4 g_ScreenSize : register(c0);

sampler2D g_MySampler : register(s0)
{
};

float4 main(float4 pos : VPOS, float4 clr : COLOR0) : COLOR0
{
    float2 texUV = { (pos.x + 0.5f) / g_ScreenSize.x, (pos.y + 0.5f) / g_ScreenSize.y };
    float4 framebufferColorUnderThePixel = tex2D(g_MySampler, texUV);
    
    float4 absDiff = abs(framebufferColorUnderThePixel - clr);
    float sum = absDiff.r + absDiff.g + absDiff.b;
        
    float mult = (float)(sum >= 0.0824f); // 21 out of 255
    return float4(clr.r * mult, clr.g * mult, clr.b * mult, 0.0f);
    
}
