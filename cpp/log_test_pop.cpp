#include "Lockfree.h"

#include <cstdlib>

#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>

using namespace std;
using namespace std::chrono_literals;

atomic_bool stopLogging(false);
lockfree::Stack<string> logStack;

int main(void)
{
    if (!logStack.isLockFree())
        throw runtime_error("Not Lock Free!");

    // Launch consumer thread
    thread tc([&] {
            while (!logStack.empty() || !stopLogging)
            {
                string str;
                if (logStack.pop(str))
                    cout << str << flush;
                else
                    std::this_thread::sleep_for(1ms);
            }
        });

    cout << "consumer launched\n" << flush;

    // Launch producer threads
    vector<thread> pts;
    for (int i = 0; i < 8; i ++)
    {
        pts.emplace_back([=] {
                int idx = 0;
                for (int k = 0; k < 100; k++)
                {
                    int count = std::rand() * 1000LL / RAND_MAX;
                    for (int j = 0; j < count; j++)
                    {
                        ostringstream os;
                        os << i << "." << setw(8) << setfill('0') << idx++ << endl;
                        string str = os.str();
                        logStack.push(str);
                        //cout << "Pushed " << str << endl << flush;
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
    stopLogging = true;
    tc.join();

    return 0;
}
