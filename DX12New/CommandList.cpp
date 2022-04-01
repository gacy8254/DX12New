#include "CommandList.h"
#include "Application.h"
#include "ByteAddressBuffer.h"
#include "ConstantBuffer.h"
#include "CommandQueue.h"
#include "GenerateMipsPSO.h"
#include "IndexBuffer.h"
#include "PanoToCubemapPSO.h"
#include "RenderTarget.h"
#include "Resource.h"
#include "ResourceStateTracker.h"
#include "StructuredBuffer.h"
#include "Texture.h"
#include "UploadBuffer.h"
#include "VertexBuffer.h"
#include "DynamicDescriptorHeap.h"
#include "D3D12LibPCH.h"
#include "Device.h"
#include "PipelineStateObject.h"
#include "ShaderResourceView.h"
#include "UnorderedAccessView.h"
#include "Material.h"


using namespace DirectX;

class MakeUploadBuffer : public UploadBuffer
{
public:
	MakeUploadBuffer(Device& device, size_t pageSize = _2MB)
		: UploadBuffer(device, pageSize)
	{}

	virtual ~MakeUploadBuffer() {}
};

CommandList::CommandList(Device& _device, D3D12_COMMAND_LIST_TYPE _type)
	:m_CommandListType(_type), m_Device(_device)
{
	auto device = m_Device.GetD3D12Device();

	//���������б�ͷ�����
	ThrowIfFailed(device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&m_CommandAllocator)));

	ThrowIfFailed(device->CreateCommandList(0, m_CommandListType, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));

	//�����ϴ���
	m_UploadBuffer = std::make_unique<MakeUploadBuffer>(m_Device);
	//������Դ������
	m_ResourceStateTrack = std::make_unique<ResourceStateTracker>();

	//������̬��������
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(_device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		m_DescriptorHeaps[i] = nullptr;
	}
}

CommandList::~CommandList()
{

}

void CommandList::TransitionBarrier(const std::shared_ptr<Resource>& _resource, 
	D3D12_RESOURCE_STATES _state, 
	UINT _subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/, 
	bool _flushBarriers /*= false*/)
{
	if (_resource)
	{
		TransitionBarrier(_resource->GetResource(), _state, _subresource, _flushBarriers);
	}
	
}

void CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, 
	D3D12_RESOURCE_STATES _state, 
	UINT _subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/, 
	bool _flushBarriers /*= false*/)
{
	//��ȡ��Դ
	//����һ����Դ����
	//������Դ����׷������д���
	if (_resource)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_resource.Get(), D3D12_RESOURCE_STATE_COMMON, _state, _subresource);
		m_ResourceStateTrack->ResourceBarrier(barrier);
	}

	//�����Դ������Ҫ�ύ�������б�,��ˢ����Դ����
	//�÷�������ResourceStateTracker::FlushResourceBarriers����
	if (_flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::UAVBarrier(const std::shared_ptr<Resource>& _resource, bool _flushBarriers /*= false*/)
{
	auto resource = _resource ? _resource->GetResource() : nullptr;
	UAVBarrier(resource, _flushBarriers);
}

void CommandList::UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, bool _flushBarriers /*= false*/)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(_resource.Get());

	m_ResourceStateTrack->ResourceBarrier(barrier);

	if (_flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::AliasingBarrier(const Resource& _beforeResource, const Resource& _afterResource, bool _flushBarriers /*= false*/)
{
	AliasingBarrier(_beforeResource.GetResource(), _afterResource.GetResource(), _flushBarriers);
}

void CommandList::AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _beforeResource, Microsoft::WRL::ComPtr<ID3D12Resource> _afterResource, bool _flushBarriers /*= false*/)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(_beforeResource.Get(), _afterResource.Get());

	m_ResourceStateTrack->ResourceBarrier(barrier);

	if (_flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::FlushResourceBarriers()
{
	m_ResourceStateTrack->FlushResourceBarrier(*this);
}

void CommandList::CopyResource(const std::shared_ptr<Resource>& _dstRes, const std::shared_ptr<Resource>& _srcRes)
{
	assert(_dstRes && _srcRes);

	CopyResource(_dstRes->GetResource(), _srcRes->GetResource());
}

void CommandList::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> _dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> _srcRes)
{
	assert(_dstRes);
	assert(_srcRes);
	auto d = _srcRes->GetDesc();
	TransitionBarrier(_dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(_srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

	FlushResourceBarriers();

	m_CommandList->CopyResource(_dstRes.Get(), _srcRes.Get());

	TrackResource(_dstRes);
	TrackResource(_srcRes);
}

void CommandList::ResolveSubresource(const std::shared_ptr<Resource>& _detRes, const std::shared_ptr<Resource>& _srcRes, uint32_t _detSubresource /*= 0*/, uint32_t _srcSubresource /*= 0*/)
{
	assert(_detRes && _srcRes);

	TransitionBarrier(_detRes, D3D12_RESOURCE_STATE_RESOLVE_DEST, _detSubresource);
	TransitionBarrier(_srcRes, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, _srcSubresource);

	FlushResourceBarriers();

	m_CommandList->ResolveSubresource(_detRes->GetResource().Get(), _detSubresource, _srcRes->GetResource().Get(), _srcSubresource, _detRes->GetResourceDesc().Format);

	TrackResource(_detRes);
	TrackResource(_srcRes);
}

std::shared_ptr<VertexBuffer> CommandList::CopyVertexBuffer(size_t _numVertices, size_t _vertexStride, const void* _vertexBufferData)
{
	auto resource = CopyBuffer(_numVertices * _vertexStride, _vertexBufferData);
	std::shared_ptr<VertexBuffer> buffer = m_Device.CreateVertexBuffer(resource, _numVertices, _vertexStride);

	return buffer;
}

std::shared_ptr<IndexBuffer> CommandList::CopyIndexBuffer(size_t _numIndicies, DXGI_FORMAT _indexFormat, const void* _indexBufferData)
{
	size_t indexSizeInBytes = _indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	auto resource = CopyBuffer(_numIndicies * indexSizeInBytes, _indexBufferData);
	std::shared_ptr<IndexBuffer> buffer = m_Device.CreateIndexBuffer(resource, _numIndicies, _indexFormat);

	return buffer;
}

std::shared_ptr<ByteAddressBuffer> CommandList::CopyByteAddressBuffer(size_t _bufferSize, const void* _BufferData)
{
	auto resource = CopyBuffer(_bufferSize, _BufferData, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	std::shared_ptr<ByteAddressBuffer> byteAdressBuffer = m_Device.CreateByteAddressBuffer(resource);

	return byteAdressBuffer;
}

std::shared_ptr<StructuredBuffer> CommandList::CopyStructuredBuffer(size_t _numElements, size_t _elementSize, const void* _BufferData)
{
	size_t bufferSize = _elementSize * _numElements;
	auto resource = CopyBuffer(bufferSize, _BufferData, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	std::shared_ptr<StructuredBuffer> structuredBuffer = m_Device.CreateStructuredBuffer(resource, _numElements, _elementSize);

	return structuredBuffer;
}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY _primitiveTopology)
{
	m_CommandList->IASetPrimitiveTopology(_primitiveTopology);
}

std::shared_ptr<Texture> CommandList::LoadTextureFromFile(const std::wstring& _fileName, bool _sRGB)
{
	auto device = m_Device.GetD3D12Device();
	std::shared_ptr<Texture> _texture;

	fs::path filePath(_fileName);
	if (!fs::exists(filePath))
	{
		throw std::exception("�ļ�������");
	}

	std::lock_guard<std::mutex> lock(ms_TextureCacheMutex);
	//�ж���ͼ�Ƿ��Ѿ����ع�������Ѿ����ع��͸�����ͼ������
	auto iter = ms_TextureCache.find(_fileName);
	if (iter != ms_TextureCache.end())
	{
		_texture = m_Device.CreateTexture(iter->second);
	}
	else
	{
		//û�м��ع�
		TexMetadata metadata;
		ScratchImage scratchImage;

		//�����ļ���ʽ��ȡ��ͼ����
		if (filePath.extension() == ".dds")
		{
			ThrowIfFailed(LoadFromDDSFile(_fileName.c_str(), DDS_FLAGS_NONE, &metadata, scratchImage));
		}
		else if (filePath.extension() == ".hdr")
		{
			ThrowIfFailed(LoadFromHDRFile(_fileName.c_str(), &metadata, scratchImage));
		}
		else if (filePath.extension() == ".tga")
		{
			ThrowIfFailed(LoadFromTGAFile(_fileName.c_str(), &metadata, scratchImage));
		}
		else
		{
			ThrowIfFailed(LoadFromWICFile(_fileName.c_str(), WIC_FLAGS_NONE, &metadata, scratchImage));
		}

		//�������ɫ��ͼ����ΪsRGB
		if (_sRGB)
		{
			metadata.format = MakeSRGB(metadata.format);
		}

		//�ж���ͼά��
		D3D12_RESOURCE_DESC desc = {};
		switch (metadata.dimension)
		{
		case TEX_DIMENSION_TEXTURE1D:
			desc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT16>(metadata.arraySize));
			break;
		case TEX_DIMENSION_TEXTURE2D:
			desc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.arraySize));
			break;
		case TEX_DIMENSION_TEXTURE3D:
			desc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.depth));
			break;
		default:
			throw std::exception("��Ч����ͼ��ʽ.");
			break;
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

		//������Դ
		auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(
			&p,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&textureResource)));

		_texture = m_Device.CreateTexture(textureResource);
		_texture->SetName(_fileName);

		//���µ�ȫ����Դ״̬׷������
		ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);

		//�������������Դ������Mipmaps
		std::vector<D3D12_SUBRESOURCE_DATA> subResources(scratchImage.GetImageCount());
		const Image* pImages = scratchImage.GetImages();
		for (int i = 0; i < scratchImage.GetImageCount(); ++i)
		{
			auto& subResource = subResources[i];
			subResource.RowPitch = pImages[i].rowPitch;
			subResource.SlicePitch = pImages[i].slicePitch;
			subResource.pData = pImages[i].pixels;
		}

		CopyTextureSubresource(_texture, 0, static_cast<uint32_t>(subResources.size()), subResources.data());

		//����ҵ�������Դ������С�������Mips��������Ӧ������mipmaps
		if (subResources.size() < textureResource->GetDesc().MipLevels)
		{
			GenerateMips(_texture);
		}

		//����ͼ���뵽��ͼ������
		ms_TextureCache[_fileName] = textureResource.Get();
	}

	return _texture;
}

std::shared_ptr<Scene> CommandList::LoadSceneFromFile(const std::wstring& _fileName, const std::function<bool(float)>& _loadingProgress)
{
	std::shared_ptr<Scene> scene = std::make_shared<Scene>();

	if (scene->LoadSceneFromFile(*this, _fileName, _loadingProgress))
	{
		return scene;
	}

	return nullptr;
}

std::shared_ptr<Scene> CommandList::LoadSceneFromString(const std::string& _sceneString, const std::string& _format)
{
	auto scene = std::make_shared<Scene>();

	scene->LoadSceneFromString(*this, _sceneString, _format);

	return scene;
}

