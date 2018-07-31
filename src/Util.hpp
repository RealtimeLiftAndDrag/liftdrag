#pragma once



#include <chrono>



namespace util {



// nanoseconds since epoch
inline long long now() {
    return std::chrono::nanoseconds(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// seconds since the first time this function is called
inline double time() {
    static long long s_start(now());
    return double(now() - s_start) * 1.0e-9;
}



}