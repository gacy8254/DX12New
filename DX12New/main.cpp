#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <Shlwapi.h>
#include <dxgidebug.h>

#include <shellapi.h>
#include <memory>

#include "Application.h"
#include "Device.h"
#include "Lesson5.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#if defined(_DEBUG)
	Device::EnableDebufLayer();
#endif
	//设置项目路径
	WCHAR path[MAX_PATH];
	int argc = 0;
	LPWSTR* argv = ::CommandLineToArgvW(lpCmdLine, &argc);

	if (argv)
	{
		for (int i = 0; i < argc; i++)
		{
			if (::wcscmp(argv[i], L"-wd") == 0)
			{
				::wcscpy_s(path, argv[++i]);
				::SetCurrentDirectoryW(path);
			}
		}
		::LocalFree(argv);
	}

	int retCode = 0;

	Application::Create(hInstance);
	{
		std::unique_ptr<Lesson5> demo = std::make_unique<Lesson5>(L"Learning DX12", 1601, 900);
		retCode = demo->Run();
	}
	Application::Destroy();

	atexit(&Device::ReportLiveObjects);

	return retCode;
}