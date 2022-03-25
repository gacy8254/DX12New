#include "Adapter.h"
#include "D3D12LibPCH.h"

using namespace Microsoft::WRL;

// Provide an adapter that can be used by make_shared
class MakeAdapter : public Adapter
{
public:
	MakeAdapter(Microsoft::WRL::ComPtr<IDXGIAdapter4> _dxgiAdapter)
		: Adapter(_dxgiAdapter)
	{}

	virtual ~MakeAdapter() {}
};

std::shared_ptr<Adapter> Adapter::Create(DXGI_GPU_PREFERENCE _gpuPreference /*= DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE*/, bool _useWrap /*= false*/)
{
	std::shared_ptr<Adapter> adapter = nullptr;

	//创建DXGI工厂
	ComPtr<IDXGIFactory6> dxgiFactory6;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;
	ComPtr<IDXGIAdapter>  dxgiAdapter;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory6)));

	if (_useWrap)
	{
		//如果使用软件适配器则直接找到软件适配器返回
		ThrowIfFailed(dxgiFactory6->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter)));
		ThrowIfFailed(dxgiAdapter.As(&dxgiAdapter4));
	}
	else
	{
		//首先枚举所有的显示适配器
		//排除DXGI_ADAPTER_FLAG_SOFTWARE标志的软件适配器
		//找到兼容DX12的硬件显示适配器
		//选择显存最大的一个
		//将IDXGIAdapter1赋值给IDXGIAdapter4
		SIZE_T maxDadicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory6->EnumAdapterByGpuPreference(i, _gpuPreference, IID_PPV_ARGS(&dxgiAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
			{
				ThrowIfFailed(dxgiAdapter.As(&dxgiAdapter4));
				break;
			}
		}
	}
	if (dxgiAdapter4)
	{
		adapter = std::make_shared<MakeAdapter>(dxgiAdapter4);
	}

	return adapter;
}

Adapter::Adapter(Microsoft::WRL::ComPtr<IDXGIAdapter4> _dxgiAdapter)
	:m_Desc{0}, m_dxgiAdapter(_dxgiAdapter)
{
	if (m_dxgiAdapter)
	{
		//获取适配器描述
		ThrowIfFailed(m_dxgiAdapter->GetDesc3(&m_Desc));
	}
}
