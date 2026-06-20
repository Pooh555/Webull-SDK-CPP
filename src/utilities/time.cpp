#include "time.hpp"

#include <chrono>
#include <format>

namespace utilities::time {

std::string get_utc_timestamp() {
    std::chrono::utc_clock::time_point current_time = std::chrono::utc_clock::now();
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}", std::chrono::floor<std::chrono::seconds>(current_time));
}

}