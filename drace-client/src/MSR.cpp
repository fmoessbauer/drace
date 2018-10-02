#include "MSR.h"
#include "globals.h"
#include "ipc/SMData.h"
#include "ipc/MtSyncSHMDriver.h"

#include <mutex>

bool MSR::attach(const module_data_t * mod)
{
	LOG_INFO(0, "wait 10s for external resolver to attach");
	shmdriver = std::make_unique<ipc::MtSyncSHMDriver<true, true>>(DRACE_SMR_NAME, false);
	if (shmdriver->valid())
	{
		shmdriver->wait_receive(std::chrono::seconds(10));
		if (shmdriver->id() == ipc::SMDataID::CONNECT) {
			// Send PID and CLR path
			auto & sendInfo = shmdriver->emplace<ipc::BaseInfo>(ipc::SMDataID::PID);
			sendInfo.pid = (int)dr_get_process_id();
			strncpy(sendInfo.path.data(), mod->full_path, sendInfo.path.size());
			shmdriver->commit();

			if (shmdriver->wait_receive(std::chrono::seconds(10)) && shmdriver->id() == ipc::SMDataID::ATTACHED)
			{
				LOG_INFO(0, "MSR attached");
			}
			else {
				LOG_WARN(0, "MSR did not attach");
				shmdriver.reset();
			}
		}
		else {
			LOG_WARN(0, "MSR is not ready to connect");
			shmdriver.reset();
		}
	}
	else {
		LOG_WARN(0, "MSR is not running");
		shmdriver.reset();
	}
	return static_cast<bool>(shmdriver);
}

bool MSR::request_symbols(const module_data_t * mod)
{
	std::lock_guard<decltype(*shmdriver)> lg(*shmdriver);
	LOG_INFO(0, "MSR downloads the symbols from a symbol server (might take long)");
	// TODO: Download Symbols for some other .Net dlls as well
	auto & symreq = shmdriver->emplace<ipc::SymbolRequest>(ipc::SMDataID::LOADSYMS);
	symreq.base = (uint64_t)mod->start;
	symreq.size = mod->module_internal_size;
	strncpy(symreq.path.data(), mod->full_path, symreq.path.size());
	shmdriver->commit();

	while (bool valid_state = shmdriver->wait_receive(std::chrono::seconds(2))) {
		if (!valid_state) {
			LOG_WARN(0, "timeout during symbol download: ID %u", shmdriver->id());
			shmdriver.reset();
			break;
		}
		// we got a message
		switch (shmdriver->id()) {
		case ipc::SMDataID::CONFIRM:
			LOG_INFO(0, "Symbols downloaded, rescan");
			break;
		case ipc::SMDataID::WAIT:
			LOG_NOTICE(0, "wait for download to finish");
			shmdriver->id(ipc::SMDataID::CONFIRM);
			shmdriver->commit();
			break;
		default:
			LOG_WARN(0, "Protocol error, got %u", shmdriver->id());
			shmdriver.reset();
			break;
		}
	}
}

ipc::SymbolInfo MSR::lookup_address(app_pc pc) {
	std::lock_guard<decltype(*shmdriver)> lg(*shmdriver);
	shmdriver->put<uint64_t>(ipc::SMDataID::IP, (uint64_t)pc);
	shmdriver->commit();
	if (shmdriver->wait_receive(std::chrono::seconds(100))) {
		return shmdriver->get<ipc::SymbolInfo>();
	}
	LOG_WARN(0, "Timeout expired");
	return ipc::SymbolInfo();
}

ipc::SymbolResponse MSR::search_symbol(const module_data_t * mod, const std::string & match) {
	std::lock_guard<decltype(*shmdriver)> lg(*shmdriver);
	auto & sr = shmdriver->emplace<ipc::SymbolRequest>(ipc::SMDataID::SEARCHSYMS);
	sr.base = (uint64_t)mod->start;
	sr.size = mod->module_internal_size;
	strncpy(sr.path.data(), mod->full_path, sr.path.size());
	DR_ASSERT(match.size() <= sr.match.size(), "Matchstr larger than buffer");
	std::copy(match.begin(), match.end(), sr.match.begin());
	shmdriver->commit();
	if (shmdriver->wait_receive(std::chrono::seconds(100))) {
		return shmdriver->get<ipc::SymbolResponse>();
	}
	LOG_WARN(0, "Timeout expired");
	return ipc::SymbolResponse();
}