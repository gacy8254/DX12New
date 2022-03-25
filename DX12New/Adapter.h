#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <memory>
#include <vector>
#include <string>

class Adapter
{
public:

	static std::shared_ptr<Adapter> Create(DXGI_GPU_PREFERENCE _gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, bool _useWrap = false);

	const std::wstring GetDescriptor() const { return m_Desc.Description; }

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetDXGIAdapter() const { return m_dxgiAdapter; }

protected:
	Adapter(Microsoft::WRL::ComPtr<IDXGIAdapter4> _dxgiAdapter);

private:
	DXGI_ADAPTER_DESC3 m_Desc;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_dxgiAdapter;
};

