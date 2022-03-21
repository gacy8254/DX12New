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
	assert(m_FenceEvent && "创建围栏事件失败");
}

CommandQueue::~CommandQueue()
{

}

std::shared_ptr<CommandList> CommandQueue::GetCommandList()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	std::shared_ptr<CommandList> commandList;

	//检查命令分配器队列中是否有可用的分配器，如果有就直接使用，如果没有就创建一个新的分配器
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

	//同样检查是否有可用的列表，有就直接使用，没有就创建一个
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

	//将命令分配器的指针分配给列表的私有数据来与之关联
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList)
{
	//关闭列表
	_commandList->Close();

	//获取与列表关联的分配器
	ID3D12CommandAllocator* commadnAllocator;
	UINT dataSize = sizeof(commadnAllocator);
	ThrowIfFailed(_commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commadnAllocator));

	//向队列中添加命令
	ID3D12CommandList* const commandlist[] = { _commandList.Get() };
	m_CommandQueue->ExecuteCommandLists(1, commandlist);

	//获取当前GPU的围栏值
	uint64_t femceValue = Signal();

	//将分配器和列表放到队列的队尾
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

//返回队列
Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() const
{
	return m_CommandQueue;
}

//创建分配器
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(m_Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

//创建列表
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _allocator)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
	ThrowIfFailed(m_Device->CreateCommandList(0, m_CommandListType, _allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}
