/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * Based on https://stackoverflow.com/questions/34733155/resolve-managed-and-native-stack-trace-which-api-to-use
 */

#include "ManagedResolver.h"
#include "LoggerTypes.h"

namespace msr {

	ManagedResolver::ManagedResolver() { }
	ManagedResolver::~ManagedResolver()
	{
		Close();
	}

	void ManagedResolver::Close() {
		if (debugClient)
			debugClient->DetachProcesses();

		if (_hProcess)
		{
			CloseHandle(_hProcess);
			_hProcess = nullptr;
		}

		clrDataProcess.Release();
		target.Release();
		debugClient.Release();
		debugControl.Release();
		debugSymbols.Release();
		debugSymbols3.Release();

		if (nullptr != _mscordacwks_dll)
		{
			FreeLibrary(_mscordacwks_dll);
			_mscordacwks_dll = nullptr;
		}
        if (nullptr != _dbgeng_dll)
        {
            FreeLibrary(_dbgeng_dll);
            _dbgeng_dll = nullptr;
        }
	}

	bool ManagedResolver::InitSymbolResolver(
		HANDLE hProcess,
		const char * mscordaccore_path,
		CString& lastError) {
		_hProcess = hProcess;

		_mscordacwks_dll = LoadLibrary(_T(mscordaccore_path));

		PFN_CLRDataCreateInstance pCLRCreateInstance = 0;

		if (_mscordacwks_dll != 0)
			pCLRCreateInstance = (PFN_CLRDataCreateInstance)GetProcAddress(_mscordacwks_dll, "CLRDataCreateInstance");

		if (_mscordacwks_dll == 0 || pCLRCreateInstance == 0)
		{
			lastError.Format(_T("Required dll mscordaccore.dll from .core2.0 installation was not found"));
			Close();
			return false;
		}

		BOOL isWow64 = FALSE;
		IsWow64Process(hProcess, &isWow64);
		target.Attach(new CLRDataTarget(hProcess, isWow64 != FALSE));


		HRESULT hr = pCLRCreateInstance(__uuidof(IXCLRDataProcess), target, (void**)&clrDataProcess);

		if (hr != S_OK) {
			lastError.Format(_T("failed to initialize Dotnet debugging instance: Err (Error code: 0x%08X)"), HRESULT_FROM_WIN32(GetLastError()));
			Close();
			return false;
		}

        // TODO: fix this memory leak
		_dbgeng_dll = LoadLibrary("dbgeng.dll");
		using PFN_DebugCreate = decltype(DebugCreate);
		PFN_DebugCreate* pDebugCreate = (PFN_DebugCreate*)GetProcAddress(_dbgeng_dll, "DebugCreate");

		hr = pDebugCreate(__uuidof(IDebugClient), (void**)&debugClient);
		if (FAILED(hr))
		{
			lastError.Format(_T("Could retrieve symbolic debug information using dbgeng.dll (Error code: 0x%08X)"), hr);
			Close();
			return false;
		}

		DWORD processId = GetProcessId(hProcess);
		const ULONG64 LOCAL_SERVER = 0;
		int flags = DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND;

		hr = debugClient->AttachProcess(LOCAL_SERVER, processId, flags);
		if (hr != S_OK)
		{
			lastError.Format(_T("Could attach to process 0x%X (Error code: 0x%08X)"), processId, hr);
			Close();
			return false;
		}

		debugControl = debugClient;

		debugControl->SetExecutionStatus(DEBUG_STATUS_GO);
		if ((hr = debugControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) != S_OK)
		{
			Close();
			return false;
		}

		debugSymbols3 = debugClient;
		debugSymbols = debugClient;
		// if debugSymbols3 == NULL - GetManagedFileLineInfo will not work
		return true;
	}

	bool ManagedResolver::GetFileLineInfo(void* ip, CStringA& lineInfo)
	{
		return GetModuleFileLineInfo(ip, &lineInfo, NULL);
	}

	bool ManagedResolver::GetModuleName(void* ip, CStringA& modulePath)
	{
		return GetModuleFileLineInfo(ip, NULL, &modulePath);
	}


