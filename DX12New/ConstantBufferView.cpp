#include "ConstantBufferView.h"
#include "Device.h"
#include "ConstantBuffer.h"
#include "D3D12LibPCH.h"

ConstantBufferView::ConstantBufferView(Device& _device, const std::shared_ptr<ConstantBuffer>& _constantBuffer, size_t _offset /*= 0*/)
	:m_Device(_device),
	m_ConstantBuffer(_constantBuffer)
{
	assert(m_ConstantBuffer);

	auto device = m_Device.GetD3D12Device();
	auto resource = m_ConstantBuffer->GetResource();

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	desc.BufferLocation = resource->GetGPUVirtualAddress() + _offset;
	//常量BUFFER必须与硬件要求对齐
	desc.SizeInBytes = Math::AlignUp(m_ConstantBuffer->GetSizeInBytes(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	m_Descriptor = _device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	device->CreateConstantBufferView(&desc, m_Descriptor.GetDescriptorHandle());
}
