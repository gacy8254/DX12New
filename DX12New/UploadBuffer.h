#pragma once
#include "Defines.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

class Device;

class UploadBuffer
{
public:
	//�����ϴ���Դ��GPU
	struct Allocation 
	{
		void* CPU;
		D3D12_GPU_VIRTUAL_ADDRESS GPU;
	};

	//����pageSize�����ڷ�����ڴ�ҳ�Ĵ�С
	explicit UploadBuffer(Device& _device, size_t pageSize = _2MB);

	virtual ~UploadBuffer();

	size_t GetPageSize() const { return m_PageSize; }

	//���ϴ����з����ڴ�
	//������ڴ治�ܳ����ڴ�ҳ�Ĵ�С
	//ʹ��memcpy�������Ƶĺ�������
	//�����������ݸ�ֵ��allocation�е�CPUָ��
	//�����ֱ�����Ҫ������ڴ��С�����ֽ�Ϊ��λ���� ���뷽ʽ
	Allocation Allocate(size_t _sizeInBytes, size_t _alignment);

	//�����ѷ�����ڴ�
	//ֻ�е����з��䶼���ڶ�����ִ��ʱ�ſ��Խ���
	void Reset();

private:
	struct Page
	{
		Page(Device& _device, size_t _sizeInBytes);
		~Page();

		//����Ƿ����㹻���ڴ�ȥ���䣬����������˳���ǰ�ڴ�ҳ�������µ��ڴ�ҳ
		bool HasSpace(size_t _sizeInBytes, size_t _alignment) const;

		//ʵ�ʷ����ڴ�
		//�׳� std::bad_alloc ����Ҫ������ڴ泬���޶�
		Allocation Allocate(size_t _sizeInBytes, size_t _alignment);

		//���ã���ƫ��������Ϊ0
		void Reste();

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;

		Device& m_Device;

		//����ָ��,ָ������ڴ����ʼ��ַ
		void* m_CPUPtr;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

		//������ڴ��С
		size_t m_PageSize;
		//ƫ����
		size_t m_Offset;
	};
	//ָ���ڴ�ҳ���ָ�������
	using PagePool = std::deque<std::shared_ptr<Page>>;

	//�������һ�����õ��ڴ�ҳ�����û�оʹ���һ���µĲ������ڴ�ҳ��
	std::shared_ptr<Page> RequestPage();

	Device& m_Device;

	//���е��ڴ�ҳ
	PagePool m_PagePool;
	//���õ��ڴ�ҳ
	PagePool m_AvaliablePages;
	//��ǰ���ڴ�ҳ
	std::shared_ptr<Page> m_CurrentPage;

	size_t m_PageSize;
};

