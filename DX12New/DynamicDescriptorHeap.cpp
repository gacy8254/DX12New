#include "DynamicDescriptorHeap.h"

#include "D3D12LibPCH.h"
#include "Application.h"
#include "RootSignature.h"
#include "CommandList.h"

DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptor /*= 1024*/)
	:m_DescriptorType(_type), 
	m_NumDescriptorPerHeap(_numDescriptor),
	m_DescriptorTableBitMask(0),
	m_StaleDescriptorTableBitMake(0), 
	m_CurrentCPUDescriptorHandle(D3D12_DEFAULT),
	m_CurrentGPUDescriptorHandle(D3D12_DEFAULT),
	m_NumFreeHandles(0)
{
	m_DescriptorHandleIncrementSize = Application::Get().GetDescriptorHandleIncrementSize(_type);

	//根据可以复制到GPU描述符堆的最大描述符数量创建描述符句柄缓存
	m_DescriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_NumDescriptorPerHeap);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{

}

void DynamicDescriptorHeap::StageDescriptor(uint32_t _rootParameterIndex, uint32_t _offset, uint32_t _numDescriptor, const D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle)
{
	//确保没有复制超过描述符堆容量的描述符，或者尝试在无效的索引处设置描述符
	if (_numDescriptor > m_NumDescriptorPerHeap || _rootParameterIndex >= MaxDescriptorTables)
	{
		throw std::bad_alloc();
	}

	DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[_rootParameterIndex];

	//确保没有超出描述符表的最大容量
	if ((_offset + _numDescriptor) > descriptorTableCache.NumDescriptor )
	{
		throw std::length_error("描述符的数量超出了描述符表的最大容量");
	}

	//检索偏移到特定偏移量的指针
	D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (descriptorTableCache.BaseDescriptor + _offset);
	//将描述符句柄复制到描述符句柄缓存中
	for (uint32_t i = 0; i < _numDescriptor; i++)
	{
		dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cpuHandle, i, m_DescriptorHandleIncrementSize);
	}

	//设置掩码值，确保在调用CommitStagedDescriptors方法时将描述符提交到GPU中
	m_StaleDescriptorTableBitMake |= (1 << _rootParameterIndex);
}

