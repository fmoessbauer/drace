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

#include "detector/Detector.h"

#include <array>

namespace drace {
	/**
    * \brief shadow stack implementation for stack-tracing
    *
	* This class implements a shadow stack, used for
	* getting stack traces for race detection
	* Whenever a call has been detected, the memory-refs buffer is
	* flushed as all memory-refs happend in the current function.
	* This avoids storing a stack-trace per memory-entry.
	*/
	class ShadowStack {
	public:
        /**
         * \brief maximum number of stack entries.
         * That's one less than in the detector,
         * as there the last element represents the
         * racy pc
         */
		static constexpr int max_size = Detector::max_stack_size-1;

    private:
        std::array<void*, max_size> _data;
        uint _entries{0};
        Detector * _detector{nullptr};

	public:

        ShadowStack() = default;
        explicit ShadowStack(Detector * det)
        : _detector(det) { }

        inline void bindDetector(Detector * det){
            _detector = det;
        }

        /// return true if the stack is empty
        inline bool isEmpty() const {
            return _entries == 0;
        }

		/**
        * \brief Push a pc on the stack.
        *
        * If the pc is already
		* on the stack, skip. This avoids ever growing stacks
		* if the returns are not detected properly
		*/
		inline void push(void *addr, void* det_data)
		{
			if (_entries >= max_size) return;
			_detector->func_enter(det_data, addr);
			_data[_entries++] = addr;
		}

		inline void *pop(void* det_data)
		{
			DR_ASSERT(_entries > 0);
			--_entries;

			_detector->func_exit(det_data);
			return _data[_entries];
		}
	};
} // namespace drace
