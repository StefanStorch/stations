#pragma once
#include <unordered_map>

namespace stations::preprocessing {

    struct station {
        std::unordered_map<std::string, std::int64_t> platforms_{};
    };
} // stations::preprocessing
