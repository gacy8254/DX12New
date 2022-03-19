#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <queue>



class CommandQueue
{
public:
	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> _device, D3D12_COMMAND_LIST_TYPE _type);
	virtual ~CommandQueue();

	//获取一个可以直接使用的命令列表，无需重置列表
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

	//执行命令列表
	//返回一个围栏值去等待命令的完成
	uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t _fenceValue);
	void WaitForFenceValue(uint64_t _fenceValue);
	void Flush();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

protected:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _allocator);

private:
	//该结构体用于将分配器与围栏值关联起来
	struct CommandAllocatorEntry
	{
		uint64_t fenceValue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	};

	//起别名
	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	using CommandListQueue = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>>;

	D3D12_COMMAND_LIST_TYPE m_CommandListType;
	Microsoft::WRL::ComPtr<ID3D12Device2> m_Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
	HANDLE m_FenceEvent;
	uint64_t m_fenceValue;

	CommandAllocatorQueue m_CommandAllocatorQueue;
	CommandListQueue m_CommandListQueue;
};

