#pragma once
#include "Game.h"
#include "Window.h"
#include <DirectXMath.h>

class Lesson2 :
    public Game
{
public:
    using super = Game;

    Lesson2(const std::wstring& _name, int _width, int _height, bool _vSync);

    //������ʾ���������
    virtual bool LoadContent() override;

    //ж�ؼ��ص�����
    virtual void UnLoadContent() override;

	//������Ϸ�߼�
	virtual void OnUpdate(UpdateEventArgs& _e)override;

	//������Ⱦ
	virtual void OnRender(RenderEventArgs& _e)override;

	//��д���뺯��
	virtual void OnKeyPressed(KeyEventArgs& _e)override;
	virtual void OnKeyReleased(KeyEventArgs& _e)override;
	virtual void OnMouseMoved(MouseMotionEventArgs& _e)override;
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& _e)override;
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& _e)override;
	virtual void OnMouseWheel(MouseWheelEventArgs& _e)override;
	virtual void OnResize(ResizeEventArgs& _e)override;

	virtual void OnWindowDestroy()override;

private:
	//ת����Դ
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> _resource,
		D3D12_RESOURCE_STATES _before, D3D12_RESOURCE_STATES _after);

	//���RTV
	void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE _handle,
		FLOAT* _color);

	//���DSV
	void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE _handle,
		FLOAT _depth = 1.0f);

	//����һ��GPU����
	//��Ҫһ���м��ϴ����������û�������Ҫפ����֪�������б������Դ�ϴ�
	//����Դ��ȫ�ϴ���Ŀ����Դ֮ǰ���޷����ٴ˺������ص�ָ��
	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList,
		ID3D12Resource** _pDestinationResource,
		ID3D12Resource** _pIntermediateResource,
		size_t _numElements,
		size_t _elementSize,
		const void* _bufferData,
		D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE);

	//������Ȼ���ĳߴ�
	void ResizeDepthBuffer(int _width, int _height);


	//Χ��ֵ
	uint64_t m_FenceValues[Window::m_BufferCount] = {};

	//���㻺�����������
	Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	//��Ȼ���Ͷ�
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap = nullptr;

	//��ǩ����PSO
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO = nullptr;

	//�ӿ�����
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_Rect;

	//�任����
	float m_FOV;

	DirectX::XMMATRIX m_ModelMat;
	DirectX::XMMATRIX m_ViewMat;
	DirectX::XMMATRIX m_ProjMat;

	//�Ƿ��������
	bool m_ContentLoaded;
};

