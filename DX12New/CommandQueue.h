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
	

	//��ȡһ������ֱ��ʹ�õ������б����������б�
	std::shared_ptr<CommandList> GetCommandList();

	//ִ�������б�
	//����һ��Χ��ֵȥ�ȴ���������
	uint64_t ExecuteCommandList(const std::vector<std::shared_ptr<CommandList>>& _commandList);
	uint64_t ExecuteCommandList(std::shared_ptr<CommandList> _commandList);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t _fenceValue);
	void WaitForFenceValue(uint64_t _fenceValue);
	void Flush();

	//�ȴ�������������н���
	void Wait(const CommandQueue& _other);

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

protected:
	friend class std::default_delete<CommandQueue>;

	CommandQueue(Device& _device, D3D12_COMMAND_LIST_TYPE _type);
	virtual ~CommandQueue();


private:
	//�ͷ�����������Ѿ���ɴ�������������б�
	void ProcessInFlightCommandLists();

	Device& m_Device;

	//���ֶ����ڹ����������������׷�٣���һ��ֵ��Χ����ֵ���ڶ��������ڹ����������б�Ĺ���ָ��
	using CommandAllocatorEntry = std::tuple<uint64_t, std::shared_ptr<CommandList>>;

	ThreadSafeQueue<std::shared_ptr<CommandList>> m_AvaliableCommandList;
	ThreadSafeQueue<CommandAllocatorEntry> m_InFlightCommandLists;

	D3D12_COMMAND_LIST_TYPE m_CommandListType;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
	uint64_t m_fenceValue;


	//�߳�
	std::thread m_ProcessInFlightCommandListsThread;
	std::atomic_bool m_bProcessInFlightCommandLists;
	std::mutex m_ProcessInFlightCommandListsThreadMutex;
	std::condition_variable m_ProcessInFlightCommandListsThreadCV;
};

