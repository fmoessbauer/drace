/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "ProtocolHandler.h"
#include "LoggerTypes.h"

#include <future>

#include <Windows.h>
#include <DbgHelp.h>

#undef min
#undef max

BOOL PsymEnumeratesymbolsCallback(
	PSYMBOL_INFO pSymInfo,
	ULONG SymbolSize,
	PVOID UserContext)
{
	logger->debug("called {} @ {}", pSymInfo->Name, (void*)pSymInfo->Address);
	return true;
}

BOOL SymbolMatchCallback(
	PSYMBOL_INFO pSymInfo,
	ULONG SymbolSize,
	PVOID UserContext)
{
	auto vec = (std::vector<uint64_t> *)UserContext;
	vec->push_back(pSymInfo->Address);
	logger->debug("called {} @ {}", pSymInfo->Name, (void*)pSymInfo->Address);
	return true;
}

namespace msr {
	using namespace ipc;

	ProtocolHandler::ProtocolHandler(
		SyncSHMDriver shmdriver)
		: _shmdriver(shmdriver)
	{
		// Load external libs
		_dbghelp_dll = LoadLibrary("dbghelp.dll");
		syminit = (PFN_SymInitialize)GetProcAddress(_dbghelp_dll, "SymInitialize");
		symloadmod = (PFN_SymLoadModuleExW)GetProcAddress(_dbghelp_dll, "SymLoadModuleExW");
		symunloadmod = (PFN_SymUnloadModule)GetProcAddress(_dbghelp_dll, "SymUnloadModule");
		symgetmoduleinfo = (PFN_SymGetModuleInfoW64)GetProcAddress(_dbghelp_dll, "SymGetModuleInfoW64");
		symsearch = (PFN_SymSearch)GetProcAddress(_dbghelp_dll, "SymSearch");
		symgetopts = (PFN_SymGetOptions)GetProcAddress(_dbghelp_dll, "SymGetOptions");
		symsetopts = (PFN_SymSetOptions)GetProcAddress(_dbghelp_dll, "SymSetOptions");

		_shmdriver->id(SMDataID::READY);
		_shmdriver->commit();
	}

	ProtocolHandler::~ProtocolHandler() {
		quit();
	}

	void ProtocolHandler::connect() {
		const auto & bi = _shmdriver->get<BaseInfo>();
		logger->info("--- connected with DRace ---");
		_shmdriver->id(SMDataID::CONFIRM);
		_shmdriver->commit();
	}

	void ProtocolHandler::attachProcess() {
		CString lastError;

		const BaseInfo & bi = _shmdriver->get<BaseInfo>();
		_pid = bi.pid;
		// TODO: Probably less rights are sufficient
		_phandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, _pid);
		if (_phandle == NULL) {
			throw std::runtime_error("could not attach to process");
		}
		logger->info("--- attached to process {} ---", _pid);
		// we cannot validate this handle, hence pass it
		// to the resolver and handle errors there

		// Split *clr.dll path into filename and dir
		std::string path = bi.path.data();
		std::string dirname = path.substr(0, path.find_last_of("/\\")).c_str();
		std::string filename = path.substr(path.find_last_of("/\\") + 1).c_str();

		// helper dll
		std::string dacdll;

		if (filename == "coreclr.dll") {
			dacdll = "mscordaccore.dll";
		}
		else if (filename == "clr.dll") {
			dacdll = "mscordacwks.dll";
		}
		else {
			_shmdriver->id(SMDataID::EXIT);
			_shmdriver->commit();
			return;
		}
		std::string dacdllpath(dirname + '\\' + dacdll);
		logger->info("Load helper {}", dacdllpath);

		auto result = std::async([&]() {
			return _resolver.InitSymbolResolver(_phandle, dacdllpath.c_str(), lastError);
		});
		waitHeartbeat(result);

		if (!result.get()) {
			logger->error("{}", lastError.GetString());
			throw std::runtime_error("could not initialize resolver");
		}
		logger->info("Debugger initialized", bi.pid);
		_shmdriver->id(SMDataID::ATTACHED);
		_shmdriver->commit();

