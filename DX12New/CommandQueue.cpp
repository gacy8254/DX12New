#include "CommandQueue.h"

#include "D3D12LibPCH.h"

CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> _device, D3D12_COMMAND_LIST_TYPE _type)
	: m_fenceValue(0), m_CommandListType(_type), m_Device(_device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = _type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(m_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
	ThrowIfFailed(m_Device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));

	m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_FenceEvent && "����Χ���¼�ʧ��");
}

CommandQueue::~CommandQueue()
{

}

std::shared_ptr<CommandList> CommandQueue::GetCommandList()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	std::shared_ptr<CommandList> commandList;

	//�������������������Ƿ��п��õķ�����������о�ֱ��ʹ�ã����û�оʹ���һ���µķ�����
	if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
	{
		commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
		m_CommandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator->Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator();
	}

	//ͬ������Ƿ��п��õ��б��о�ֱ��ʹ�ã�û�оʹ���һ��
	if (!m_CommandListQueue.empty())
	{
		commandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	//�������������ָ�������б��˽����������֮����
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList)
{
	//�ر��б�
	_commandList->Close();

	//��ȡ���б�����ķ�����
	ID3D12CommandAllocator* commadnAllocator;
	UINT dataSize = sizeof(commadnAllocator);
	ThrowIfFailed(_commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commadnAllocator));

	//��������������
	ID3D12CommandList* const commandlist[] = { _commandList.Get() };
	m_CommandQueue->ExecuteCommandLists(1, commandlist);

	//��ȡ��ǰGPU��Χ��ֵ
	uint64_t femceValue = Signal();

	//�����������б�ŵ����еĶ�β
	m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ femceValue, commadnAllocator });
	m_CommandListQueue.push(_commandList);

	commadnAllocator->Release();

	return 0;
}

uint64_t CommandQueue::Signal()
{
	uint64_t fenceValueForSignal = ++m_fenceValue;
	ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}

bool CommandQueue::IsFenceComplete(uint64_t _fenceValue)
{
	return m_Fence->GetCompletedValue() >= _fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t _fenceValue)
{
	if (!IsFenceComplete(_fenceValue))
	{
		m_Fence->SetEventOnCompletion(_fenceValue, m_FenceEvent);
		::WaitForSingleObject(m_FenceEvent, DWORD_MAX);
	}
}

void CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

//���ض���
Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() const
{
	return m_CommandQueue;
}

//����������
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(m_Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

//�����б�
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _allocator)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
	ThrowIfFailed(m_Device->CreateCommandList(0, m_CommandListType, _allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}
