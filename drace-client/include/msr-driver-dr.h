#pragma once

#include <dr_api.h>

#include "ipc/msr-driver.h"

template<bool SENDER>
class MsrDriverDr : public MsrDriver<SENDER, true> {
	void yield() const {
		dr_thread_yield();
	}
};
