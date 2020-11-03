//--------------------------------------------------------------------------------------
// File: DX11 Framework.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;

    float4 DiffuseMtrl; 
    float4 DiffuseLight; 
    //a vector that points in the dirrection of the light source
    float3 LightVecW;

    float4 AmbientMaterial;
    float4 AmbientLight;

    float4 SpecularMtrl; 
    float4 SpecularLight; 
    float SpecularPower; 
    float3 EyePosW;
}

//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 normalW : NORMAL;
    float3 eye : POSITION;
    float2 Tex : TEXCOORD;
    //float3 Tan : TANGENT;
    //float3  Bitan : BITANGENT;
};

//----------------------------------------------------------------------------
// Vertex Shader - Implements Gouraud Shading using Diffuse lighting only
//----------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos : POSITION, float3 NormalL : NORMAL, float2 Tex : TEXCOORD /*,float3 Tan : TANGENT, float3  Bitan : BITANGENT*/)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul(Pos, World); 

    output.eye = normalize(EyePosW.xyz - output.Pos.xyz);

    output.Pos = mul(output.Pos, View);  
    output.Pos = mul(output.Pos, Projection);
    output.Tex = mul(output.Tex, World);

    // W component of vector is 0 as vectors cannot be translated     
    float3 normalW = mul(float4(NormalL, 0.0f), World).xyz;
    output.normalW = normalize(normalW);
    output.Tex = Tex;

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    float4 textureColour = txDiffuse.Sample(samLinear, input.Tex);
    //compute reflection vector
    input.normalW = normalize(input.normalW);
    float r = reflect(-LightVecW, input.normalW);
    // Compute Colour using Diffuse lighting only     
    float diffuseAmount = max(dot(LightVecW, input.normalW), 0.0f);
    // Determine how much (if any) specular light makes it into the eye.
    float specularAmount = pow(max(dot(r, input.eye), 0), SpecularPower);

    //specular calc
    float3 specular = specularAmount * (SpecularMtrl * SpecularLight).rgb;
    //ambient calc
    float3 ambient = AmbientMaterial * AmbientLight;
    //diffuse calc
    float3 diffuse = diffuseAmount * (DiffuseMtrl * DiffuseLight).rgb;

    float4 finalColor;
    finalColor.rgb = clamp(diffuse, 0, 1) + ambient + clamp(specular, 0, 1);
    finalColor.a = DiffuseMtrl.a;

    return textureColour + finalColor;
}