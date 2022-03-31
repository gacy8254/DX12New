struct GSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    matrix ModelMat : MODEL;
    matrix ModelViewPorjMat : VIEWPROJ;
};

struct GSOutput
{
    float4 PosW : SV_POSITION;
};

[maxvertexcount(2)]
void main(
	point GSInput gin[1], //传入顶点，所以顶点数组长度为1
    inout LineStream<GSOutput> lineStream
)
{
    GSInput geoOutVert[2];
    float L = 0.5f; //可视化法线长度参数
    
    geoOutVert[0].Position = gin[0].Position; //第一个点坐标
    geoOutVert[1].Position.xyz = geoOutVert[0].Position.xyz + gin[0].Normal * L; //沿着法线方向平移的第二个点坐标
    
    geoOutVert[0].Normal = gin[0].Normal;
    geoOutVert[1].Normal = geoOutVert[0].Normal;
    
    GSOutput geoOut[2];
    [unroll]
    for (uint i = 0; i < 2; i++)
    {
        //float4 PosW = mul(float4(geoOutVert[i].Position.xyz, 1.0f), gin[0].ModelMat);
        geoOut[i].PosW = mul(gin[0].ModelViewPorjMat, float4(geoOutVert[i].Position.xyz, 1.0f));
        
        lineStream.Append(geoOut[i]);
    }
}
