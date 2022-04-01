struct Vin
{
    float3 Pos : POSITION;
    uint id : SV_VertexID;
};

struct VertexToPixel
{
    float2 TexCoord : TEXCOORD;
    float4 position : SV_POSITION;
    
};

// The entry point for our vertex shader
VertexToPixel main(Vin IN)
{
	// Set up output
    VertexToPixel output;

	// Calculate the UV (0,0) to (2,2) via the ID
    output.TexCoord = float2(
		(IN.id << 1) & 2, // id % 2 * 2
		IN.id & 2);

	// Adjust the position based on the UV
    output.position = float4(output.TexCoord, 0.0f, 1);
    output.position.x = output.position.x * 2 - 1;
    output.position.y = output.position.y * -2 + 1;

    return output;
}