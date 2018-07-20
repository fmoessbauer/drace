#pragma once

#include "util.h"

#include <string>
#include <vector>
#include <memory>

#include <INIReader.h>

namespace drace {
	/* Handles dynamic config information from configuration file */
	class Config {
	private:
		std::unique_ptr<INIReader> _reader;
		std::set<std::string>      _sections;

		std::map<std::string, std::string> _kvstore;
	public:
		Config() = default;
		Config(const Config &) = delete;
		Config(Config &&) = default;

		Config & operator=(const Config&) = delete;
		Config & operator=(Config&&) = default;

		explicit Config(std::string filename) :
			_reader(std::make_unique<INIReader>(filename))
		{
			_sections = _reader->Sections();
		}

		inline bool loadfile(const std::string & filename) {
			_reader = std::make_unique<INIReader>(filename);
			if (_reader->ParseError() < 0) {
				return false;
			}
			_sections = _reader->Sections();
			return true;
		}

		inline void set(
			const std::string & key,
			const std::string & val) {
			_kvstore[key] = val;
		}

		inline std::string get(
			const std::string & section,
			const std::string & key,
			const std::string & default) const
		{
			if (_kvstore.count(key) > 0) {
				return _kvstore.at(key);
			}
			return _reader->Get(section, key, default);
		}

		/* returns multiline ini-items as a vector */
		inline std::vector<std::string> get_multi(
			const std::string & section,
			const std::string & key) const
		{
			const auto & val = _reader->Get(section, key, "");
			if (val == "") {
				return std::vector<std::string>();
			}
			return util::split(val, "\n");
		}
	};
}
