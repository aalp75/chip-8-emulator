#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

void logTimePrefix() {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t t = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::cout
        << std::put_time(&tm, "%d/%m/%Y %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count()
        << " ";
}