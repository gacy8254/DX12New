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

    //加载演示所需的内容
    virtual bool LoadContent() override;

    //卸载加载的内容
    virtual void UnLoadContent() override;

	//更新游戏逻辑
	virtual void OnUpdate(UpdateEventArgs& _e)override;

	//进行渲染
	virtual void OnRender(RenderEventArgs& _e)override;

	//重写输入函数
	virtual void OnKeyPressed(KeyEventArgs& _e)override;
	virtual void OnKeyReleased(KeyEventArgs& _e)override;
	virtual void OnMouseMoved(MouseMotionEventArgs& _e)override;
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& _e)override;
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& _e)override;
	virtual void OnMouseWheel(MouseWheelEventArgs& _e)override;
	virtual void OnResize(ResizeEventArgs& _e)override;

	virtual void OnWindowDestroy()override;

private:
	//转换资源
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> _resource,
		D3D12_RESOURCE_STATES _before, D3D12_RESOURCE_STATES _after);

	//清空RTV
	void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE _handle,
		FLOAT* _color);

	//清空DSV
	void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE _handle,
		FLOAT _depth = 1.0f);

	//创建一个GPU缓冲
	//需要一个中间上传缓冲区，该缓冲区需要驻留，知道命令列表完成资源上传
	//在资源完全上传到目标资源之前，无法销毁此函数返回的指针
	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList,
		ID3D12Resource** _pDestinationResource,
		ID3D12Resource** _pIntermediateResource,
		size_t _numElements,
		size_t _elementSize,
		const void* _bufferData,
		D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE);

	//重置深度缓冲的尺寸
	void ResizeDepthBuffer(int _width, int _height);


	//围栏值
	uint64_t m_FenceValues[Window::m_BufferCount] = {};

	//顶点缓冲和索引缓冲
	Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	//深度缓冲和堆
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap = nullptr;

	//根签名和PSO
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO = nullptr;

	//视口设置
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_Rect;

	//变换矩阵
	float m_FOV;

	DirectX::XMMATRIX m_ModelMat;
	DirectX::XMMATRIX m_ViewMat;
	DirectX::XMMATRIX m_ProjMat;

	//是否加载内容
	bool m_ContentLoaded;
};