void CommandList::CopyTextureSubresource(const std::shared_ptr<Texture>& _texture, uint32_t _firstSubresource, uint32_t _numSubresource, D3D12_SUBRESOURCE_DATA* _subresourceData)
{
	assert(_texture);

	auto device = m_Device.GetD3D12Device();
	//GPU�е�Ŀ���ַ
	auto destinationResource = _texture->GetResource();

	if (destinationResource)
	{
		//����Դ״̬ת���ɸ���Ŀ�꣬��ˢ��״̬
		TransitionBarrier(_texture, D3D12_RESOURCE_STATE_COPY_DEST);
		FlushResourceBarriers();

		//������Ҫ���Ŀռ���������Դ
		uint64_t requiredSize = GetRequiredIntermediateSize(destinationResource.Get(), _firstSubresource, _numSubresource);

		//����һ���м仺���������ϴ�����
		ComPtr<ID3D12Resource> intermediateResource;
		auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto o = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&p,
			D3D12_HEAP_FLAG_NONE,
			&o,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)));

		//���������ݸ��Ƶ�Ŀ������
		UpdateSubresources(m_CommandList.Get(), destinationResource.Get(), intermediateResource.Get(), 0, _firstSubresource, _numSubresource, _subresourceData);

		TrackResource(intermediateResource);
		TrackResource(destinationResource);
	}
}

void CommandList::ClearTexture(const std::shared_ptr<Texture>& _texture, const float _clearColor[4])
{
	assert(_texture);

	TransitionBarrier(_texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	m_CommandList->ClearRenderTargetView(_texture->GetRenderTargetView(), _clearColor, 0, nullptr);

	TrackResource(_texture);
}

void CommandList::ClearDepthStencilTexture(const std::shared_ptr<Texture>& _texture, D3D12_CLEAR_FLAGS _clearFlags, float _depth /*= 1.0f*/, uint8_t _stencil /*= 0*/)
{
	assert(_texture);

	TransitionBarrier(_texture, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	m_CommandList->ClearDepthStencilView(_texture->GetDepthStencilView(), _clearFlags, _depth, _stencil, 0, nullptr);

	TrackResource(_texture);
}

void CommandList::GenerateMips(const std::shared_ptr<Texture>& _texture)
{
	if (!_texture)
	{
		return;
	}

	auto device = m_Device.GetD3D12Device();

	//�����ǰ�����б�ʱ���������б�
	//�������������б�
	//�ڼ��������б���ִ�иò���
	if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_COPY)
	{
		if (!m_ComputerCommandList)
		{
			m_ComputerCommandList = m_Device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE).GetCommandList();
		}
		m_ComputerCommandList->GenerateMips(_texture);
		return;
	}

	//��ȡ��Դ
	auto resource = _texture->GetResource();

	//����ײ���Դ��Ч������
	if (!resource)
	{
		return;
	}
	
	//��ѯ��Դ����
	auto resDesc = resource->GetDesc();

	//���Miplevelsֻ��1�㣬ֱ�ӷ���
	if (resDesc.MipLevels == 1)
	{
		return;
	}

	//�������2d��ͼ���������������飬�����ǳ�������ͼ����ֱ�ӷ���
	if (resDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || resDesc.DepthOrArraySize != 1 || resDesc.SampleDesc.Count > 1)
	{
		throw std::exception("��2D��ͼ���������飬��������ͼ����֧������Mips");
	}

	ComPtr<ID3D12Resource> uavResource = resource;
	//������Ҫ��ԭʼ��Դ������UAV��Դʱ��ʹ��
	ComPtr<ID3D12Resource> aliasResource;

	//����Ƿ�֧��UAV���غʹ洢����
	//�����֧�֣�����һ��֧�ֵ���ʱ��Դ
	if (!_texture->CheckUAVSupport() || (resDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
	{
		auto aliasDesc = resDesc;

		//������Դ������UAV������RT�����ģ��ͼ
		aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		aliasDesc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		//��ȡ���Ϲ涨��UAV��ʽ
		auto uavDesc = aliasDesc;
		uavDesc.Format = Texture::GetUAVCompatableFormat(resDesc.Format);

		D3D12_RESOURCE_DESC resourceDescs[] = {
		aliasDesc,
		uavDesc
		};

		//��ѯ����ѵĴ�С�����ֽ�Ϊ��λ��
		auto allocationInfo = device->GetResourceAllocationInfo(0, _countof(resourceDescs), resourceDescs);

		//�����ѵ�����
		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.SizeInBytes = allocationInfo.SizeInBytes;
		heapDesc.Alignment = allocationInfo.Alignment;
		heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
		heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

		//������
		ComPtr<ID3D12Heap> heap;
		ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));

		//׷����Դ��ȷ����Դ�����������ִ��֮ǰ�����ͷ�
		TrackResource(heap);

		//������ԭʼ��Դ����ƥ��ķ�����Դ
		//���ڽ�ԭʼ��Դ���Ƶ�����UAV����Դ
		ThrowIfFailed(device->CreatePlacedResource(heap.Get(),
			0,
			&aliasDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&aliasResource)));

		//׷����Դ״̬
		ResourceStateTracker::AddGlobalResourceState(aliasResource.Get(), D3D12_RESOURCE_STATE_COMMON);

		//׷����Դ
		TrackResource(aliasResource);

		//����һ������UAV����Դ���ͱ�����Դ��ͬһ������
		ThrowIfFailed(device->CreatePlacedResource(heap.Get(),
			0,
			&uavDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&uavResource)));

		//׷����Դ״̬
		ResourceStateTracker::AddGlobalResourceState(uavResource.Get(), D3D12_RESOURCE_STATE_COMMON);

		//׷����Դ
		TrackResource(uavResource);

		//���һ���������ϣ�ʹ������Դ���ڻ״̬
		AliasingBarrier(nullptr, aliasResource);

		//��ԭʼ��Դ������������Դ
		CopyResource(aliasResource, resource);

		//��UAV��Դ����Ϊ���Դ
		AliasingBarrier(aliasResource, uavResource);
	}

	//ʹ��һ��UAV���ݵ���Դ����Mips
	auto t = m_Device.CreateTexture(uavResource);
	GenerateMips_UAV(t, Texture::IsSRGBFormat(resDesc.Format));

	if (aliasResource)
	{
		//��������Դת��Ϊ�״̬
		AliasingBarrier(uavResource, aliasResource);

		//����Դ���Ƶ�ԭʼ��Դ��
		CopyResource(resource, aliasResource);
	}
}

