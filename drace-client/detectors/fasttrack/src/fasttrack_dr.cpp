#include "fasttrack.h"
#include "fasttrack_dr_export.h"
#include "ipc/DrLock.h"

extern "C" FASTTRACK_DR_EXPORT Detector* CreateDetector() {
  return new drace::detector::Fasttrack<DrLock>();  // NOLINT
}