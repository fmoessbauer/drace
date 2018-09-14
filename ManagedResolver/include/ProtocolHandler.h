#pragma once
#include <memory>

#include "ManagedResolver.h"
#include "ipc/msr-driver.h"

/* Handles the communication protocol of Drace and MSR */
class ProtocolHandler {
public:
	using MsrDriverPtr = std::shared_ptr<MsrDriver<false, false>>;
private:
	ManagedResolver _resolver;
	MsrDriverPtr _msrdriver;
	bool _keep_running{ true };

private:
	/* Attach to the target process and load correct helper dll */
	void attachProcess();
	/* Detach from process */
	void detachProcess();
	/* Resolve single instruction pointer */
	void resolveIP();

public:
	explicit ProtocolHandler(MsrDriverPtr);
	~ProtocolHandler();

	/* Wait for incoming messages and process them */
	void process_msgs();
	/* End Protocol */
	void quit();
};
