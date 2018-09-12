#pragma once

// Based on https://stackoverflow.com/questions/34733155/resolve-managed-and-native-stack-trace-which-api-to-use
// Regarding Managed IPs, see also
// https://github.com/dotnet/coreclr/blob/master/Documentation/botr/dac-notes.md

#include <windows.h>

#pragma warning (disable: 4091)     //dbghelp.h(1544): warning C4091: 'typedef ': ignored on left of '' when no variable is declared
#include "clrdata.h"                //IXCLRDataProcess
#include <atlbase.h>                //CComPtr
#include <atlstr.h>                 //CString
#include <Dbgeng.h>                 //IDebugClient
#pragma warning (default: 4091)

#include <string>
#include <iostream>

class ManagedResolver {
private:
	HMODULE _mscordacwks_dll;
	HANDLE  _hProcess{ nullptr };

	CComPtr<IDebugClient>       debugClient;
	CComQIPtr<IDebugControl>    debugControl;
	CComQIPtr<IDebugSymbols>    debugSymbols;
	CComQIPtr<IDebugSymbols3>   debugSymbols3;

public:
	CComPtr<IXCLRDataProcess> clrDataProcess;
	CComPtr<ICLRDataTarget> target;

public:
	ManagedResolver();
	~ManagedResolver();

	/*Clean up all open handles and detach debugger*/
	void Close();

	bool InitSymbolResolver(
		HANDLE hProcess,
		const char * mscordaccore_path,
		CString& lastError);

	bool GetFileLineInfo(void* ip, CStringA& lineInfo);
	bool GetModuleName(void* ip, CStringA& modulePath);

	/* Based on a native offset, passed in the first argument this function
	 * identifies the corresponding source file name and line number. */
	bool GetModuleFileLineInfo(void* ip, CStringA* lineInfo, CStringA* modulePath);
	bool GetMethodName(void* ip, CStringA& symbol);
};
