#pragma once
#include <mutex>
#include <map>
#include <unordered_map>
#include <vector>

#include<d3d12.h>

class CommandList;
class Resource;

class ResourceStateTracker
{
public:
	ResourceStateTracker();

	virtual ~ResourceStateTracker();

	void ResourceBarrier(const D3D12_RESOURCE_BARRIER& _barrier);

	//将D3D12_RESOURCE_BARRIER发送给ResourceBarrier函数，默认情况下将所有的子资源都转换为相同的状态
	void TransitionResource(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _stateAfter, UINT _subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void TransitionResource(const Resource& _resource, D3D12_RESOURCE_STATES _stateAfter, UINT _subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	//向给定UAV资源添加一个资源屏障
	//如果不指定资源，将会给所有UAV资源添加资源屏障
	void UAVBarrier(const Resource* _resource = nullptr);

	void AliasBarrier(const Resource* _resourceBefore = nullptr, const Resource* _resourceAfter = nullptr);

	//将挂起的资源屏障刷新到指定的命令列表，这个方法在命令列表关闭之后在命令队列上执行之前呗调用
	uint32_t FlushPendingResourceBarriers(CommandList& _commandList);

	//将任何非挂起的资源屏障刷新到指定的命令列表
	void FlushResourceBarrier(CommandList& _commandList);

	//使用最终的已知状态更新全局资源状态图
	void CommitFinalResourceStates();

	//当命令列表呗重置时，会调用该方法
	//确保所有的资源状态数组都被重置
	void Reset();

	//为了确保跨多个线程的资源状态一致
	//在刷新挂起的资源屏障之前，以及在最终资源状态刷新到全局资源状态映射之前，必须进行锁定
	static void Lock();
	//解除锁定
	static void UnLock();
	
	//向全局资源状态映射注册资源和初始状态
	//每次创建新资源时都会执行此操作
	static void AddGlobalResourceState(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _states);
	//从全局资源状态图中删除该资源
	static void RemoveGlobalResourceState(ID3D12Resource* _resource);

private:
	//资源屏障数组
	using ResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>;

	//存储挂起的资源屏障
	ResourceBarriers m_PendingResourceBarriers;
	//飞挂起的资源屏障
	ResourceBarriers m_ResourceBarriers;

	struct ResourceState
	{
		explicit ResourceState(D3D12_RESOURCE_STATES _state = D3D12_RESOURCE_STATE_COMMON)
			:State(_state){}

		//设置子资源的状态
		void SetSubresourceState(UINT _subresource, D3D12_RESOURCE_STATES _state)
		{
			//如果指定为D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES，则更新整个资源的状态，并清除映射
			if (_subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
			{
				State = _state;
				SubresourceState.clear();
			}
			//否则更新指定子资源的状态
			else
			{
				SubresourceState[_subresource] = _state;
			}
		}

		//获取子资源的状态
		D3D12_RESOURCE_STATES GetSubresourceState(UINT _subresource) const
		{
			//先查询子资源的状态，如果没有就返回整个资源的状态
			D3D12_RESOURCE_STATES state = State;
			const auto iter = SubresourceState.find(_subresource);
			if (iter != SubresourceState.end())
			{
				state = iter->second;
			}
			return state;
		}
		
		//用于存储子资源状态的数组，如果为空则说明整个资源处于同一个状态
		std::map<UINT, D3D12_RESOURCE_STATES> SubresourceState;
		D3D12_RESOURCE_STATES State;
	};

	//将资源指针映射到它的状态
	using ResourceStateMap = std::unordered_map<ID3D12Resource*, ResourceState>;

	//存储最终的资源状态
	ResourceStateMap m_FinalResourceState;

	//存储资源的全局状态，每当命令列表关闭时，状态映射会被更新
	static ResourceStateMap ms_GlobalResourceState;

	//用于跨线程访问资源状态
	static std::mutex ms_GlobalMutex;
	static bool ms_IsLocked;
};

