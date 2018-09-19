#include "MSR.h"
#include "globals.h"
#include "ipc/SyncSHMDriver.h"

bool MSR::attach(const module_data_t * mod)
{
	LOG_INFO(0, "wait 10s for external resolver to attach");
	shmdriver = std::make_unique<ipc::SyncSHMDriver<true, true>>(DRACE_SMR_NAME, false);
	if (shmdriver->valid())
	{
		shmdriver->wait_receive(std::chrono::seconds(10));
		if (shmdriver->id() == ipc::SMDataID::CONNECT) {
			// Send PID and CLR path
			auto & sendInfo = shmdriver->emplace<ipc::BaseInfo>(ipc::SMDataID::PID);
			sendInfo.pid = (int)dr_get_process_id();
			strncpy(sendInfo.path, mod->full_path, 128);
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
	LOG_INFO(0, "MSR downloads the symbols from a symbol server (might take long)");
	// TODO: Download Symbols for some other .Net dlls as well
	auto & symreq = shmdriver->emplace<ipc::SymbolRequest>(ipc::SMDataID::LOADSYMS);
	symreq.base = (uint64_t)mod->start;
	symreq.size = mod->module_internal_size;
	strncpy(symreq.path, mod->full_path, 128);
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
