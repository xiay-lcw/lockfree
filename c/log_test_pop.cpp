#include <cstdlib>
#include <cassert>

#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <future>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "lockfree.h"

using namespace std;
using namespace std::chrono_literals;

atomic_bool stopLogging(false);
LFStack* logStack;

int main(void)
{
    atomic<uint64_t> print_count(0);
    atomic<uint64_t> store_count(0);

    logStack = createLockFreeStack();

    if (!isStackLockFree(logStack))
        throw runtime_error("Not Lock Free!");

    // Launch consumer thread
    auto future = async(launch::async,
        [&]() -> bool {
            while (!isStackEmpty(logStack) || !stopLogging)
            {
                waitStackNonEmpty(logStack);

                string* pstr;
                if (popLockFreeStack(logStack, (void**)&pstr))
                {
                    cout << *pstr << flush;
                    delete pstr;
                    print_count.fetch_add(1);
                }
            }
            return true;
        });

    cout << "consumer launched\n" << flush;

    // Launch producer threads
    vector<thread> pts;
    for (int i = 0; i < 8; i ++)
    {
        pts.emplace_back([=,&store_count] {
                int idx = 0;
                for (int k = 0; k < 100; k++)
                {
                    int count = std::rand() * 1000LL / RAND_MAX;
                    for (int j = 0; j < count; j++)
                    {
                        ostringstream os;
                        os << i << "." << setw(8) << setfill('0') << idx++ << endl;
                        pushLockFreeStack(logStack, (void*)new string(os.str()));
                        store_count.fetch_add(1);
                    }
                    int time = std::rand() * 10LL / RAND_MAX;
                    std::this_thread::sleep_for(time * 1ms);
                }
            });
    }

    cout << "Producers launched\n"  << flush;

    for (auto& t : pts)
    {
        t.join();
    }

    cerr << "Producers finished\n" << flush;

    stopLogging = true;
    future.wait_for(std::chrono::seconds(5));
    if (print_count != store_count)
    {
        cerr << print_count << " != " << store_count << endl << flush;
        //throw runtime_error("Not all stuff printed");
    }
    else
    {
        wakeStackWaiters(logStack);
    }

    future.wait();

    cerr << "Logger finished\n" << flush;

    destroyLockFreeStack(logStack);

    return 0;
}
