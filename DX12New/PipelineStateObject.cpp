#include "PipelineStateObject.h"
#include "Device.h"
#include "D3D12LibPCH.h"

PipelineStateObject::PipelineStateObject(Device& _device, const D3D12_PIPELINE_STATE_STREAM_DESC& _desc)
	:m_Device(_device)
{
	auto device = _device.GetD3D12Device();

	ThrowIfFailed(device->CreatePipelineState(&_desc, IID_PPV_ARGS(&m_d3d12PipelineState)));
}
