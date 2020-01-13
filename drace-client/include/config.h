#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "util.h"

#include <string>
#include <vector>
#include <memory>

#include <INIReader.h>

namespace drace {
	/// Handles dynamic config information from configuration file
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

        explicit Config(const std::string & filename);

        bool loadfile(
            const std::string & filename,
            const std::string & hint);

        void set(
            const std::string & key,
            const std::string & val);

        std::string get(
            const std::string & section,
            const std::string & key,
            const std::string & def_val) const;

		/// returns multiline ini-items as a vector
        std::vector<std::string> get_multi(
            const std::string & section,
            const std::string & key) const;
	};
}
