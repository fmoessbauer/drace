#pragma once
#include <memory>

#include "ManagedResolver.h"
#include "ipc/SyncSHMDriver.h"

namespace msr {
	/* Handles the communication protocol of Drace and MSR */
	class ProtocolHandler {
	public:
		using SyncSHMDriver = std::shared_ptr<ipc::SyncSHMDriver<false, false>>;
	private:
		ManagedResolver _resolver;
		SyncSHMDriver   _shmdriver;
		bool _keep_running{ true };

	private:
		/* Attach to the target process and load correct helper dll */
		void attachProcess();
		/* Detach from process */
		void detachProcess();
		/* Resolve single instruction pointer */
		void resolveIP();

	public:
		explicit ProtocolHandler(SyncSHMDriver);
		~ProtocolHandler();

		/* Wait for incoming messages and process them */
		void process_msgs();
		/* End Protocol */
		void quit();
	};

} // namespace msr
