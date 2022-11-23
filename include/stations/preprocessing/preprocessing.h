#pragma once
#include <unordered_map>
#include "stations/preprocessing/station.h"

namespace stations::preprocessing {

struct preprocessing_result {
    bool successful() {
        return successful_ || !stations_.empty();
    }

    bool successful_{false};
    std::unordered_map<std::string, station> stations_{};
};

int map_stations(preprocessing_result& result);
} // stations::preprocessing