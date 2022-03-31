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
	point GSInput gin[1], //���붥�㣬���Զ������鳤��Ϊ1
    inout LineStream<GSOutput> lineStream
)
{
    GSInput geoOutVert[2];
    float L = 0.5f; //���ӻ����߳��Ȳ���
    
    geoOutVert[0].Position = gin[0].Position; //��һ��������
    geoOutVert[1].Position.xyz = geoOutVert[0].Position.xyz + gin[0].Normal * L; //���ŷ��߷���ƽ�Ƶĵڶ���������
    
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
