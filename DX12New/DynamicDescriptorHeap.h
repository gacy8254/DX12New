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


class DynamicDescriptorHeap
{
public:
	DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptor = 1024);

	virtual ~DynamicDescriptorHeap();

	//��������
	//1 ��Ҫ���Ƶ��ĸ�����������
	//2 ���Ƶ����������е�ƫ����
	//3 ������������
	//4 ���Ƶ���ʼ��ַ
	//ʹ�ô˷���������������������Ƶ����������У���������������������
	void StageDescriptor(uint32_t _rootParameterIndex, uint32_t _offset, uint32_t _numDescriptor, const D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle);

	//���������ύ��GPU�ɼ�����������
	//��Ӧ��ֱ�ӵ��ø÷���
	void CommitStagedDescriptors(CommandList& _commandList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> _setFunc);
	//���������󶨵�ͼ�ι���  ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
	void CommitStagedDescriptorsForDraw(CommandList& _commandList);
	//���������󶨵��������  ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
	void CommitStagedDescriptorsForDispatch(CommandList& _commandList);

	//������CPU�ɼ�������������GPU�ɼ���������
	D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(CommandList& _commandList, D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle);

	//������ǩ��
	void ParseRootSignature(const dx12lib::RootSignature& _rootSignature);

	//�����������Ѻͻ��棬���������б���ɴ����ʹ��
	void Reset();

private:
	//�������һ�����õ���������
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RequestDescriptorHeap();
	//����һ���µ��������ѣ���û�п�����������ʱ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap();

	//������Ҫ���Ƶ�GPU�������ѵ�CPU����������
	uint32_t ComputerStaleDescriptorCount()const;

	//��ǩ���п��Դ��ڵ�����������������
	static const uint32_t MaxDescriptorTables = 32;

	//�洢�����������е���������������ʼ��ַ��Ĭ��������ǿյģ������󶨵ĸ�ǩ���еĸ���Ŀ��������������
	struct DescriptorTableCache
	{
		DescriptorTableCache(){}

		void Reset()
		{
			NumDescriptor = 0;
			BaseDescriptor = nullptr;
		}

		//�������������������������
		uint32_t NumDescriptor = 0;
		//��ʵ��ַ
		D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor = nullptr;
	};

	//�ѵ�����
	D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
	//ÿ�����з������������
	uint32_t m_NumDescriptorPerHeap;
	//�����������Ĵ�С
	uint32_t m_DescriptorHandleIncrementSize;

	//���Ի���������������ִ��ݸ����캯���Ĳ�������
	std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> m_DescriptorHandleCache;

	DescriptorTableCache m_DescriptorTableCache[MaxDescriptorTables];

	//������ǰ��ǩ��������Щ��Ŀ������������
	uint32_t m_DescriptorTableBitMask;

	//���ϴ��ύ�����޸Ĺ�����������
	//���´λ���֮ǰֻ��Ҫ���¸����޸Ĺ�������������
	uint32_t m_StaleDescriptorTableBitMake;

	using DescriptorHeapPool = std::queue<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>;

	//�������ѳ�
	DescriptorHeapPool m_DescriptorHeapPool;
	//���õ���������
	DescriptorHeapPool m_AvaliableDescriptorHeaps;

	//ָ��ǰ�󶨵������б����������
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CurrentDescriptorHeap;
	//��ǰ�������ѵ�CPU��GPU���
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_CurrentCPUDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrentGPUDescriptorHandle;

	//��ǰ���������п��õľ����
	uint32_t m_NumFreeHandles;
};

