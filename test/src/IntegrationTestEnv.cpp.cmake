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
/*
 * This file is auto-generated
 */

#include <string>
#include "IntegrationTest.h"

// encode path to drrun into test executable to be able to execute
// integration tests without explicitly specifiying the path
std::string Integration::drrun = R"(@PATH_TO_DRRUN@)";
