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

		//�����ж��Ƿ�֮ǰ�Ѿ��������б���ʹ�ù�������ǣ���ô����״̬����֪�ģ��洢��m_FinalResourceStateӳ�����
		const auto iter = m_FinalResourceState.find(transitionBarrier.pResource);
		if (iter != m_FinalResourceState.end())
		{
			//��ȡ��Դ��ǰ��״̬
			auto& resourceState = iter->second;

			//���ת����������Դ��״̬
			if (transitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !resourceState.SubresourceState.empty())
			{
				//������������Դ��Ѱ�Ҵ��ڲ���ȷ״̬������Դ��������Դ������ӵ�m_ResourceBarriers��
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
			//���ֻת����������Դ��������������Դ������ͬ״̬��SubresourceStateΪ�գ�
			else
			{
				//��Դ�ĵ�ǰ״̬������״̬��ͬʱ�����һ����Դ���ϵ�ӳ�����
				auto finalState = resourceState.GetSubresourceState(transitionBarrier.Subresource);
				if (transitionBarrier.StateAfter != finalState)
				{
					D3D12_RESOURCE_BARRIER newBrrier = _barrier;
					newBrrier.Transition.StateBefore = finalState;
					m_ResourceBarriers.push_back(newBrrier);
				}
			}
		}
		//�����Դ�ǵ�һ��ʹ��
		else
		{
			//����Դ���Ϲ����Ժ󽫽���֮ǰ��״̬
			m_PendingResourceBarriers.push_back(_barrier);
		}
		//����Դ������״̬��ӵ�m_FinalResourceStateӳ�����
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

	//��UAV������ӵ��б�ʱ��ָ������Դ����Ϊ��
	//���Ϊ����������������UAV������Ȼ�����ִ��UAV�������ᵼ����ˮ��ͣ�٣�Ӧ������
	ResourceBarrier(CD3DX12_RESOURCE_BARRIER::UAV(pResource));
}

void ResourceStateTracker::AliasBarrier(const Resource* _resourceBefore /*= nullptr*/, const Resource* _resourceAfter /*= nullptr*/)
{
	ID3D12Resource* pResourceBefore = _resourceBefore != nullptr ? _resourceBefore->GetResource().Get() : nullptr;
	ID3D12Resource* pResourceAfter = _resourceAfter != nullptr ? _resourceAfter->GetResource().Get() : nullptr;

	//�����ϰ����ڽ�����ӳ���������Դת����ͬһ�����С���ͨ��������ͬһ���о����ص�ӳ��ķ��û�����Դ��һ����������Դ����Ϊ null������������κη��û�������Դ�����ܵ��±�����
	ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Aliasing(pResourceBefore, pResourceAfter));
}

uint32_t ResourceStateTracker::FlushPendingResourceBarriers(CommandList& _commandList)
{
	//Ϊ�˱�֤ȫ����Դ״̬ӳ���һ�£���ȫ��ӳ��ķ��ʱ������̶߳�ռ��
	assert(ms_IsLocked);

	//�����й������Դ���ϼ�������
	ResourceBarriers resourceBarriers;
	//�����㹻�Ŀռ�
	resourceBarriers.reserve(m_PendingResourceBarriers.size());

	//�������й��������
	for (auto pendingBarrier : m_PendingResourceBarriers)
	{
		//������ϵ�����
		if (pendingBarrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
		{
			//��ȡת���ṹ
			auto pendingTransition = pendingBarrier.Transition;

			//��ȫ��״̬�����в�����֪״̬
			const auto& iter = ms_GlobalResourceState.find(pendingTransition.pResource);
			if (iter != ms_GlobalResourceState.end())
			{
				auto& resourceState = iter->second;
				//����������Դ��������ת����������Դ��״̬��������һЩ����Դ��״̬��Ϊ��
				if (pendingTransition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && ~resourceState.SubresourceState.empty())
				{
					//ת����������Դ
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

	//������������Ƿ�������ϣ�����оͽ�������Դ���ϼ����б�
	UINT numBarriers = static_cast<UINT>(resourceBarriers.size());
	if (numBarriers > 0)
	{
		auto commandList = _commandList.GetGraphicsCommandList();
		commandList->ResourceBarrier(numBarriers, m_ResourceBarriers.data());
		m_ResourceBarriers.clear();
	}
	//��չ������Դ��������
	m_PendingResourceBarriers.clear();

	return numBarriers;
}

void ResourceStateTracker::FlushResourceBarrier(CommandList& _commandList)
{
	//������������Ƿ�������ϣ�����оͽ�������Դ���ϼ����б�
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


