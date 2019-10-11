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

#include <dr_api.h>
#include "config.h"
#include "util.h"

#include <string>
#include <vector>
#include <memory>

#include <INIReader.h>

namespace drace {
    /// Handles dynamic config information from configuration file
    Config::Config(std::string filename) :
        _reader(std::make_unique<INIReader>(filename))
    {
        _sections = _reader->Sections();
    }

    bool Config::loadfile(
        const std::string & filename,
        const std::string & hint) {
        std::string filepath = filename;

        for (int i = 0; i < 2; ++i) {
            _reader = std::make_unique<INIReader>(filepath);
            const auto err = _reader->ParseError();
            if (err < 0) {
                // file not found, but just filename provided
                if (i == 0 && util::basename(filename).compare(filename) == 0)
                {
                    filepath = util::dir_from_path(hint) + "\\" + filename;
                    LOG_WARN(0, "Try to load config file in %s", filepath.c_str());
                    continue;
                }
                else {
                    return false;
                }
            }
            else if (err != 0) {
                // file found, but parser error
                LOG_ERROR(0, "Error in config file @line %d", err);
                return false;
            }
            break;
        }
        _sections = _reader->Sections();
        return true;
    }

    void Config::set(
        const std::string & key,
        const std::string & val) {
        _kvstore[key] = val;
    }

    std::string Config::get(
        const std::string & section,
        const std::string & key,
        const std::string & default) const
    {
        if (_kvstore.count(key) > 0) {
            return _kvstore.at(key);
        }
        return _reader->Get(section, key, default);
    }

    /// returns multiline ini-items as a vector
    std::vector<std::string> Config::get_multi(
        const std::string & section,
        const std::string & key) const
    {
        const auto & val = _reader->Get(section, key, "");
        if (val == "") {
            return std::vector<std::string>();
        }
        return util::split(val, "\n");
    }
}
