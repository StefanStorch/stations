#pragma once
#include <unordered_map>

namespace stations::preprocessing {

    struct station {
        // TODO maybe add more info like bus/railway
        std::unordered_map<std::string, std::int64_t> platforms_{};
    };
} // stations::preprocessing
