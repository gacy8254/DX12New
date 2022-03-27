#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // Ϊ��ʹ��CommandLineToArgW()����

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
//�����������Ĺ��ܲ�࣬�������������꣬��ȡ�������Ķ���
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif
//����궨����Windows.hͷ�ļ���
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
//Com�����Ҫ��ͷ�ļ�
#include <wrl.h>
using namespace Microsoft::WRL;

//DX12ͷ�ļ�
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include "d3dx12.h"

//��׼��
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

// Assimp header files.
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/anim.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#if defined( __cpp_lib_filesystem )
namespace fs = std::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif

//�Լ����ļ�
#include "helpers.h"
