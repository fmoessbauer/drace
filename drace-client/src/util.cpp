#include "util.h"
#include "globals.h"
#include "module/Metadata.h"

#include <vector>
#include <string>
#include <regex>
#include <chrono>

#include <date/date.h>

bool util::common_prefix(const std::string& a, const std::string& b)
{
	if (!a.size() || !b.size()) {
		return false;
	}
	else  if (a.size() > b.size()) {
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

std::string util::dir_from_path(const std::string & fullpath) {
	return fullpath.substr(0, fullpath.find_last_of("/\\"));
}

std::string util::basename(const std::string & fullpath) {
	return fullpath.substr(fullpath.find_last_of("/\\") + 1);
}

std::string util::to_iso_time(std::chrono::system_clock::time_point tp) {
	return date::format("%FT%TZ", date::floor<std::chrono::milliseconds>(tp));
}

std::string util::instr_flags_to_str(uint8_t flags) {
	auto _flags = (module::Metadata::INSTR_FLAGS)flags;
	std::stringstream buffer;
	if (_flags & module::Metadata::INSTR_FLAGS::MEMORY) {
		buffer << "MEMORY ";
	}
	if (_flags & module::Metadata::INSTR_FLAGS::STACK) {
		buffer << "STACK ";
	}
	if (_flags & module::Metadata::INSTR_FLAGS::SYMBOLS) {
		buffer << "SYMBOLS ";
	}
	if (!_flags) {
		buffer << "NONE ";
	}
	return buffer.str();
}
