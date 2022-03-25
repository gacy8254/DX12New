#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <condition_variable>
#include <atomic>

#include "ThreadSafeQueue.h"

class CommandList;
class Device;

class CommandQueue
{
public:
	

	//获取一个可以直接使用的命令列表，无需重置列表
	std::shared_ptr<CommandList> GetCommandList();

	//执行命令列表
	//返回一个围栏值去等待命令的完成
	uint64_t ExecuteCommandList(const std::vector<std::shared_ptr<CommandList>>& _commandList);
	uint64_t ExecuteCommandList(std::shared_ptr<CommandList> _commandList);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t _fenceValue);
	void WaitForFenceValue(uint64_t _fenceValue);
	void Flush();

	//等待其他的命令队列结束
	void Wait(const CommandQueue& _other);

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

protected:
	friend class std::default_delete<CommandQueue>;

	CommandQueue(Device& _device, D3D12_COMMAND_LIST_TYPE _type);
	virtual ~CommandQueue();


private:
	//释放命令队列上已经完成处理的所有命令列表
	void ProcessInFlightCommandLists();

	Device& m_Device;

	//保持对正在工作的命令分配器的追踪，第一个值是围栏的值，第二个是正在工作的命令列表的共享指针
	using CommandAllocatorEntry = std::tuple<uint64_t, std::shared_ptr<CommandList>>;

	ThreadSafeQueue<std::shared_ptr<CommandList>> m_AvaliableCommandList;
	ThreadSafeQueue<CommandAllocatorEntry> m_InFlightCommandLists;

	D3D12_COMMAND_LIST_TYPE m_CommandListType;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
	uint64_t m_fenceValue;


	//线程
	std::thread m_ProcessInFlightCommandListsThread;
	std::atomic_bool m_bProcessInFlightCommandLists;
	std::mutex m_ProcessInFlightCommandListsThreadMutex;
	std::condition_variable m_ProcessInFlightCommandListsThreadCV;
};

