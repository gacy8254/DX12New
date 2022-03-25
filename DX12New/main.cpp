#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <Shlwapi.h>
#include <dxgidebug.h>
#include <iostream>

#include "Application.h"
#include "Lesson3.h"

void ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	dxgiDebug->Release();
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	AllocConsole();
	freopen("conout$", "w", stdout);
	printf("hello hplonline!-_-\n");
	freopen("conout$", "w", stderr);

	int retCode = 0;

	//������Ŀ·��
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandleW(NULL);
	if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}

	Application::Create(hInstance);
	std::shared_ptr<Lesson3> demo = std::make_shared<Lesson3>(L"Learning DX12", 1280, 720, true);
	retCode = Application::Get().Run(demo);

	Application::Destroy();

	atexit(&ReportLiveObjects);

	return retCode;
}