void CommandList::PanoToCubeMap(const std::shared_ptr<Texture>& _cubeMap, const std::shared_ptr<Texture>& _pano)
{
	assert(_cubeMap && _pano);
	//�����ǰ�����б�ʱ���������б�
	//�������������б�
	//�ڼ��������б���ִ�иò���
	if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_COPY)
	{
		if (!m_ComputerCommandList)
		{
			m_ComputerCommandList = m_Device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE).GetCommandList();
		}
		m_ComputerCommandList->PanoToCubeMap(_cubeMap, _pano);
		return;
	}

	if (!m_PanoToCubemapPSO)
	{
		m_PanoToCubemapPSO = std::make_unique<PanoToCubemapPSO>(m_Device);
	}

	auto cubemapResource = _cubeMap->GetResource();
	if (!cubemapResource)
	{
		return;
	}

	CD3DX12_RESOURCE_DESC cubemapDesc(cubemapResource->GetDesc());

	auto stagingResource = cubemapResource;
	auto stagingTexture = m_Device.CreateTexture(stagingResource);
	//����������Դ��֧��UAV���ʣ�������һ���������ɵ��м���Դ
	if ((cubemapDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
	{
		//��ȡ������Դ���������޸ĸ�ʽ�ͱ�־λUAV֧�ֵĸ�ʽ
		auto stragingDesc = cubemapDesc;
		stragingDesc.Format = Texture::GetUAVCompatableFormat(cubemapDesc.Format);
		stragingDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		auto device = m_Device.GetD3D12Device();

		//������Դ
		auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(
			&p,
			D3D12_HEAP_FLAG_NONE,
			&stragingDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&stagingResource)));

		//׷����Դ״̬
		ResourceStateTracker::AddGlobalResourceState(stagingResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);

		//������ͼ
		stagingTexture = m_Device.CreateTexture(stagingResource);
		stagingTexture->SetName(L"PanoToCubemap");

		//��ԭ��ͼ����Դ��������ʱ��Դ
		CopyResource(stagingTexture, _cubeMap);
	}

	//ת����Դ״̬
	TransitionBarrier(stagingTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//����PSO�͸�ǩ��
	SetPipelineState(m_PanoToCubemapPSO->GetPSO());
	SetComputerRootSignature(m_PanoToCubemapPSO->GetRootSignature());

	//��ɫ������ĳ�������
	PanoToCubemapCB panoToCubemapCB;

	//uav��������2D����
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = Texture::GetUAVCompatableFormat(cubemapDesc.Format);
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	uavDesc.Texture2DArray.FirstArraySlice = 0;
	uavDesc.Texture2DArray.ArraySize = 6;

	auto srv = m_Device.CreateShaderResourceView(_pano);
	SetShaderResourceView(PanoToCubemapRS::SrcTexture, 0, srv, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//��ʼ����
	for (uint32_t mipSlics = 0; mipSlics < cubemapDesc.MipLevels;)
	{
		//��ȡ��Ҫ���ɵ�MIP����
		uint32_t numMips = std::min<uint32_t>(5, cubemapDesc.MipLevels - mipSlics);
		
		//���ó�������
		panoToCubemapCB.FirstMip = mipSlics;
		panoToCubemapCB.CubemapSize = std::max<uint32_t>(static_cast<uint32_t>(cubemapDesc.Width), cubemapDesc.Height) >> mipSlics;
		panoToCubemapCB.NumMips = numMips;
		SetComputer32BitConstants(PanoToCubemapRS::PanoToCubemapCB, panoToCubemapCB);

		//����UAV
		for (uint32_t mip = 0; mip < numMips; ++mip)
		{
			uavDesc.Texture2DArray.MipSlice = mipSlics + mip;

			auto uav = m_Device.CreateUnorderedAccessView(stagingTexture, nullptr, &uavDesc);

			SetUnorderedAccessView(PanoToCubemapRS::DstMips, mip, uav, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, 0);
		}

		//�����Ҫ���ɵ�mip����С�������ʹ��Ĭ����Դ���
		if (numMips < 5)
		{
			m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptor(
				PanoToCubemapRS::DstMips, 
				panoToCubemapCB.NumMips, 
				5 - numMips,
				m_PanoToCubemapPSO->GetDefaultUAV());
		}

		//ִ�м�����ɫ��
		Dispatch(Math::DivideByMultiple(panoToCubemapCB.CubemapSize, 16), Math::DivideByMultiple(panoToCubemapCB.CubemapSize, 16), 6);

		//���õ�ǰ��mip�������Ա���һ�ε���
		mipSlics += numMips;
	}

	//�����ʱ��Դ�����ڴ������Դ������Դ������ȥ
	if (stagingResource != cubemapResource)
	{
		CopyResource(_cubeMap, stagingTexture);
	}

}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t _rootParameterIndex, size_t _sizeInBytes, const void* _bufferData)
{
	//�����������256λ����
	//ʹ��UploadBuffer������ڴ�
	//���ڸ��¾������ĵĳ���������
	//Allocate��������һ��ALLOCATION�ṹ��,������ָ���ϴ������ڴ��CPU��GPUָ��
	auto heapAllocation = m_UploadBuffer->Allocate(_sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	//�����ڴ�
	memcpy(heapAllocation.CPU, _bufferData, _sizeInBytes);

	//����CBV
	m_CommandList->SetGraphicsRootConstantBufferView(_rootParameterIndex, heapAllocation.GPU);
}

void CommandList::SetGraphics32BitConstants(uint32_t _rootParameterIndex, size_t _numContants, const void* _bufferData)
{
	m_CommandList->SetGraphicsRoot32BitConstants(_rootParameterIndex, _numContants, _bufferData, 0);
}

void CommandList::SetComputer32BitConstants(uint32_t _rootParameterIndex, size_t _numContants, const void* _bufferData)
{
	m_CommandList->SetComputeRoot32BitConstants(_rootParameterIndex, _numContants, _bufferData, 0);
}

void CommandList::SetDynamicVertexBuffer(uint32_t _slot, size_t _numVertices, size_t _vertexSize, const void* _vertexBufferData)
{
	//������Ҫ������ڴ��С
	size_t bufferSize = _numVertices * _vertexSize;

	//�����ڴ�
	auto heapAllocation = m_UploadBuffer->Allocate(bufferSize, _vertexSize);
	//����
	memcpy(heapAllocation.CPU, _vertexBufferData, bufferSize);

	//vbv
	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = heapAllocation.GPU;
	vbv.SizeInBytes = static_cast<UINT>(bufferSize);
	vbv.StrideInBytes = static_cast<UINT>(_vertexSize);

	m_CommandList->IASetVertexBuffers(_slot, 1, &vbv);
}

void CommandList::SetVertexBuffers(uint32_t _slot, const std::vector<std::shared_ptr<VertexBuffer>>& _vertexBufffer)
{
	std::vector<D3D12_VERTEX_BUFFER_VIEW> views;
	views.reserve(_vertexBufffer.size());

	for (auto vb :_vertexBufffer)
	{
		if (vb)
		{
			TransitionBarrier(vb, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			TrackResource(vb);

			views.push_back(vb->GetVertexBufferView());

		}
	}
	m_CommandList->IASetVertexBuffers(_slot, views.size(), views.data());
}

void CommandList::SetVertexBuffer(uint32_t _slot, const std::shared_ptr<VertexBuffer>& _vertexBufffer)
{
	SetVertexBuffers(_slot, { _vertexBufffer });
}

void CommandList::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& _indexBuffer)
{
	if (_indexBuffer)
	{
		TransitionBarrier(_indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		TrackResource(_indexBuffer);
		auto y = _indexBuffer->GetIndexBufferView();
		m_CommandList->IASetIndexBuffer(&y);
	}
}

void CommandList::SetDynamicIndexBuffer(size_t _numIndicies, DXGI_FORMAT _indexFormat, const void* _indexxBufferData)
{
	//�����С
	size_t indexSizeInByte = _indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	size_t bufferSize = _numIndicies * indexSizeInByte;

	//�����ڴ�
	auto heapAllocation = m_UploadBuffer->Allocate(bufferSize, indexSizeInByte);
	//����
	memcpy(heapAllocation.CPU, _indexxBufferData, bufferSize);

	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = heapAllocation.GPU;
	ibv.SizeInBytes = static_cast<UINT>(bufferSize);
	ibv.Format = _indexFormat;

	m_CommandList->IASetIndexBuffer(&ibv);
}

void CommandList::SetGraphicsDynamicStructuredBuffer(uint32_t _slot, size_t _numElements, size_t _elementSize, const void* _bufferData)
{
	size_t bufferSize = _numElements * _elementSize;

	//�����ڴ�
	auto heapAllocation = m_UploadBuffer->Allocate(bufferSize, _elementSize);
	//����
	memcpy(heapAllocation.CPU, _bufferData, bufferSize);

	m_CommandList->SetGraphicsRootShaderResourceView(_slot, heapAllocation.GPU);
}

void CommandList::SetViewport(const D3D12_VIEWPORT& _viewport)
{
	SetViewports({ _viewport });
}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& _viewports)
{
	assert(_viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);

	m_CommandList->RSSetViewports(static_cast<UINT>(_viewports.size()), _viewports.data());
}

void CommandList::SetScissorRect(const D3D12_RECT& _rect)
{
	SetScissorRects({ _rect });
}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& _rects)
{
	assert(_rects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);

	m_CommandList->RSSetScissorRects(static_cast<UINT>(_rects.size()), _rects.data());
}

