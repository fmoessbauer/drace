#include "util.h"
#include <vector>
#include <string>
#include <regex>

bool util::common_prefix(const std::string& a, const std::string& b)
{
	if (a.size() > b.size()) {
		return a.substr(0, b.size()) == b;
	}
	else {
		return b.substr(0, a.size()) == a;
	}
}


std::vector<std::string> util::split(const std::string & str, const std::string & delimiter) {
	std::string regstr("([");
	regstr += (delimiter + "]+)");
	std::regex regex{ regstr }; // split on space and comma
	std::sregex_token_iterator it{ str.begin(), str.end(), regex, -1 };
	std::vector<std::string> words{ it,{} };
	return words;
}
