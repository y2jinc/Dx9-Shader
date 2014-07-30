// -------------------------------------------------------------
// Ȯ�걤
// 
// Copyright (c) 2003 IMAGIRE Takashi. All rights reserved.
// -------------------------------------------------------------

// -------------------------------------------------------------
// ��������
// -------------------------------------------------------------

float4x4 mWVP;
float4x4 mW;

float3 vLightDir;							// ������ ����

// �������
float4 I_a = { 0.3f, 0.3f, 0.3f, 0.0f };    // ambient
float4 I_d = { 0.7f, 0.7f, 0.7f, 0.0f };    // diffuse

// �ݻ���
float4 k_a = { 1.0f, 1.0f, 1.0f, 1.0f };    // ambient
float4 k_d = { 1.0f, 1.0f, 1.0f, 1.0f };    // diffuse

// -------------------------------------------------------------
// �������̴����� �ȼ����̴��� �ѱ�� ������
// -------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos			: POSITION;
    float4 Color		: COLOR0;
};
// -------------------------------------------------------------
// �������
// -------------------------------------------------------------
VS_OUTPUT VS(
      float4 Pos    : POSITION,          // ������ġ��ǥ
      float3 Normal : NORMAL            // ��������
){
	VS_OUTPUT Out = (VS_OUTPUT)0;        // ��µ�����
	
	// ��ǥ��ȯ
	Out.Pos = mul(Pos, mWVP);
	
	// ������
	float3 L = -vLightDir; // ������ǥ�迡���� ��������
	float3 N = Normal;

	Out.Color = I_a * k_a					   // �ֺ���
	          + I_d * k_d * max(0, dot(N, L)); // ȯ�汤
	
	return Out;
}
// -------------------------------------------------------------
float4 PS(VS_OUTPUT In) : COLOR
{   
    return In.Color;
}

// -------------------------------------------------------------
// ��ũ��
// -------------------------------------------------------------
technique TShader
{
    pass P0
    {
        VertexShader = compile vs_1_1 VS();
        PixelShader  = compile ps_1_1 PS();
    }
}