void CommandList::SetPipelineState(const std::shared_ptr<PipelineStateObject>& _pso)
{
	assert(_pso);

	auto pso = _pso->GetD3D12PipelineState().Get();

	if (m_PSO != pso)
	{
		m_PSO = pso;
		m_CommandList->SetPipelineState(pso);
		TrackResource(pso);
	}
}

void CommandList::SetGraphicsRootSignature(const std::shared_ptr<RootSignature>& _rootSignature)
{
	assert(_rootSignature);

	auto rs = _rootSignature->GetRootSignature().Get();
	if (m_RootSignature != rs)
	{
		m_RootSignature = rs;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_DynamicDescriptorHeap[i]->ParseRootSignature(_rootSignature);
		}

		m_CommandList->SetGraphicsRootSignature(m_RootSignature);

		TrackResource(m_RootSignature);
	}
}


void CommandList::SetComputerRootSignature(const std::shared_ptr<RootSignature>& _rootSignature)
{
	assert(_rootSignature);

	auto rs = _rootSignature->GetRootSignature().Get();
	if (m_RootSignature != rs)
	{
		m_RootSignature = rs;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_DynamicDescriptorHeap[i]->ParseRootSignature(_rootSignature);
		}

		m_CommandList->SetComputeRootSignature(m_RootSignature);

		TrackResource(m_RootSignature);
	}
}
		