	// Based on a native offset, passed in the first argument this function
	// identifies the corresponding source file name and line number.
	bool ManagedResolver::GetModuleFileLineInfo(void* ip, CStringA* lineInfo, CStringA* modulePath)
	{
		USES_CONVERSION;
		ULONG lineN = 0;
		char path[MAX_PATH];
		ULONG64 dispacement = 0;

		CComPtr<IXCLRDataMethodInstance> method;
		if (!debugSymbols || !debugSymbols3)
			return false;

		// Get managed method by address
		CLRDATA_ENUM methEnum;
		// TODO: This does not work for coreclr (reason is unclear even after massive debugging)
		HRESULT hr = clrDataProcess->StartEnumMethodInstancesByAddress((CLRDATA_ADDRESS)ip, NULL, &methEnum);
		if (hr == S_OK)
		{
			hr = clrDataProcess->EnumMethodInstanceByAddress(&methEnum, &method);
			clrDataProcess->EndEnumMethodInstancesByAddress(methEnum);
		}

		if (!method)
			goto lDefaultFallback;


		ULONG32 ilOffsets = 0;
		hr = method->GetILOffsetsByAddress((CLRDATA_ADDRESS)ip, 1, NULL, &ilOffsets);

		switch ((long)ilOffsets)
		{
		case CLRDATA_IL_OFFSET_NO_MAPPING:
			goto lDefaultFallback;

		case CLRDATA_IL_OFFSET_PROLOG:
			// Treat all of the prologue as part of the first source line.
			ilOffsets = 0;
			break;

		case CLRDATA_IL_OFFSET_EPILOG:
		{
			// Back up until we find the last real IL offset.
			CLRDATA_IL_ADDRESS_MAP mapLocal[16];
			CLRDATA_IL_ADDRESS_MAP* map = mapLocal;
			ULONG32 count = _countof(mapLocal);
			ULONG32 needed = 0;

			for (; ; )
			{
				hr = method->GetILAddressMap(count, &needed, map);

				if (needed <= count || map != mapLocal)
					break;

				map = new CLRDATA_IL_ADDRESS_MAP[needed];
			}

			ULONG32 highestOffset = 0;
			for (unsigned i = 0; i < needed; i++)
			{
				long l = (long)map[i].ilOffset;

				if (l == CLRDATA_IL_OFFSET_NO_MAPPING || l == CLRDATA_IL_OFFSET_PROLOG || l == CLRDATA_IL_OFFSET_EPILOG)
					continue;

				if (map[i].ilOffset > highestOffset)
					highestOffset = map[i].ilOffset;
			} //for

			if (map != mapLocal)
				delete[] map;

			ilOffsets = highestOffset;
		}
		break;
		} //switch

		mdMethodDef methodToken;
		void* moduleBase = 0;
		{
			CComPtr<IXCLRDataModule> module;

			hr = method->GetTokenAndScope(&methodToken, &module);
			if (!module)
				goto lDefaultFallback;

			// Get module file path.
			if (modulePath != NULL)
			{
				wchar_t wpath[MAX_PATH] = { 0 };
				ULONG32 PathLen = 0;

				hr = module->GetFileName(MAX_PATH, &PathLen, wpath);

				if (!SUCCEEDED(hr))
					return false;

				modulePath->Append(W2A(wpath));

				if (lineInfo == 0)
					// No need to dig any deeper.
					return true;
			}

			//
			// Retrieve ImageInfo associated with the IXCLRDataModule instance passed in. First look for NGENed module, second for IL modules.
			//
			for (int extentType = CLRDATA_MODULE_PREJIT_FILE; extentType >= CLRDATA_MODULE_PE_FILE; extentType--)
			{
				CLRDATA_ENUM enumExtents;
				if (module->StartEnumExtents(&enumExtents) != S_OK)
					continue;

				CLRDATA_MODULE_EXTENT extent;
				while (module->EnumExtent(&enumExtents, &extent) == S_OK)
				{
					logger->debug("+ EnumExtent @ {}, type {}", (void*)extent.base, extent.type);
					if (extentType != extent.type)
						continue;

					ULONG startIndex = 0;
					ULONG64 modBase = 0;

					hr = debugSymbols3->GetModuleByOffset((ULONG64)extent.base, 0, &startIndex, &modBase);
					logger->debug("++ Extent: base {}, length {}, status {}", (void*)extent.base, extent.length, hr);
					if (FAILED(hr))
						continue;

					moduleBase = (void*)modBase;

					if (moduleBase)
						break;
				}
				module->EndEnumExtents(enumExtents);

				if (moduleBase != 0)
					break;
			} //for
		} //module scope

		DEBUG_MODULE_AND_ID id;
		DEBUG_SYMBOL_ENTRY symInfo;
		hr = debugSymbols3->GetSymbolEntryByToken((ULONG64)moduleBase, methodToken, &id);
		if (FAILED(hr))
			goto lDefaultFallback;

		hr = debugSymbols3->GetSymbolEntryInformation(&id, &symInfo);
		if (FAILED(hr))
			goto lDefaultFallback;

		char* IlOffset = (char*)symInfo.Offset + ilOffsets;

		//
		// Source maps for managed code can end up with special 0xFEEFEE markers that
		// indicate don't-stop points.  Try and filter those out.
		//
		for (ULONG SkipCount = 64; SkipCount > 0; SkipCount--)
		{
			hr = debugSymbols3->GetLineByOffset((ULONG64)IlOffset, &lineN, path, sizeof(path), NULL, &dispacement);
			if (FAILED(hr))
				break;

			if (lineN == 0xfeefee)
				IlOffset++;
			else
				goto lCollectInfoAndReturn;
		}

		if (!FAILED(hr))
			// Fall into the regular translation as a last-ditch effort.
			ip = IlOffset;

	lDefaultFallback:
		if (lineInfo == NULL)
			return true;

		hr = debugSymbols3->GetLineByOffset((ULONG64)ip, &lineN, path, sizeof(path), NULL, &dispacement);

		if (FAILED(hr))
			return false;


	lCollectInfoAndReturn:
		*lineInfo += path;
		*lineInfo += "(";
		*lineInfo += std::to_string((uint64_t)lineN).c_str();
		*lineInfo += ")";
		return true;
	}


	bool ManagedResolver::GetMethodName(void* ip, CStringA& symbol)
	{
		USES_CONVERSION;
		CLRDATA_ADDRESS displacement = 0;
		ULONG32 len = 0;
		wchar_t name[1024];
		if (!clrDataProcess)
			return false;

		HRESULT hr = clrDataProcess->GetRuntimeNameByAddress((CLRDATA_ADDRESS)ip, 0, sizeof(name) / sizeof(name[0]), &len, name, &displacement);

		if (FAILED(hr))
			return false;

		name[len] = 0;
		symbol += W2A(name);
		return true;
	}

	void ManagedResolver::getStackTrace(void* ip) {
		ULONG frames_size = 32;
		PDEBUG_STACK_FRAME frames = new DEBUG_STACK_FRAME[frames_size];
		ULONG frames_filled;
		HRESULT hr = debugControl->GetStackTrace(0, 0, 0, frames, frames_size, &frames_filled);
		if (hr == S_OK) {
			logger->error("Could not resolve stack");
		}
	}

	CComQIPtr<IDebugControl4> ManagedResolver::getController() {
		return debugControl;
	}
} // namespace msr
