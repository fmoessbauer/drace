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

#include <string>
#include <dr_api.h>


namespace drace {
    /**
     * \brief C++ wrapper around a DynamoRIO file handle
     */
    class DrFile {
    private:
        file_t _handle{0};
        bool   _needs_closing{false};

    public:
        DrFile(std::string filename, uint mode_flags) {
            if (filename == "stdout")
                _handle = STDOUT;
            else if (filename == "stderr")
                _handle = STDERR;
            else { // filename
                if ((_handle = dr_open_file(filename.c_str(), mode_flags)) != INVALID_FILE)
                    _needs_closing = true;
            }
        }
        ~DrFile() {
            if (_needs_closing) {
                dr_close_file(_handle);
            }
            else {
                dr_flush_file(_handle);
            }
        }

        bool good() const {
            return _handle != INVALID_FILE;
        }

        inline file_t get() const {
            return _handle;
        }
    };
}