void CommandList::SetShaderResourceView(
	uint32_t _rootParameterIndex, 
	uint32_t _descriptorOffset, 
	const std::shared_ptr<ShaderResourceView>& _srv, 
	D3D12_RESOURCE_STATES _stateAfter /*= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE*/, 
	UINT _firstSubresource /*= 0*/, 
	UINT _numSubresources /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	assert(_srv);

	auto resource = _srv->GetResource();
	if (resource)
	{
		if (_numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
		{
			for (uint32_t i = 0; i < _numSubresources; i++)
			{
				TransitionBarrier(resource, _stateAfter, _firstSubresource + i);
			}
		}
		else
		{
			TransitionBarrier(resource, _stateAfter);
		}

		TrackResource(resource);
	}
	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptor(_rootParameterIndex, _descriptorOffset, 1, _srv->GetDescriptorHandle());
	
}

void CommandList::SetShaderResourceView(
	uint32_t _rootParameterIndex, 
	uint32_t _descriptorOffset, 
	const std::shared_ptr<Texture>& _texture, 
	D3D12_RESOURCE_STATES _stateAfter /*= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE*/, 
	UINT _firstSubresource /*= 0*/, 
	UINT _numSubresources /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	if (_texture)
	{
		if (_numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
		{
			for (uint32_t i = 0; i < _numSubresources; i++)
			{
				TransitionBarrier(_texture, _stateAfter, _firstSubresource + i);
			}
		}
		else
		{
			TransitionBarrier(_texture, _stateAfter);
		}

		TrackResource(_texture);

		m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptor(_rootParameterIndex, _descriptorOffset, 1, _texture->GetShaderResourceView());
	}
}

void CommandList::SetShaderResourceView(uint32_t _rootParameterIndex, 
	const std::shared_ptr<Buffer>& _buffer, 
	D3D12_RESOURCE_STATES _stateAfter /*= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE*/, 
	size_t _bufferOffset /*= 0*/)
{
	//��ת����Դ��״̬
	if (_buffer)
	{
		auto resource = _buffer->GetResource();
		TransitionBarrier(resource, _stateAfter);

		//�ݴ���Դ
		m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageInlineSRV(_rootParameterIndex, resource->GetGPUVirtualAddress() + _bufferOffset);

		//׷����Դ
		TrackResource(resource);
	}
}

void CommandList::SetUnorderedAccessView(uint32_t _rootParameterIndex, 
	const std::shared_ptr<Buffer>& _buffer, 
	D3D12_RESOURCE_STATES _stateAfter /*= D3D12_RESOURCE_STATE_UNORDERED_ACCESS*/, 
	size_t _bufferOffset /*= 0*/)
{
	if (_buffer)
	{
		auto d3d12Resource = _buffer->GetResource();
		TransitionBarrier(d3d12Resource, _stateAfter);

		m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageInlineUAV(_rootParameterIndex, d3d12Resource->GetGPUVirtualAddress() + _bufferOffset);

		TrackResource(_buffer);
	}
}


void CommandList::SetUnorderedAccessView(uint32_t _rootParameterIndex, 
	uint32_t _descriptorOffset, 
	const std::shared_ptr<Texture>& _texture, UINT _mip, 
	D3D12_RESOURCE_STATES _stateAfter /*= D3D12_RESOURCE_STATE_UNORDERED_ACCESS*/, 
	UINT _firstSubresource /*= 0*/, 
	UINT _numSubresources /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	if (_texture)
	{
		if (_numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
		{
			for (uint32_t i = 0; i < _numSubresources; ++i)
			{
				TransitionBarrier(_texture, _stateAfter, _firstSubresource + i);
			}
		}
		else
		{
			TransitionBarrier(_texture, _stateAfter);
		}

		TrackResource(_texture);

		m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptor(_rootParameterIndex, _descriptorOffset, 1, _texture->GetUnorderedAccessView(_mip));
	}
}

void CommandList::SetUnorderedAccessView(
	uint32_t _rootParameterIndex, 
	uint32_t _descriptorOffset, 
	const std::shared_ptr<UnorderedAccessView>& _uav, 
	D3D12_RESOURCE_STATES _stateAfter /*= D3D12_RESOURCE_STATE_UNORDERED_ACCESS*/, 
	UINT _firstSubresource /*= 0*/, 
	UINT _numSubresources /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	assert(_uav);

	auto resource = _uav->GetResource();
	if (resource)
	{
		if (_numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
		{
			for (uint32_t i = 0; i < _numSubresources; ++i)
			{
				TransitionBarrier(resource, _stateAfter, _firstSubresource + i);
			}
		}
		else
		{
			TransitionBarrier(resource, _stateAfter);
		}

		TrackResource(resource);
	}

	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptor(_rootParameterIndex, _descriptorOffset, 1, _uav->GetDescriptorHandle());
}

void CommandList::SetRenderTarget(const RenderTarget& _renderTarget)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtDescriptor;
	rtDescriptor.reserve(AttachmentPoint::NumAttachmentPoints);

	const auto& textures = _renderTarget.GetTextures();

	for (int i = 0; i < 8; i++)
	{
		auto texture = textures[i];

		if (texture)
		{
			TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
			rtDescriptor.push_back(texture->GetRenderTargetView());

			TrackResource(texture);
		}
	}

	auto depthTexture = _renderTarget.GetTexture(AttachmentPoint::DepthStencil);

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsDescriptor(D3D12_DEFAULT);
	if (depthTexture)
	{
		TransitionBarrier(depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		dsDescriptor = depthTexture->GetDepthStencilView();

		TrackResource(depthTexture);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = dsDescriptor.ptr != 0 ? &dsDescriptor : nullptr;

	m_CommandList->OMSetRenderTargets(static_cast<UINT>(rtDescriptor.size()), rtDescriptor.data(), FALSE, pDSV);
}

void CommandList::Draw(uint32_t _vertexCount, uint32_t _instanceCount /*= 1*/, uint32_t _startVertex /*= 0*/, uint32_t _startInstance /*= 0*/)
{
	//�����Դת��
	FlushResourceBarriers();

	//�ύ�����������ѵ���Դ
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	//����
	m_CommandList->DrawInstanced(_vertexCount, _instanceCount, _startVertex, _startInstance);
}

void CommandList::DrawIndexed(
	uint32_t _indexCount, 
	uint32_t _instancesCount /*= 1*/, 
	uint32_t _startIndex /*= 0*/, 
	uint32_t _baseVertex /*= 0*/, 
	uint32_t _startInstance /*= 0*/)
{
	//�����Դת��
	FlushResourceBarriers();

	//�ύ�����������ѵ���Դ
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	//����
	m_CommandList->DrawIndexedInstanced(_indexCount, _instancesCount, _startIndex, _baseVertex, _startInstance);
}

void CommandList::Dispatch(uint32_t _numGroupsX, uint32_t _numGroupsY, uint32_t _numGroupsZ /*= 1*/)
{
	FlushResourceBarriers();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
	{
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
	}

	m_CommandList->Dispatch(_numGroupsX, _numGroupsY, _numGroupsZ);
}

bool CommandList::Close(CommandList& _pendingCommandList)
{
	FlushResourceBarriers();

	m_CommandList->Close();

	//ˢ�¹������Դ����
	uint32_t numPendingBarriers = m_ResourceStateTrack->FlushPendingResourceBarriers(_pendingCommandList);
	//�ύ������Դ״̬��ȫ��״̬
	m_ResourceStateTrack->CommitFinalResourceStates();

	return numPendingBarriers > 0;
}

void CommandList::Close()
{
	FlushResourceBarriers();
	m_CommandList->Close();
}

void CommandList::Reset()
{
	ThrowIfFailed(m_CommandAllocator->Reset());
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	m_ResourceStateTrack->Reset();
	m_UploadBuffer->Reset();

	ReleaseTrackedObjects();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i]->Reset();
		m_DescriptorHeaps[i] = nullptr;
	}

	m_RootSignature = nullptr;
	m_ComputerCommandList = nullptr;
	m_PSO = nullptr;
}

void CommandList::ReleaseTrackedObjects()
{
	m_TrackedObjects.clear();
}

void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _type, ID3D12DescriptorHeap* _heap)
{
	if (m_DescriptorHeaps[_type] != _heap)
	{
		m_DescriptorHeaps[_type] = _heap;
		BindDescriptorHeaps();
	}
}

std::shared_ptr<Scene> CommandList::CreateScene(const VertexCollection& _vertices, const IndexCollection& _indicies)
{
	if (_vertices.empty())
	{
		return nullptr;
	}

	auto vertexBuffer = CopyVertexBuffer(_vertices);
	std::shared_ptr<IndexBuffer> indexBuffer = CopyIndexBuffer(_indicies);

	auto mesh = std::make_shared<Mesh>();

	auto material = std::make_shared<Material>(Material::White);

	mesh->SetVertexBuffer(0, vertexBuffer);
	mesh->SetIndexBuffer(indexBuffer);
	mesh->SetMaterial(material);

	auto node = std::make_shared<SceneNode>();
	node->AddMesh(mesh);

	auto scene = std::make_shared<Scene>();
	scene->SetRootNode(node);

	return scene;
}

void CommandList::TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> _object)
{
	m_TrackedObjects.push_back(_object);
}

void CommandList::TrackResource(const std::shared_ptr<Resource>& _res)
{
	assert(_res);
	TrackResource(_res->GetResource());
}

void CommandList::GenerateMips_UAV(const std::shared_ptr<Texture>& _texture, bool _isRGB)
{
	//���PSO�Ƿ���ڣ�û�оʹ���һ��
	if (!m_GenerateMipsPSO)
	{
		m_GenerateMipsPSO = std::make_unique<GenerateMipsPSO>(m_Device);
	}

	//����PSO�͸�ǩ��
	SetPipelineState(m_GenerateMipsPSO->GetPSO());
	SetComputerRootSignature(m_GenerateMipsPSO->GetRootSignature());

	GenerateMipsCB generateMipsCB;
	generateMipsCB.IsSRGB = _isRGB;

	auto res = _texture->GetResource();
	auto resDesc = _texture->GetResourceDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format =_isRGB ? Texture::GetSRGBFormat(resDesc.Format) : resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

	auto srv = m_Device.CreateShaderResourceView(_texture, &srvDesc);

	for (uint32_t srcMip = 0; srcMip < resDesc.MipLevels - 1u;)
	{
		//Դ��ͼ�Ŀ�ߣ�>> ������������൱�ڳ���2��mip��ά��Ҳ�����˴�PASS�ļ������ά��
		uint64_t srcWidth = resDesc.Width >> srcMip;
		uint32_t srcHeight = resDesc.Height >> srcMip;
		uint32_t dstWidth = static_cast<uint32_t>(srcWidth >> 1);
		uint32_t desHeight = srcHeight >> 1;

		//SrcDimension��һ��λ���룬����ָʾԴMIP�Ŀ�Ȼ�߶��Ƿ�Ϊ����
		generateMipsCB.SrcDimension = (srcHeight & 1) << 1 | (srcWidth & 1);

		//��ǰ�������ɵ�Mip�����������Ϊ4
		DWORD mipCount;
		//����һϵ����������㷽������ʱ������
		_BitScanForward(&mipCount, (dstWidth == 1 ? desHeight : dstWidth) | (desHeight == 1 ? dstWidth : desHeight));

		//��������ĸ�MIP
		mipCount = std::min<DWORD>(4, mipCount + 1);
		//�������ɵ�mip��ԭʼ�����е�mipcount�࣬��mipCount����Ϊʣ���mip����
		mipCount = (srcMip + mipCount) >= resDesc.MipLevels ? resDesc.MipLevels - srcMip - 1 : mipCount;

		dstWidth = std::max<DWORD>(1, dstWidth);
		desHeight = std::max<DWORD>(1, desHeight);

		generateMipsCB.SrcMipLevel = srcMip;
		generateMipsCB.NumMipLevels = mipCount;
		generateMipsCB.TexelSize.x = 1.0f / (float)dstWidth;
		generateMipsCB.TexelSize.y = 1.0f / (float)desHeight;

		//���ø���������
		SetComputer32BitConstants(GenerateMips::GenerateMipsCB, generateMipsCB);

		//����SRV
		SetShaderResourceView(GenerateMips::SrcMip, 0, srv, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srcMip, 1);

		//����UAV
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = resDesc.Format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = srcMip + mip + 1;

			auto uav = m_Device.CreateUnorderedAccessView(_texture, nullptr, &uavDesc);
			SetUnorderedAccessView(GenerateMips::OutMip, mip, uav, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srcMip + mip + 1, 1);
		}

		//������ɵ�MIPS��С��4����δʹ�õ�mip��Ӧ��ʹ��Ĭ��UAV���
		if (mipCount < 4)
		{
			m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptor(GenerateMips::OutMip, mipCount, 4 - mipCount, m_GenerateMipsPSO->GetDefaultUAV());
		}

		//ִ�м������
		//DivideByMultiple����һ������ĳ�����õ����������  ��100 / 8 = 13
		Dispatch(Math::DivideByMultiple(dstWidth, 8), Math::DivideByMultiple(desHeight, 8));

		UAVBarrier(_texture);

		//Դmip���������ѿ�ʼ��һ�ε���
		srcMip += mipCount;
	}
}

