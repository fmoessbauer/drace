#include "gtest/gtest.h"

#include "ipc/spinlock.h"

#include <mutex>
#include <thread>

TEST(Spinlock, Interface) {
    ipc::spinlock mx;
    ASSERT_TRUE(mx.try_lock());
    mx.unlock();

    {
        std::lock_guard<ipc::spinlock> lg(mx);
        ASSERT_FALSE(mx.try_lock());
    }
}

TEST(Spinlock, Congestion){
    ipc::spinlock mx;

    std::vector<std::thread> threads;

    for(unsigned i=0; i<std::thread::hardware_concurrency() * 4; ++i){
        threads.emplace_back([&mx](){
            std::lock_guard<ipc::spinlock> lg(mx);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }
    for(auto & t : threads){
        t.join();
    }
}

TEST(Spinlock, Correctness){
    ipc::spinlock mx;
    std::vector<std::thread> threads;
    int var{0};
    const unsigned num_threads = std::thread::hardware_concurrency() * 4;
    const unsigned loc_incs = 20;

    for(unsigned i=0; i<num_threads; ++i){
        threads.emplace_back([&](){
            for(unsigned j=0; j<loc_incs; ++j){
                std::lock_guard<ipc::spinlock> lg(mx);
                int tmp = var + 1;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                var = tmp;
            }
        });
    }
    for(auto & t : threads){
        t.join();
    }
    ASSERT_EQ(var, loc_incs * num_threads);
}
