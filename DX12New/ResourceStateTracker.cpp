#include "ResourceStateTracker.h"
#include "D3D12LibPCH.h"
#include "CommandList.h"
#include "Resource.h"


ResourceStateTracker::ResourceStateMap ResourceStateTracker::ms_GlobalResourceState;

std::mutex ResourceStateTracker::ms_GlobalMutex;

bool ResourceStateTracker::ms_IsLocked;

ResourceStateTracker::ResourceStateTracker()
{

}

ResourceStateTracker::~ResourceStateTracker()
{

}

void ResourceStateTracker::ResourceBarrier(const D3D12_RESOURCE_BARRIER& _barrier)
{
	if (_barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
	{
		const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier = _barrier.Transition;

		//首先判断是否之前已经在命令列表中使用过，如果是，那么它的状态是已知的，存储在m_FinalResourceState映射表中
		const auto iter = m_FinalResourceState.find(transitionBarrier.pResource);
		if (iter != m_FinalResourceState.end())
		{
			//获取资源当前的状态
			auto& resourceState = iter->second;

			//如果转换的是子资源的状态
			if (transitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !resourceState.SubresourceState.empty())
			{
				//迭代所有子资源，寻找处于不正确状态的子资源，并将资源屏障添加到m_ResourceBarriers中
				for (auto subresourceState : resourceState.SubresourceState)
				{
					if (transitionBarrier.StateAfter != subresourceState.second)
					{
						D3D12_RESOURCE_BARRIER newBrrier = _barrier;
						newBrrier.Transition.Subresource = subresourceState.first;
						newBrrier.Transition.StateBefore = subresourceState.second;
						m_ResourceBarriers.push_back(newBrrier);
					}
				}
			}
			//如果只转换单个子资源，或者所有子资源处于相同状态（SubresourceState为空）
			else
			{
				//资源的当前状态与请求状态不同时，添加一个资源屏障到映射表中
				auto finalState = resourceState.GetSubresourceState(transitionBarrier.Subresource);
				if (transitionBarrier.StateAfter != finalState)
				{
					D3D12_RESOURCE_BARRIER newBrrier = _barrier;
					newBrrier.Transition.StateBefore = finalState;
					m_ResourceBarriers.push_back(newBrrier);
				}
			}
		}
		//如果资源是第一次使用
		else
		{
			//将资源屏障挂起，稍后将解析之前的状态
			m_PendingResourceBarriers.push_back(_barrier);
		}
		//将资源的最终状态添加到m_FinalResourceState映射表中
		m_FinalResourceState[transitionBarrier.pResource].SetSubresourceState(transitionBarrier.Subresource, transitionBarrier.StateAfter);
	}
}

void ResourceStateTracker::TransitionResource(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _stateAfter, UINT _subResource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	if (_resource)
	{
		ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(_resource, D3D12_RESOURCE_STATE_COMMON, _stateAfter, _subResource));
	}
}

void ResourceStateTracker::TransitionResource(const Resource& _resource, D3D12_RESOURCE_STATES _stateAfter, UINT _subResource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	TransitionResource(_resource.GetResource().Get(), _stateAfter, _subResource);
}

void ResourceStateTracker::UAVBarrier(const Resource* _resource /*= nullptr*/)
{
	ID3D12Resource* pResource = _resource != nullptr ? _resource->GetResource().Get() : nullptr;

	//将UAV屏障添加到列表时，指定的资源可以为空
	//如果为空则必须先完成所有UAV操作，然后才能执行UAV操作，会导致流水线停顿，应当避免
	ResourceBarrier(CD3DX12_RESOURCE_BARRIER::UAV(pResource));
}

void ResourceStateTracker::AliasBarrier(const Resource* _resourceBefore /*= nullptr*/, const Resource* _resourceAfter /*= nullptr*/)
{
	ID3D12Resource* pResourceBefore = _resourceBefore != nullptr ? _resourceBefore->GetResource().Get() : nullptr;
	ID3D12Resource* pResourceAfter = _resourceAfter != nullptr ? _resourceAfter->GetResource().Get() : nullptr;

	//别名障碍用于将具有映射的两个资源转换到同一个堆中。这通常用于在同一堆中具有重叠映射的放置或保留资源。一个或两个资源可以为 null，这表明访问任何放置或保留的资源都可能导致别名。
	ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Aliasing(pResourceBefore, pResourceAfter));
}

uint32_t ResourceStateTracker::FlushPendingResourceBarriers(CommandList& _commandList)
{
	//为了保证全局资源状态映射的一致，对全局映射的访问必须是线程独占的
	assert(ms_IsLocked);

	//将所有挂起的资源屏障加入容器
	ResourceBarriers resourceBarriers;
	//分配足够的空间
	resourceBarriers.reserve(m_PendingResourceBarriers.size());

	//迭代所有挂起的屏障
	for (auto pendingBarrier : m_PendingResourceBarriers)
	{
		//检查屏障的类型
		if (pendingBarrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
		{
			//获取转换结构
			auto pendingTransition = pendingBarrier.Transition;

			//从全局状态容器中查找已知状态
			const auto& iter = ms_GlobalResourceState.find(pendingTransition.pResource);
			if (iter != ms_GlobalResourceState.end())
			{
				auto& resourceState = iter->second;
				//如果挂起的资源屏障正在转换所有子资源的状态，并且有一些子资源的状态不为空
				if (pendingTransition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && ~resourceState.SubresourceState.empty())
				{
					//转换所有子资源
					for (auto subresource : resourceState.SubresourceState)
					{
						if (pendingTransition.StateAfter != subresource.second)
						{
							D3D12_RESOURCE_BARRIER newBarrier = pendingBarrier;
							newBarrier.Transition.Subresource = subresource.first;
							newBarrier.Transition.StateBefore = subresource.second;
							resourceBarriers.push_back(newBarrier);
						}
					}
				}
				else
				{
					auto globalState = (iter->second).GetSubresourceState(pendingTransition.Subresource);
					if (pendingTransition.StateAfter != globalState)
					{
						pendingBarrier.Transition.StateBefore = globalState;
						resourceBarriers.push_back(pendingBarrier);
					}
				}
			}
		}
	}

	//检查屏障数组是否包含屏障，如果有就将所有资源屏障加入列表
	UINT numBarriers = static_cast<UINT>(resourceBarriers.size());
	if (numBarriers > 0)
	{
		auto commandList = _commandList.GetGraphicsCommandList();
		commandList->ResourceBarrier(numBarriers, m_ResourceBarriers.data());
		m_ResourceBarriers.clear();
	}
	//清空挂起的资源屏障数组
	m_PendingResourceBarriers.clear();

	return numBarriers;
}

void ResourceStateTracker::FlushResourceBarrier(CommandList& _commandList)
{
	//检查屏障数组是否包含屏障，如果有就将所有资源屏障加入列表
	UINT numBarriers = static_cast<UINT>(m_ResourceBarriers.size());
	if (numBarriers > 0)
	{
		auto commandList = _commandList.GetGraphicsCommandList();
		commandList->ResourceBarrier(numBarriers, m_ResourceBarriers.data());
		m_ResourceBarriers.clear();
	}
}

void ResourceStateTracker::CommitFinalResourceStates()
{

}

void ResourceStateTracker::Reset()
{

}

void ResourceStateTracker::Lock()
{

}

void ResourceStateTracker::UnLock()
{

}

void ResourceStateTracker::AddGlobalResourceState(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _states)
{

}

void ResourceStateTracker::RemoveGlobalResourceState(ID3D12Resource* _resource)
{

}


