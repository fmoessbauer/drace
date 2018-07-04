#include "util.h"

bool util::common_prefix(const std::string& a, const std::string& b)
{
	if (a.size() > b.size()) {
		return a.substr(0, b.size()) == b;
	}
	else {
		return b.substr(0, a.size()) == a;
	}
}
