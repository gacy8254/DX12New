
//ÿ֡����ı�ĳ�������
//struct Object
//{
//    matrix gWorld;
//    matrix gInverseTransposeWorld;
//    matrix gTexcoordTransform;
//};
//ConstantBuffer<Object> ObjectCB : register(b0);
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gPreviousWorld;
    float4x4 gInverseTransposeWorld;
    float4x4 gTexcoordTransform;
}