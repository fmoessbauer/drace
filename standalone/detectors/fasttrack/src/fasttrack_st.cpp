#include <shared_mutex>
#include "fasttrack.h"
#include "fasttrack_st_export.h"

extern "C" FASTTRACK_ST_EXPORT Detector* CreateDetector() {
  return new drace::detector::Fasttrack<std::shared_mutex>();  // NOLINT
}
