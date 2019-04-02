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

#include <Windows.h>
#include <Dbghelp.h>
#include <memory>
#include <future>

#include "ManagedResolver.h"
#include "ipc/SyncSHMDriver.h"

namespace msr {
    /** Handles the communication protocol of Drace and MSR */
    class ProtocolHandler {
    public:
        using SyncSHMDriver = std::shared_ptr<ipc::SyncSHMDriver<false, false>>;
    private:
        ManagedResolver _resolver;
        SyncSHMDriver   _shmdriver;
        bool _keep_running{ true };

        HMODULE _dbghelp_dll;
        int    _pid;
        HANDLE _phandle;

        /* DbgHelp.dll Symbols */

        using PFN_SymInitialize = decltype(SymInitialize)*;
        using PFN_SymCleanup = decltype(SymCleanup)*;
        using PFN_SymLoadModuleExW = decltype(SymLoadModuleExW)*;
        using PFN_SymUnloadModule = decltype(SymUnloadModule)*;
        using PFN_SymGetModuleInfoW64 = decltype(SymGetModuleInfoW64)*;
        using PFN_SymSearch = decltype(SymSearch)*;
        using PFN_SymGetOptions = decltype(SymGetOptions)*;
        using PFN_SymSetOptions = decltype(SymSetOptions)*;
        using PFN_SymGetSearchPath = decltype(SymGetSearchPath)*;

        PFN_SymInitialize syminit;
        PFN_SymCleanup symcleanup;
        PFN_SymLoadModuleExW symloadmod;
        PFN_SymUnloadModule symunloadmod;
        PFN_SymGetModuleInfoW64 symgetmoduleinfo;
        PFN_SymSearch symsearch;
        PFN_SymGetOptions symgetopts;
        PFN_SymSetOptions symsetopts;
        PFN_SymGetSearchPath symgetsearchpath;

    private:
        /** Connect with DRace */
        void connect();
        /** Attach to the target process and load correct helper dll */
        void attachProcess();
        /** Detach from process */
        void detachProcess();
        /** TEST: Determin current Stack */
        void getCurrentStack();
        /** Resolve single instruction pointer */
        void resolveIP();
        /** Initialize symbol resolver */
        void init_symbols();
        /** Download Symbols from SymServer */
        void loadSymbols();
        /** Close opened symbols and release resources */
        void unloadSymbols();
        /** Return adresses of matching symols */
        void searchSymbols();
        /** wait for a long running action to finish and perform a heartbeat in the meantime */
        template<typename T>
        void waitHeartbeat(const std::future<T> & fut);

    public:
        explicit ProtocolHandler(SyncSHMDriver);
        ~ProtocolHandler();

        /** Wait for incoming messages and process them */
        void process_msgs();
        /** End Protocol */
        void quit();
    };

} // namespace msr
