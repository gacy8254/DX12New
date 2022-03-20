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

	//��D3D12_RESOURCE_BARRIER���͸�ResourceBarrier������Ĭ������½����е�����Դ��ת��Ϊ��ͬ��״̬
	void TransitionResource(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _stateAfter, UINT _subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void TransitionResource(const Resource& _resource, D3D12_RESOURCE_STATES _stateAfter, UINT _subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	//�����UAV��Դ���һ����Դ����
	//�����ָ����Դ�����������UAV��Դ�����Դ����
	void UAVBarrier(const Resource* _resource = nullptr);

	void AliasBarrier(const Resource* _resourceBefore = nullptr, const Resource* _resourceAfter = nullptr);

	//���������Դ����ˢ�µ�ָ���������б���������������б�ر�֮�������������ִ��֮ǰ�µ���
	uint32_t FlushPendingResourceBarriers(CommandList& _commandList);

	//���κηǹ������Դ����ˢ�µ�ָ���������б�
	void FlushResourceBarrier(CommandList& _commandList);

	//ʹ�����յ���֪״̬����ȫ����Դ״̬ͼ
	void CommitFinalResourceStates();

	//�������б�������ʱ������ø÷���
	//ȷ�����е���Դ״̬���鶼������
	void Reset();

	//Ϊ��ȷ�������̵߳���Դ״̬һ��
	//��ˢ�¹������Դ����֮ǰ���Լ���������Դ״̬ˢ�µ�ȫ����Դ״̬ӳ��֮ǰ�������������
	static void Lock();
	//�������
	static void UnLock();
	
	//��ȫ����Դ״̬ӳ��ע����Դ�ͳ�ʼ״̬
	//ÿ�δ�������Դʱ����ִ�д˲���
	static void AddGlobalResourceState(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _states);
	//��ȫ����Դ״̬ͼ��ɾ������Դ
	static void RemoveGlobalResourceState(ID3D12Resource* _resource);

private:
	//��Դ��������
	using ResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>;

	//�洢�������Դ����
	ResourceBarriers m_PendingResourceBarriers;
	//�ɹ������Դ����
	ResourceBarriers m_ResourceBarriers;

	struct ResourceState
	{
		explicit ResourceState(D3D12_RESOURCE_STATES _state = D3D12_RESOURCE_STATE_COMMON)
			:State(_state){}

		//��������Դ��״̬
		void SetSubresourceState(UINT _subresource, D3D12_RESOURCE_STATES _state)
		{
			//���ָ��ΪD3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES�������������Դ��״̬�������ӳ��
			if (_subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
			{
				State = _state;
				SubresourceState.clear();
			}
			//�������ָ������Դ��״̬
			else
			{
				SubresourceState[_subresource] = _state;
			}
		}

		//��ȡ����Դ��״̬
		D3D12_RESOURCE_STATES GetSubresourceState(UINT _subresource) const
		{
			//�Ȳ�ѯ����Դ��״̬�����û�оͷ���������Դ��״̬
			D3D12_RESOURCE_STATES state = State;
			const auto iter = SubresourceState.find(_subresource);
			if (iter != SubresourceState.end())
			{
				state = iter->second;
			}
			return state;
		}
		
		//���ڴ洢����Դ״̬�����飬���Ϊ����˵��������Դ����ͬһ��״̬
		std::map<UINT, D3D12_RESOURCE_STATES> SubresourceState;
		D3D12_RESOURCE_STATES State;
	};

	//����Դָ��ӳ�䵽����״̬
	using ResourceStateMap = std::unordered_map<ID3D12Resource*, ResourceState>;

	//�洢���յ���Դ״̬
	ResourceStateMap m_FinalResourceState;

	//�洢��Դ��ȫ��״̬��ÿ�������б�ر�ʱ��״̬ӳ��ᱻ����
	static ResourceStateMap ms_GlobalResourceState;

	//���ڿ��̷߳�����Դ״̬
	static std::mutex ms_GlobalMutex;
	static bool ms_IsLocked;
};

