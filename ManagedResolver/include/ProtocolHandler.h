#pragma once
#include <memory>

#include "ManagedResolver.h"
#include "ipc/msr-driver.h"

class ProtocolHandler {
public:
	using MsrDriverPtr = std::shared_ptr<MsrDriver<false, false>>;
private:
	ManagedResolver _resolver;
	MsrDriverPtr _msrdriver;
	bool _keep_running{ true };

private:
	void attachProcess();
	void detachProcess();
	void resolveIP();

public:
	explicit ProtocolHandler(MsrDriverPtr);
	~ProtocolHandler();

	void process_msgs();
	void quit();
};
