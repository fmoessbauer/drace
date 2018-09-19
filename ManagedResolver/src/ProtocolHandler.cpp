#include "ProtocolHandler.h"
#include "spdlog/logger.h"

#include <future>

#include <Windows.h>
#include <DbgHelp.h>

extern std::shared_ptr<spdlog::logger> logger;

BOOL PsymEnumeratesymbolsCallback(
	PSYMBOL_INFO pSymInfo,
	ULONG SymbolSize,
	PVOID UserContext)
{
	logger->debug("called {}", pSymInfo->Name);
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

		_shmdriver->id(SMDataID::CONNECT);
		_shmdriver->commit();
	}

	ProtocolHandler::~ProtocolHandler() {
		quit();
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
		std::string path = bi.path;
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

		bool success = _resolver.InitSymbolResolver(_phandle, dacdllpath.c_str(), lastError);
		if (!success) {
			logger->error("{}", lastError.GetString());
			throw std::runtime_error("could not initialize resolver");
		}
		logger->info("Debugger initialized", bi.pid);
		_shmdriver->id(SMDataID::ATTACHED);
		_shmdriver->commit();
	}

	void ProtocolHandler::detachProcess() {
		logger->info("--- detach MSR ---");
		_resolver.Close();
		_shmdriver->id(SMDataID::CONNECT);
		_shmdriver->commit();
	}

	void ProtocolHandler::resolveIP() {
		CString buffer;
		void* ip = _shmdriver->get<void*>();
		logger->debug("resolve IP: {}", ip);

		SymbolInfo & sym = _shmdriver->emplace<SymbolInfo>(ipc::SMDataID::IP);

		// Get Module Name
		_resolver.GetModuleName(ip, buffer);
		strncpy_s(sym.module, buffer.GetBuffer(128), 128);

		// Get Function Name
		buffer = "";
		_resolver.GetMethodName(ip, buffer);
		strncpy_s(sym.function, buffer.GetBuffer(128), 128);

		// Get Line Info
		buffer = "";
		_resolver.GetFileLineInfo(ip, buffer);
		strncpy_s(sym.path, buffer.GetBuffer(128), 128);

		_shmdriver->commit();
	}

	void ProtocolHandler::loadSymbols() {
		using PFN_SymInitialize       = decltype(SymInitialize)*;
		using PFN_SymLoadModuleExW    = decltype(SymLoadModuleExW)*;
		using PFN_SymUnloadModule     = decltype(SymUnloadModule)*;
		using PFN_SymGetModuleInfoW64 = decltype(SymGetModuleInfoW64)*;
		using PFN_SymSearch           = decltype(SymSearch)*;
		using PFN_SymGetOptions       = decltype(SymGetOptions)*;
		using PFN_SymSetOptions       = decltype(SymSetOptions)*;

		PFN_SymInitialize syminit = (PFN_SymInitialize)GetProcAddress(_dbghelp_dll, "SymInitialize");
		PFN_SymLoadModuleExW symloadmod = (PFN_SymLoadModuleExW)GetProcAddress(_dbghelp_dll, "SymLoadModuleExW");
		PFN_SymUnloadModule symunloadmod = (PFN_SymUnloadModule)GetProcAddress(_dbghelp_dll, "SymUnloadModule");
		PFN_SymGetModuleInfoW64 symgetmoduleinfo = (PFN_SymGetModuleInfoW64)GetProcAddress(_dbghelp_dll, "SymGetModuleInfoW64");
		PFN_SymSearch symsearch = (PFN_SymSearch)GetProcAddress(_dbghelp_dll, "SymSearch");
		PFN_SymGetOptions symgetopts = (PFN_SymGetOptions)GetProcAddress(_dbghelp_dll, "SymGetOptions");
		PFN_SymSetOptions symsetopts = (PFN_SymSetOptions)GetProcAddress(_dbghelp_dll, "SymSetOptions");

		DWORD symopts = symgetopts() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
		symopts &= ~(SYMOPT_DEFERRED_LOADS); // remove this flag
		symsetopts(symopts);

		syminit(_phandle, NULL, false);

		const auto & sr = _shmdriver->get<ipc::SymbolRequest>();
		// Convert path
		std::string strpath(sr.path);
		std::wstring wstrpath(strpath.begin(), strpath.end());

		logger->info("download symbols for {}", sr.path);
		logger->debug("base: {}, size: {}", sr.base, sr.size);
		auto symload = std::async([&]() {
			return symloadmod(_phandle, NULL, wstrpath.c_str(), NULL, sr.base, (DWORD)sr.size, NULL, 0);
		});
		// wait for download to become ready
		while(symload.wait_for(std::chrono::seconds(1)) != std::future_status::ready) {
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
		DWORD64 loaded_base = symload.get();
		
		if (!loaded_base && GetLastError() != ERROR_SUCCESS) {
			logger->error("could not load debug symbols: {}", GetLastError());
		}
		else {
			IMAGEHLP_MODULEW64 info;
			/* i#1376c#12: we want to use the pre-SDK 8.0 size.  Otherwise we'll
			 * fail when used with an older dbghelp.dll.
			 */
			#define IMAGEHLP_MODULEW64_SIZE_COMPAT 0xcb8
			memset(&info, 0, sizeof(info));
			info.SizeOfStruct = IMAGEHLP_MODULEW64_SIZE_COMPAT;
			/* i#1197: SymGetModuleInfo64 fails on internal wide-to-ascii conversion,
			 * so we use wchar version SymGetModuleInfoW64 instead.
			 */
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
			symunloadmod(_phandle, sr.base);
			logger->info("download finished");
		}
		_shmdriver->id(ipc::SMDataID::CONFIRM);
		_shmdriver->commit();
	}

	void ProtocolHandler::process_msgs() {
		// wait for incoming requests
		do {
			if (_shmdriver->wait_receive()) {
				logger->trace("Message with id {}", (int)_shmdriver->id());
				switch (_shmdriver->id()) {
				case SMDataID::PID:
					attachProcess(); break;
				case SMDataID::IP:
					resolveIP(); break;
				case SMDataID::LOADSYMS:
					loadSymbols(); break;
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