		// Initialize DbgHelp Symbol Resolver
		init_symbols();
	}

	void ProtocolHandler::detachProcess() {
		logger->info("--- detach MSR ---");
		//symcleanup(_phandle);
		_resolver.Close();
		
		_shmdriver->id(SMDataID::READY);
		_shmdriver->commit();
	}

	void ProtocolHandler::getCurrentStack() {
		auto ctx = _shmdriver->get<ipc::MachineContext>();
		logger->info("getCurrentStack");

		// theoretically a connection to the thread is not required
		// as the relevant information is passed from DRace
		// TODO: It is unclear if the stack-walk also works if the thread is not suspended
		// 
		// The stack-walk has to be executed at or shortly after the interesting ip
		// as otherwise the history gets blurry
		HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT, FALSE, ctx.threadid);
		if (hThread == NULL) {
			logger->error("could not attach to thread");
		}

		CONTEXT context = { 0 };
		context.ContextFlags = CONTEXT_FULL;
		if (GetThreadContext(hThread, &context)) {
			logger->info("Got Context");
			logger->info("Patch Context:");
			logger->debug("\tRbp {}->{}", (void*)context.Rbp, (void*)ctx.rbp);
			logger->debug("\tRsp {}->{}", (void*)context.Rsp, (void*)ctx.rsp);
			logger->debug("\tRip {}->{}", (void*)context.Rip, (void*)ctx.rip);
			context.Rbp = ctx.rbp;
			context.Rsp = ctx.rsp;
			context.Rip = ctx.rip;
			auto debugCtrl = _resolver.getController();
			if (!debugCtrl) goto cleanup;

			PDEBUG_STACK_FRAME frames   = new DEBUG_STACK_FRAME[32];
			PCONTEXT           contexts = new CONTEXT[32];
			ULONG              filled;
			HRESULT hr = debugCtrl->GetContextStackTrace(
				&context, sizeof(CONTEXT),
				frames, 32,
				contexts, 32 * sizeof(CONTEXT),
				sizeof(CONTEXT),
				&filled);
			if (hr == S_OK){
				for (unsigned i = 0; i < filled; ++i) {
					ATL::CString symbol;
					ATL::CString line;
					_resolver.GetMethodName((void*)contexts[i].Rip, symbol);
					_resolver.GetFileLineInfo((void*)contexts[i].Rip, line);
					logger->info("+ {}: Rip is {}, symbol: {} @ {}", i, (void*)contexts[i].Rip, symbol.GetBuffer(128), line.GetBuffer(128));
				}
			} else {
				logger->error("Could not walk stack");
			}

			//cleanup
			delete[] frames;
			delete[] contexts;
		}

		cleanup:
		if(hThread)
			CloseHandle(hThread);

		_shmdriver->id(SMDataID::CONFIRM);
		_shmdriver->commit();
	}

	void ProtocolHandler::resolveIP() {
		CString buffer;
		size_t bs;
		void* ip = _shmdriver->get<void*>();
		logger->debug("resolve IP: {}", ip);

		SymbolInfo & sym = _shmdriver->emplace<SymbolInfo>(ipc::SMDataID::IP);

		// Get Module Name
		_resolver.GetModuleName(ip, buffer);
		bs = sym.module.size();
		strncpy_s(sym.module.data(), bs, buffer.GetBuffer(static_cast<int>(bs)), bs);

		// Get Function Name
		buffer = "";
		_resolver.GetMethodName(ip, buffer);
		bs = sym.function.size();
		strncpy_s(sym.function.data(), bs, buffer.GetBuffer(static_cast<int>(bs)), bs);

		// Get Line Info
		buffer = "";
		_resolver.GetFileLineInfo(ip, buffer);
		bs = sym.path.size();
		strncpy_s(sym.path.data(), bs, buffer.GetBuffer(static_cast<int>(bs)), bs);
		logger->debug("+ resolve IP to: {}", sym.function.data());
		_shmdriver->commit();
	}

	void ProtocolHandler::init_symbols() {
		syminit(_phandle, NULL, false);

		DWORD symopts = symgetopts() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEBUG;
		symopts &= ~(SYMOPT_DEFERRED_LOADS); // remove this flag
		symsetopts(symopts);
	}

	void ProtocolHandler::loadSymbols() {
		const auto & sr = _shmdriver->get<ipc::SymbolRequest>();
		// Convert path
		std::string strpath(sr.path.data());
		std::wstring wstrpath(strpath.begin(), strpath.end());

		logger->info("download symbols for {}", sr.path.data());
		logger->debug("base: {}, size: {}", (void*)sr.base, sr.size);
		auto symload = std::async([&]() {
			return symloadmod(_phandle, NULL, wstrpath.c_str(), NULL, sr.base, (DWORD)sr.size, NULL, SYMSEARCH_ALLITEMS);
		});
		// wait for download to become ready
		waitHeartbeat(symload);
		DWORD64 loaded_base = symload.get();
		
		if (!loaded_base && GetLastError() != ERROR_SUCCESS) {
			logger->error("could not load debug symbols: {}", GetLastError());
		}
		else {
			IMAGEHLP_MODULEW64 info;
			memset(&info, 0, sizeof(info));
			info.SizeOfStruct = sizeof(info);
			if (symgetmoduleinfo(_phandle, sr.base, &info)) {
				switch (info.SymType) {
				case SymNone: logger->debug("No symbols found"); break;
				case SymExport: logger->debug("Only export symbols found"); break;
				case SymPdb:
					logger->debug("Loaded pdb symbols");
					break;
				case SymDeferred: logger->debug("Symbol load deferred"); break;
				case SymCoff:
				case SymCv:
				case SymSym:
				case SymVirtual:
				case SymDia: logger->debug("Symbols in image file loaded"); break;
				default: logger->debug("Symbols in unknown format"); break;
				}
		
				if (info.LineNumbers) {
					logger->debug("  module has line number information");
				}
			}
			// Do not close automatically, as further requests otherwise have to reopen
			// close using unloadSymbols()
			//symunloadmod(_phandle, sr.base);
			logger->info("download finished");
		}
		_shmdriver->id(ipc::SMDataID::CONFIRM);
		_shmdriver->commit();
	}

	void ProtocolHandler::unloadSymbols() {
		const auto & sr = _shmdriver->get<ipc::SymbolRequest>();
		symunloadmod(_phandle, sr.base);
		logger->debug("closed symbols");
		_shmdriver->id(ipc::SMDataID::CONFIRM);
		_shmdriver->commit();
	}

	void ProtocolHandler::searchSymbols() {
		const auto & sr = _shmdriver->get<ipc::SymbolRequest>();
		std::vector<uint64_t> symbol_addrs;
		std::string strpath(sr.path.data());
		std::wstring wstrpath(strpath.begin(), strpath.end());

		logger->debug("search symobls matching {} at {}", sr.match.data(), (void*)sr.base);
		if (symsearch(_phandle, sr.base, 0, 0, sr.match.data(), 0, SymbolMatchCallback,
			(void*)&symbol_addrs, sr.full ? SYMSEARCH_ALLITEMS : NULL))
		{
			auto & sr = _shmdriver->emplace<ipc::SymbolResponse>(ipc::SMDataID::SEARCHSYMS);
			sr.size = std::min(symbol_addrs.size(), sr.adresses.size());
			logger->debug("found {} matching symbols", symbol_addrs.size());
			// Do not copy more symbols than buffersize
			auto cpy_range_end = sr.size <= sr.adresses.size() ? symbol_addrs.end() : (symbol_addrs.begin() + sr.adresses.size());
			std::copy(symbol_addrs.begin(), cpy_range_end, sr.adresses.begin());
			_shmdriver->commit();
		}
	}

	template<typename T>
	void ProtocolHandler::waitHeartbeat(const std::future<T> & fut) {
		while (fut.wait_for(std::chrono::seconds(1)) != std::future_status::ready) {
			_shmdriver->id(ipc::SMDataID::WAIT);
			_shmdriver->commit();
			logger->debug("sent WAIT");
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			if (!_shmdriver->wait_receive(std::chrono::seconds(2))
				|| (_shmdriver->id() != ipc::SMDataID::CONFIRM))
			{
				logger->error("protocol error during symbol download");
				quit();
				return;
			}
		}
	}

	void ProtocolHandler::process_msgs() {
		// wait for incoming requests
		do {
			if (_shmdriver->wait_receive()) {
				logger->trace("Message with id {}", (int)_shmdriver->id());
				switch (_shmdriver->id()) {
				case SMDataID::CONNECT:
					connect(); break;
				case SMDataID::ATTACH:
					attachProcess(); break;
				case SMDataID::STACK:
					getCurrentStack(); break;
				case SMDataID::IP:
					resolveIP(); break;
				case SMDataID::LOADSYMS:
					loadSymbols(); break;
				case SMDataID::UNLOADSYMS:
					unloadSymbols(); break;
				case SMDataID::SEARCHSYMS:
					searchSymbols(); break;
				case SMDataID::CONFIRM:
					break;
				case SMDataID::EXIT:
					detachProcess(); break;
				default:
					logger->error("protocol error, got: {}", (int)_shmdriver->id());
					quit();
				}
			}
		} while (_keep_running);
	}

	void ProtocolHandler::quit() {
		_keep_running = false;
	}

} // namespace msr
