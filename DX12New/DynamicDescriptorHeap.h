#pragma once
#include "d3dx12.h"
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <queue>
#include <functional>
#include "RootSignature.h"

class RootSignature;
class CommandList;
class Device;

class DynamicDescriptorHeap
{
public:
	DynamicDescriptorHeap(Device& _device, D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptor = 1024);

	virtual ~DynamicDescriptorHeap();

	//参数描述
	//1 将要复制到的根参数的索引
	//2 复制到描述符表中的偏移量
	//3 描述符的数量
	//4 复制的起始地址
	//使用此方法仅仅将描述符句柄复制到描述符堆中，而不复制描述符的内容
	void StageDescriptor(uint32_t _rootParameterIndex, uint32_t _offset, uint32_t _numDescriptor, const D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle);

	//将描述符提交到GPU可见的描述符堆
	//不应该直接调用该方法
	void CommitStagedDescriptors(CommandList& _commandList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> _setFunc);
	//提交一个内联描述符
	void CommitInlineDescriptor(CommandList& _commandList, 
		const D3D12_GPU_VIRTUAL_ADDRESS* _bufferLocation, 
		uint32_t& _bitMask, 
		std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_VIRTUAL_ADDRESS)> _setFunc);

	//将描述符绑定到图形管线  ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
	void CommitStagedDescriptorsForDraw(CommandList& _commandList);
	//将描述符绑定到计算管线  ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
	void CommitStagedDescriptorsForDispatch(CommandList& _commandList);

	//提交一个内联CBV描述符
	void StageInlineCBV(uint32_t _rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS _bufferLocation);

	//提交一个内联UAV描述符
	void StageInlineUAV(uint32_t _rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS _bufferLocation);

	//提交一个内联SRV描述符
	void StageInlineSRV(uint32_t _rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS _bufferLocation);



	//将单个CPU可见描述符拷贝到GPU可见描述符堆
	D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(CommandList& _commandList, D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle);

	//分析根签名
	void ParseRootSignature(const std::shared_ptr<RootSignature>& _rootSignature);

	//重置描述符堆和缓存，仅当命令列表完成处理后使用
	void Reset();

private:
	//请求分配一个可用的描述符堆
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RequestDescriptorHeap();
	//创建一个新的描述符堆，当没有可用描述符堆时
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap();

	//计算需要复制到GPU描述符堆的CPU描述符数量
	uint32_t ComputerStaleDescriptorCount()const;


	Device& m_Device;

	//根签名中可以存在的描述符表的最大数量
	static const uint32_t MaxDescriptorTables = 32;

	//存储了描述符表中的描述符数量和起始地址，默认清空下是空的，表明绑定的根签名中的该条目不适用描述符表
	struct DescriptorTableCache
	{
		DescriptorTableCache(){}

		void Reset()
		{
			NumDescriptor = 0;
			BaseDescriptor = nullptr;
		}

		//这个描述符表中描述符的数量
		uint32_t NumDescriptor = 0;
		//其实地址
		D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor = nullptr;
	};

	//内联描述符
	D3D12_GPU_VIRTUAL_ADDRESS m_InlineCBV[MaxDescriptorTables];
	D3D12_GPU_VIRTUAL_ADDRESS m_InlineSRV[MaxDescriptorTables];
	D3D12_GPU_VIRTUAL_ADDRESS m_InlineUAV[MaxDescriptorTables];

	uint32_t m_StaleCBVBitMask;
	uint32_t m_StaleUAVBitMask;
	uint32_t m_StaleSRVBitMask;

	//堆的类型
	D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
	//每个堆中分配多少描述符
	uint32_t m_NumDescriptorPerHeap;
	//描述符递增的大小
	uint32_t m_DescriptorHandleIncrementSize;

	//可以缓存的描述符数量又传递给构造函数的参数决定
	std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> m_DescriptorHandleCache;

	DescriptorTableCache m_DescriptorTableCache[MaxDescriptorTables];

	//表明当前根签名中有哪些条目包含描述符表
	uint32_t m_DescriptorTableBitMask;

	//自上次提交以来修改过的描述符表
	//在下次绘制之前只需要重新复制修改过的描述符表即可
	uint32_t m_StaleDescriptorTableBitMake;

	using DescriptorHeapPool = std::queue<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>;

	//描述符堆池
	DescriptorHeapPool m_DescriptorHeapPool;
	//可用的描述符堆
	DescriptorHeapPool m_AvaliableDescriptorHeaps;

	//指向当前绑定到命令列表的描述符堆
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CurrentDescriptorHeap;
	//当前描述符堆的CPU和GPU句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_CurrentCPUDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrentGPUDescriptorHandle;

	//当前描述符堆中可用的句柄数
	uint32_t m_NumFreeHandles;
};

