#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // 为了使用CommandLineToArgW()函数

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
//下面三个语句的功能差不多，如果定义了这个宏，就取消这个宏的定义
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif
//这个宏定义在Windows.h头文件中
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
//Com组件需要的头文件
#include <wrl.h>
using namespace Microsoft::WRL;

//DX12头文件
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

//标准库
#include <algorithm>
#include <cassert>
#include <chrono>
#include <string>
#include <conio.h>

//自己的文件
#include "helpers.h"
