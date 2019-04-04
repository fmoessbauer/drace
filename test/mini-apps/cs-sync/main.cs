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

using System;
using System.Threading;

/**
 * Dummy application to test various C# synchronization mechanisms
 */
namespace MultithreadingApplication
{

    enum LockMechanism
    {
        None,
        Mutex,
        Monitor
    }

    class Param
    {
        public int acc = 0;
        public LockMechanism lock_mechanism = LockMechanism.None;
    }

    class ThreadCreationProgram
    {
        private static readonly Mutex mut = new Mutex();

        // Inlining is a problem
        public static void IncByOneHelper(Param p)
        {
            int acc = p.acc;
            ++acc;
            Thread.Yield();
            p.acc = acc;
        }

        public static void IncByOne(Param p)
        {
            //System.Diagnostics.Debugger.Break();
            for (int i = 0; i < 1000; ++i)
            {
                if (p.lock_mechanism == LockMechanism.Monitor)
                {
                    lock (p)
                    {
                        IncByOneHelper(p);
                    }
                }
                else if (p.lock_mechanism == LockMechanism.Mutex)
                {
                    mut.WaitOne();
                    IncByOneHelper(p);
                    mut.ReleaseMutex();
                }
                else {
                    IncByOneHelper(p);
                }
            }
        }

        public static void ThreadFunc(Param p)
        {
            IncByOne(p);
        }

        static void Main(string[] args)
        {
            int numThreads = 4;
            Param p = new Param();

            if (args.Length != 1) {
                Console.WriteLine("usage: {0} none|monitor|mutex", System.Reflection.Assembly.GetExecutingAssembly().GetName().Name);
                System.Environment.Exit(1);
            }
            if (args[0] == "monitor")
            {
                p.lock_mechanism = LockMechanism.Monitor;
                Console.WriteLine("lock: monitor");
            }
            else if (args[0] == "mutex")
            {
                p.lock_mechanism = LockMechanism.Mutex;
                Console.WriteLine("lock: mutex");
            }
            else {
                Console.WriteLine("lock: none");
            }

            int threadId = System.Threading.Thread.CurrentThread.ManagedThreadId;
            Console.WriteLine("xx - Begin Main {0}", threadId);

            Thread[] threads = new Thread[numThreads];
            for (int i = 0; i < numThreads; i++)
            {
                Thread t = new Thread(() => ThreadFunc(p));
                threads[i] = t;
            }
            for (int i = 0; i < numThreads; i++)
            {
                threads[i].Start();
            }

            Console.WriteLine("xx - Join Threads");
            foreach (var t in threads)
                t.Join();

            Console.WriteLine("xx - Value after {0} increments: {1}", numThreads * 1000, p.acc);
        }
    }
}
