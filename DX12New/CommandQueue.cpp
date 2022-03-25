#include "CommandQueue.h"

#include "D3D12LibPCH.h"
#include"ResourceStateTracker.h"
#include "Application.h"
#include "CommandList.h"
#include "Device.h"
#include <DirectXTex.h>

CommandQueue::CommandQueue(Device& _device, D3D12_COMMAND_LIST_TYPE _type)
	: m_fenceValue(0), m_CommandListType(_type), m_bProcessInFlightCommandLists(true), m_Device(_device)
{
	auto device = m_Device.GetD3D12Device();

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = _type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
	ThrowIfFailed(device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));

	switch (_type)
	{
	case D3D12_COMMAND_LIST_TYPE_COPY:
		m_CommandQueue->SetName(L"CopyQueue");
		break;
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		m_CommandQueue->SetName(L"DirectQueue");
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		m_CommandQueue->SetName(L"ComputerQueue");
		break;
	}

	m_ProcessInFlightCommandListsThread = std::thread(&CommandQueue::ProcessInFlightCommandLists, this);
	/*m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_FenceEvent && "创建围栏事件失败");*/
}

CommandQueue::~CommandQueue()
{
	m_bProcessInFlightCommandLists = false;
	m_ProcessInFlightCommandListsThread.join();
}

std::shared_ptr<CommandList> CommandQueue::GetCommandList()
{
	std::shared_ptr<CommandList> commandList;

	//检查是否有可用的命令列表，没有就重新创建一个
	if (!m_AvaliableCommandList.Empty())
	{
		m_AvaliableCommandList.TryPop(commandList);
	}
	else
	{
		commandList = std::make_shared<CommandList>(m_CommandListType);
	}

	return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(const std::vector<std::shared_ptr<CommandList> >& _commandLists)
{
	ResourceStateTracker::Lock();

	//需要放回命令队列的命令列表
	std::vector<std::shared_ptr<CommandList>> toBeQueued;
	toBeQueued.reserve(_commandLists.size() * 2);//乘2是因为每个命令列表都有一个挂起的列表

	//生成Mips的命令队列
	std::vector<std::shared_ptr<CommandList>> generateMipsCommandLists;
	generateMipsCommandLists.reserve(_commandLists.size());

	//需要执行的命令列表
	std::vector<ID3D12CommandList*> d3d12CommandLists;
	d3d12CommandLists.reserve(_commandLists.size() * 2);//乘2是因为每个命令列表都有一个挂起的列表

	//迭代所有的命令列表
	for (auto commandList : _commandLists)
	{
		//先将挂起的命令列表关闭
		//获取挂起的资源屏障数量
		auto pendingCommadnList = GetCommandList();
		bool hasPendingBarrier = commandList->Close(*pendingCommadnList);
		pendingCommadnList->Close();

		//如果有挂起的资源屏障，就将挂起的命令列表加入容器中
		if (hasPendingBarrier)
		{
			d3d12CommandLists.push_back(pendingCommadnList->GetGraphicsCommandList().Get());
		}
		d3d12CommandLists.push_back(commandList->GetGraphicsCommandList().Get());

		toBeQueued.push_back(pendingCommadnList);
		toBeQueued.push_back(commandList);

		//判断是否有需要生成Mips的命令列表，有就加入容器
		auto generateMipsCommandList = commandList->GetGenerateMipsCommandList();
		if (generateMipsCommandList)
		{
			generateMipsCommandLists.push_back(generateMipsCommandList);
		}
	}

	//将所有的命令提交到队列中去
	UINT numCommandLists = static_cast<UINT>(d3d12CommandLists.size());
	m_CommandQueue->ExecuteCommandLists(numCommandLists, d3d12CommandLists.data());
	uint64_t fenceValue = Signal();

	ResourceStateTracker::UnLock();

	for (auto commandList : toBeQueued)
	{
		m_InFlightCommandLists.Push({ fenceValue , commandList });
	}

	//提交所有生成Mips的命令列表
	if (generateMipsCommandLists.size() > 0)
	{
		auto computeQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
		computeQueue->Wait(*this);
		computeQueue->ExecuteCommandList(generateMipsCommandLists);
	}

	return fenceValue;
}

uint64_t CommandQueue::ExecuteCommandList(std::shared_ptr<CommandList> _commandList)
{
	return ExecuteCommandList(std::vector<std::shared_ptr<CommandList>>({ _commandList }));
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
		auto event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(event && "创建围栏事件失败");

		m_Fence->SetEventOnCompletion(_fenceValue, event);
		::WaitForSingleObject(event, DWORD_MAX);

		::CloseHandle(event);
	}
}

void CommandQueue::Flush()
{
	std::unique_lock<std::mutex> lock(m_ProcessInFlightCommandListsThreadMutex);
	m_ProcessInFlightCommandListsThreadCV.wait(lock, [this] {return m_InFlightCommandLists.Empty(); });
	WaitForFenceValue(m_fenceValue);
}

void CommandQueue::Wait(const CommandQueue& _other)
{
	m_CommandQueue->Wait(_other.m_Fence.Get(), _other.m_fenceValue);
}

//返回队列
Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() const
{
	return m_CommandQueue;
}

void CommandQueue::ProcessInFlightCommandLists()
{
	std::unique_lock<std::mutex> lock(m_ProcessInFlightCommandListsThreadMutex, std::defer_lock);

	while (m_bProcessInFlightCommandLists)
	{
		CommandAllocatorEntry commandListEntry;

		lock.lock();
		while (m_InFlightCommandLists.TryPop(commandListEntry))
		{
			auto fenceValue = std::get<0>(commandListEntry);
			auto commandList = std::get<1>(commandListEntry);

			WaitForFenceValue(fenceValue);

			commandList->Reset();

			m_AvaliableCommandList.Push(commandList);
		}
		lock.unlock();
		m_ProcessInFlightCommandListsThreadCV.notify_one();

		//放弃
		std::this_thread::yield();
	}
}

