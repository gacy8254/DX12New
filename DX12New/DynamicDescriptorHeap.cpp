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

	//���ݿ��Ը��Ƶ�GPU�������ѵ�������������������������������
	m_DescriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_NumDescriptorPerHeap);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{

}

void DynamicDescriptorHeap::StageDescriptor(uint32_t _rootParameterIndex, uint32_t _offset, uint32_t _numDescriptor, const D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle)
{
	//ȷ��û�и��Ƴ����������������������������߳�������Ч������������������
	if (_numDescriptor > m_NumDescriptorPerHeap || _rootParameterIndex >= MaxDescriptorTables)
	{
		throw std::bad_alloc();
	}

	DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[_rootParameterIndex];

	//ȷ��û�г�������������������
	if ((_offset + _numDescriptor) > descriptorTableCache.NumDescriptor )
	{
		throw std::length_error("����������������������������������");
	}

	//����ƫ�Ƶ��ض�ƫ������ָ��
	D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (descriptorTableCache.BaseDescriptor + _offset);
	//��������������Ƶ����������������
	for (uint32_t i = 0; i < _numDescriptor; i++)
	{
		dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cpuHandle, i, m_DescriptorHandleIncrementSize);
	}

	//��������ֵ��ȷ���ڵ���CommitStagedDescriptors����ʱ���������ύ��GPU��
	m_StaleDescriptorTableBitMake |= (1 << _rootParameterIndex);
}

void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& _commandList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> _setFunc)
{
	//�����ж�����������Ҫ�ύ
	uint32_t numDescriptorToCommit = ComputerStaleDescriptorCount();

	if (numDescriptorToCommit > 0)
	{
		auto device = Application::Get().GetDevice();
		auto commandList = _commandList.GetGraphicsCommandList().Get();

		assert(commandList != nullptr);

		//�����ǰ�����������Ϊ�գ����߿��е�����������������Ҫ�����������һ���µ���������
		if (!m_CurrentDescriptorHeap || m_NumFreeHandles < numDescriptorToCommit)
		{
			m_CurrentDescriptorHeap = RequestDescriptorHeap();
			//���þ���ĵ�ַ
			m_CurrentCPUDescriptorHandle = m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_CurrentGPUDescriptorHandle = m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			//���ÿ���������������
			m_NumFreeHandles = m_NumDescriptorPerHeap;

			//ͨ���б�������������
			_commandList.SetDescriptorHeap(m_DescriptorType, m_CurrentDescriptorHeap.Get());
			//ȷ���ɽ������ݴ����������Ƶ��µ�����������
			m_StaleDescriptorTableBitMake = m_DescriptorTableBitMask;
		}

		DWORD rootIndex;
		//����������Ҫ�ύ�ķ�����������
		while (_BitScanForward(&rootIndex, m_StaleDescriptorTableBitMake))
		{
			UINT numSrcDescriptor = m_DescriptorTableCache[rootIndex].NumDescriptor;
			D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = m_DescriptorTableCache[rootIndex].BaseDescriptor;

			//�ڽ�Ŀ�긴�Ƶ�GPU��֮ǰ����Ҫһ������Ŀ����������������飬��һ����������Χ������
			D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStars[] = { m_CurrentCPUDescriptorHandle };
			UINT pDestDescriptorRangeSize[] = { numSrcDescriptor };

			//����������
			device->CopyDescriptors(1,			 //Ҫ���Ƶ�Ŀ����������Χ������
				pDestDescriptorRangeStars,		 //Ҫ���Ƶ���D3D12_CPU_DESCRIPTOR_HANDLE����
				pDestDescriptorRangeSize,		 //Ҫ���Ƶ�����������Χ��С������
				numSrcDescriptor, 				 //������Դ������������
				pSrcDescriptorHandles, 			 //������Դ������������
				nullptr, 						 //
				m_DescriptorType);				 //�ѵ�����

			//ʹ�ô��ݹ��������ú��������б�������GPU�ɼ�������
			_setFunc(commandList, rootIndex, m_CurrentGPUDescriptorHandle);

			//���µ�ǰ��������ƫ�����Ϳ�������
			m_CurrentCPUDescriptorHandle.Offset(numSrcDescriptor, m_DescriptorHandleIncrementSize);
			m_CurrentGPUDescriptorHandle.Offset(numSrcDescriptor, m_DescriptorHandleIncrementSize);
			m_NumFreeHandles -= numSrcDescriptor;

			//ȷ����ǰ�����������ٴα����ƣ���ת����
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
	//�����ǰ�����������Ϊ�գ����߿��е�����������������Ҫ�����������һ���µ���������
	if (!m_CurrentDescriptorHeap || m_NumFreeHandles < 1)
	{
		m_CurrentDescriptorHeap = RequestDescriptorHeap();
		m_CurrentCPUDescriptorHandle = m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_CurrentGPUDescriptorHandle = m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_NumFreeHandles = m_NumDescriptorPerHeap;

		_commandList.SetDescriptorHeap(m_DescriptorType, m_CurrentDescriptorHeap.Get());

		//���������Ѹ����������б���ʱ�����е���������������¸��Ƶ��µ�����������
		m_StaleDescriptorTableBitMake = m_DescriptorTableBitMask;
	}

	auto device = Application::Get().GetDevice();

	D3D12_GPU_DESCRIPTOR_HANDLE hGPU = m_CurrentGPUDescriptorHandle;
	device->CopyDescriptorsSimple(1,	 //Ҫ���Ƶ�����������
		m_CurrentCPUDescriptorHandle, 	 //Ҫ���Ƶ��ľ��
		_cpuHandle, 					 //������Դ���
		m_DescriptorType);				 //����

	//���µ�ǰ��������ƫ�����Ϳ�������
	m_CurrentCPUDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
	m_CurrentGPUDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
	m_NumFreeHandles -= 1;

	return hGPU;
}

void DynamicDescriptorHeap::ParseRootSignature(const dx12lib::RootSignature& _rootSignature)
{
	m_StaleDescriptorTableBitMake = 0;

	const auto& rootSignatureDesc = _rootSignature.GetRootSignatureDesc();

	//�������������ͻ�ȡ��ǩ���е�������������ֵ
	m_DescriptorTableBitMask = _rootSignature.GteDescriptorTableBitMask(m_DescriptorType);
	uint32_t descriptorTableBitMask = m_DescriptorTableBitMask;

	uint32_t currentOffset = 0;
	DWORD rootIndex;
	
	while (_BitScanForward(&rootIndex, descriptorTableBitMask) && rootIndex < rootSignatureDesc.NumParameters)
	{
		//��ȡ������������
		uint32_t numDescriptors = _rootSignature.GetNumDescriptors(rootIndex);

		//���������������ж�Ӧ����������
		DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[rootIndex];
		descriptorTableCache.NumDescriptor = numDescriptors;
		descriptorTableCache.BaseDescriptor = m_DescriptorHandleCache.get() + currentOffset;
		
		//����ƫ����
		currentOffset += numDescriptors;

		descriptorTableBitMask ^= (1 << rootIndex);
	}

	assert(currentOffset <= m_NumDescriptorPerHeap && "��ǩ����Ҫ������������������ÿ���������ѵ������ֵ. Ӧ������ÿ���ѵ��������������"); 
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
	//�鿴�Ƿ��п��õ��������ѣ�����оͷ��أ�û�о����´���һ���ٷ���
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
	//����һ���µ��������Ѳ�����
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
