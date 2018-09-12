#pragma once

// fix types
typedef UINT32  mdToken;
typedef mdToken mdTypeDef;
typedef mdToken mdMethodDef;
typedef mdToken mdFieldDef;
typedef ULONG   CorElementType;

#include "prebuild/xclrdata.h"
#include <Psapi.h>

class CLRDataTarget : public ICLRDataTarget
{
public:
	ULONG refCount;
	bool bIsWow64;
	HANDLE hProcess;

	CLRDataTarget(HANDLE _hProcess, bool _bIsWow64) :
		refCount(1),
		bIsWow64(_bIsWow64),
		hProcess(_hProcess)
	{ }

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID* ppvObject)
	{
		if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, __uuidof(ICLRDataTarget)))
		{
			AddRef();
			*ppvObject = this;
			return S_OK;
		}

		*ppvObject = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return ++refCount;
	}

	ULONG STDMETHODCALLTYPE Release(void)
	{
		refCount--;

		if (refCount == 0)
			delete this;

		return refCount;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMachineType(ULONG32 *machineType)
	{
#ifdef _WIN64
		if (!bIsWow64)
			*machineType = IMAGE_FILE_MACHINE_AMD64;
		else
			*machineType = IMAGE_FILE_MACHINE_I386;
#else
		*machineType = IMAGE_FILE_MACHINE_I386;
#endif

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPointerSize(ULONG32* pointerSize)
	{
#ifdef _WIN64
		if (!bIsWow64)
#endif
			*pointerSize = sizeof(PVOID);
#ifdef _WIN64
		else
			*pointerSize = sizeof(ULONG);
#endif
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetImageBase(LPCWSTR imagePath, CLRDATA_ADDRESS *baseAddress)
	{
		HMODULE dlls[1024] = { 0 };
		DWORD nItems = 0;
		wchar_t path[MAX_PATH];
		DWORD whatToList = LIST_MODULES_ALL;

		if (bIsWow64)
			whatToList = LIST_MODULES_32BIT;

		if (!EnumProcessModulesEx(hProcess, dlls, sizeof(dlls), &nItems, whatToList))
		{
			DWORD err = GetLastError();
			return HRESULT_FROM_WIN32(err);
		}

		nItems /= sizeof(HMODULE);
		for (unsigned int i = 0; i < nItems; i++)
		{
			path[0] = 0;
			if (GetModuleFileNameExW(hProcess, dlls[i], path, sizeof(path) / sizeof(path[0])))
			{
				wchar_t* pDll = wcsrchr(path, L'\\');
				if (pDll) pDll++;

				if (_wcsicmp(imagePath, path) == 0 || _wcsicmp(imagePath, pDll) == 0)
				{
					*baseAddress = (CLRDATA_ADDRESS)dlls[i];
					return S_OK;
				}
			}
		}
		return E_FAIL;
	}

	virtual HRESULT STDMETHODCALLTYPE ReadVirtual(CLRDATA_ADDRESS address, BYTE *buffer, ULONG32 bytesRequested, ULONG32 *bytesRead)
	{
		SIZE_T readed;

		if (!ReadProcessMemory(hProcess, (void*)address, buffer, bytesRequested, &readed))
			return HRESULT_FROM_WIN32(GetLastError());

		*bytesRead = (ULONG32)readed;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE WriteVirtual(CLRDATA_ADDRESS address, BYTE *buffer, ULONG32 bytesRequested, ULONG32 *bytesWritten)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetTLSValue(ULONG32 threadID, ULONG32 index, CLRDATA_ADDRESS *value)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SetTLSValue(ULONG32 threadID, ULONG32 index, CLRDATA_ADDRESS value)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetCurrentThreadID(ULONG32 *threadID)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetThreadContext(ULONG32 threadID, ULONG32 contextFlags, ULONG32 contextSize, BYTE *context)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SetThreadContext(ULONG32 threadID, ULONG32 contextSize, BYTE *context)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Request(ULONG32 reqCode, ULONG32 inBufferSize, BYTE *inBuffer, ULONG32 outBufferSize, BYTE *outBuffer)
	{
		return E_NOTIMPL;
	}
}; //CLRDataTarget