void CommandList::GenerateMips_BGR(Texture& _texture)
{

}

void CommandList::GenerateMips_sRGB(Texture& _texture)
{

}

Microsoft::WRL::ComPtr<ID3D12Resource> CommandList::CopyBuffer(size_t _bufferSzie, const void* _bufferData, D3D12_RESOURCE_FLAGS _flags /*= D3D12_RESOURCE_FLAG_NONE*/)
{
	auto device = m_Device.GetD3D12Device();

	ComPtr<ID3D12Resource> d3d12Resource;
	if (_bufferSzie == 0)
	{
		//�⽫����һ���յ���Դ��������Ҫ����һ��Ĭ�ϵ���Դ
	}
	else
	{
		//����һ��Ĭ�϶��е���Դ
		auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto o = CD3DX12_RESOURCE_DESC::Buffer(_bufferSzie, _flags);
			ThrowIfFailed(device->CreateCommittedResource(
				&p,
				D3D12_HEAP_FLAG_NONE,
				&o,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&d3d12Resource)));

		//׷����Դ״̬
		ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

		if (_bufferData != nullptr)
		{
			//����һ���ϴ����е���Դ
			ComPtr<ID3D12Resource> uploadResource;
			p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto i = CD3DX12_RESOURCE_DESC::Buffer(_bufferSzie);
			ThrowIfFailed(device->CreateCommittedResource(
				&p,
				D3D12_HEAP_FLAG_NONE,
				&i,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadResource)));

			D3D12_SUBRESOURCE_DATA subresourceDate = {};
			subresourceDate.pData = _bufferData;
			subresourceDate.RowPitch = _bufferSzie;
			subresourceDate.SlicePitch = subresourceDate.RowPitch;

			//ת����Դ״̬
			m_ResourceStateTrack->TransitionResource(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
			FlushResourceBarriers();

			//����Դ������Ŀ��
			UpdateSubresources(m_CommandList.Get(), d3d12Resource.Get(), uploadResource.Get(), 0, 0, 1, &subresourceDate);

			//ȷ����Դ�ڲ������Ǯ�����ͷ�
			TrackResource(uploadResource);
		}
		//ȷ����Դ�ڲ������Ǯ�����ͷ�
		TrackResource(d3d12Resource);
	}

	return d3d12Resource;
}

void CommandList::BindDescriptorHeaps()
{
	UINT numDescriptor = 0;
	ID3D12DescriptorHeap* desHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* descriptorHeap = m_DescriptorHeaps[i];
		if (descriptorHeap)
		{
			desHeap[numDescriptor++] = descriptorHeap;
		}
	}

	m_CommandList->SetDescriptorHeaps(numDescriptor, desHeap);
}

std::map<std::wstring, ID3D12Resource*> CommandList::ms_TextureCache;

std::mutex CommandList::ms_TextureCacheMutex;
