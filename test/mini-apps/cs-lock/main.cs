/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) Siemens AG, 2018
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
 */

using System;
using System.Threading;

namespace MultithreadingApplication
{
    class param
    {
        public int acc = 0;
    }

    class ThreadCreationProgram
    {
        public static void IncByOne(param p)
        {
            lock (p)
            {
                ++(p.acc);
            }
        }

        static void Main(string[] args)
        {
            int numThreads = 2;
            param p        = new param();

           Thread[] threads = new Thread[numThreads];
            for (int i = 0; i < numThreads; i++)
            {
                Thread t = new Thread(() => IncByOne(p));
                threads[i] = t;
            }
            for (int i = 0; i < numThreads; i++)
            {
                threads[i].Start();
            }

            foreach (var t in threads)
                t.Join();

            Console.WriteLine("Value after {0} increments: {1}", numThreads, p.acc);
        }
    }
}
