#pragma once
#include <wrl.h>
#include <d3d12.h>


class CommandList
{
public:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList();
};