void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& _commandList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> _setFunc)
{
	//计算有多少描述符需要提交
	uint32_t numDescriptorToCommit = ComputerStaleDescriptorCount();

	if (numDescriptorToCommit > 0)
	{
		auto device = Application::Get().GetDevice();
		auto commandList = _commandList.GetGraphicsCommandList().Get();

		assert(commandList != nullptr);

		//如果当前的描述符句柄为空，或者空闲的描述符数量不满足要求就重新请求一个新的描述符堆
		if (!m_CurrentDescriptorHeap || m_NumFreeHandles < numDescriptorToCommit)
		{
			m_CurrentDescriptorHeap = RequestDescriptorHeap();
			//重置句柄的地址
			m_CurrentCPUDescriptorHandle = m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_CurrentGPUDescriptorHandle = m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			//重置可用描述符的数量
			m_NumFreeHandles = m_NumDescriptorPerHeap;

			//通过列表设置描述符堆
			_commandList.SetDescriptorHeap(m_DescriptorType, m_CurrentDescriptorHeap.Get());
			//确保可将所有暂存描述符复制到新的描述符堆中
			m_StaleDescriptorTableBitMake = m_DescriptorTableBitMask;
		}

		DWORD rootIndex;
		//迭代所有需要提交的废弃描述符表
		while (_BitScanForward(&rootIndex, m_StaleDescriptorTableBitMake))
		{
			UINT numSrcDescriptor = m_DescriptorTableCache[rootIndex].NumDescriptor;
			D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = m_DescriptorTableCache[rootIndex].BaseDescriptor;

			//在将目标复制到GPU堆之前，需要一个包含目标描述符句柄的数组，和一个描述符范围的数组
			D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStars[] = { m_CurrentCPUDescriptorHandle };
			UINT pDestDescriptorRangeSize[] = { numSrcDescriptor };

			//复制描述符
			device->CopyDescriptors(1,			 //要复制的目标描述符范围的数量
				pDestDescriptorRangeStars,		 //要复制到的D3D12_CPU_DESCRIPTOR_HANDLE数组
				pDestDescriptorRangeSize,		 //要复制到的描述符范围大小的数组
				numSrcDescriptor, 				 //复制来源的描述符数量
				pSrcDescriptorHandles, 			 //复制来源的描述符数组
				nullptr, 						 //
				m_DescriptorType);				 //堆的类型

			//使用传递过来的设置函数，在列表中设置GPU可见描述符
			_setFunc(commandList, rootIndex, m_CurrentGPUDescriptorHandle);

			//更新当前描述符的偏移量和可用数量
			m_CurrentCPUDescriptorHandle.Offset(numSrcDescriptor, m_DescriptorHandleIncrementSize);
			m_CurrentGPUDescriptorHandle.Offset(numSrcDescriptor, m_DescriptorHandleIncrementSize);
			m_NumFreeHandles -= numSrcDescriptor;

			//确保当前描述符不会再次被复制，翻转掩码
			m_StaleDescriptorTableBitMake ^= (1 << rootIndex);
		}
	}
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(CommandList& _commandList)
{
	CommitStagedDescriptors(_commandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(CommandList& _commandList)
{
	CommitStagedDescriptors(_commandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::CopyDescriptor(CommandList& _commandList, D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle)
{
	//如果当前的描述符句柄为空，或者空闲的描述符数量不满足要求就重新请求一个新的描述符堆
	if (!m_CurrentDescriptorHeap || m_NumFreeHandles < 1)
	{
		m_CurrentDescriptorHeap = RequestDescriptorHeap();
		m_CurrentCPUDescriptorHandle = m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_CurrentGPUDescriptorHandle = m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_NumFreeHandles = m_NumDescriptorPerHeap;

		_commandList.SetDescriptorHeap(m_DescriptorType, m_CurrentDescriptorHeap.Get());

		//当描述符堆更新在命令列表上时，所有的描述符表必须重新复制到新的描述符堆中
		m_StaleDescriptorTableBitMake = m_DescriptorTableBitMask;
	}

	auto device = Application::Get().GetDevice();

	D3D12_GPU_DESCRIPTOR_HANDLE hGPU = m_CurrentGPUDescriptorHandle;
	device->CopyDescriptorsSimple(1,	 //要复制的描述符数量
		m_CurrentCPUDescriptorHandle, 	 //要复制到的句柄
		_cpuHandle, 					 //复制来源句柄
		m_DescriptorType);				 //类型

	//更新当前描述符的偏移量和可用数量
	m_CurrentCPUDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
	m_CurrentGPUDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
	m_NumFreeHandles -= 1;

	return hGPU;
}

void DynamicDescriptorHeap::ParseRootSignature(const dx12lib::RootSignature& _rootSignature)
{
	m_StaleDescriptorTableBitMake = 0;

	const auto& rootSignatureDesc = _rootSignature.GetRootSignatureDesc();

	//根据描述符类型获取根签名中的描述符表掩码值
	m_DescriptorTableBitMask = _rootSignature.GteDescriptorTableBitMask(m_DescriptorType);
	uint32_t descriptorTableBitMask = m_DescriptorTableBitMask;

	uint32_t currentOffset = 0;
	DWORD rootIndex;
	
	while (_BitScanForward(&rootIndex, descriptorTableBitMask) && rootIndex < rootSignatureDesc.NumParameters)
	{
		//获取描述符的数量
		uint32_t numDescriptors = _rootSignature.GetNumDescriptors(rootIndex);

		//更新描述符表缓存中对应的描述符表
		DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[rootIndex];
		descriptorTableCache.NumDescriptor = numDescriptors;
		descriptorTableCache.BaseDescriptor = m_DescriptorHandleCache.get() + currentOffset;
		
		//更新偏移量
		currentOffset += numDescriptors;

		descriptorTableBitMask ^= (1 << rootIndex);
	}

	assert(currentOffset <= m_NumDescriptorPerHeap && "根签名需要的描述符数量超过了每个描述符堆的最大数值. 应该增加每个堆的最大描述符数量"); 
}

void DynamicDescriptorHeap::Reset()
{
	m_AvaliableDescriptorHeaps = m_DescriptorHeapPool;
	m_CurrentDescriptorHeap.Reset();
	m_CurrentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	m_CurrentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	m_NumFreeHandles = 0;
	m_DescriptorTableBitMask = 0;
	m_StaleDescriptorTableBitMake = 0;

	for (int i = 0; i < MaxDescriptorTables; ++i)
	{
		m_DescriptorTableCache[i].Reset();
	}
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DynamicDescriptorHeap::RequestDescriptorHeap()
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	//查看是否有可用的描述符堆，如果有就返回，没有就重新创建一个再返回
	if (!m_AvaliableDescriptorHeaps.empty())
	{
		descriptorHeap = m_AvaliableDescriptorHeaps.front();
		m_AvaliableDescriptorHeaps.pop();
	}
	else
	{
		descriptorHeap = CreateDescriptorHeap();
		m_DescriptorHeapPool.push(descriptorHeap);
	}

	return descriptorHeap;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DynamicDescriptorHeap::CreateDescriptorHeap()
{
	//创建一个新的描述符堆并返回
	auto device = Application::Get().GetDevice();

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = m_DescriptorType;
	desc.NumDescriptors = m_NumDescriptorPerHeap;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	return heap;
}

uint32_t DynamicDescriptorHeap::ComputerStaleDescriptorCount() const
{
	uint32_t numStaleDescriptors = 0;
	DWORD i;
	DWORD staleDescriptorBitMask = m_StaleDescriptorTableBitMake;

	while (_BitScanForward(&i, staleDescriptorBitMask))
	{
		numStaleDescriptors += m_DescriptorTableCache[i].NumDescriptor;
		staleDescriptorBitMask ^= (1 << i);
	}

	return numStaleDescriptors;
}
