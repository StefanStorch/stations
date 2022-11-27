#pragma once
#include <unordered_map>
#include "stations/preprocessing/station.h"

namespace stations::preprocessing {

struct preprocessing_result {
    bool successful() {
        return successful_ || !stations_.empty();
    }

    int64_t get_id(std::string const& station, std::string const& platform) {
        if (stations_.count(station) > 0) {
            return stations_.find(station)->second.get_id(platform);
        }
        return -1;
    }

    bool successful_{false};
    int num_entries_{0};
    std::unordered_map<std::string, station> stations_{};
};

int map_stations(preprocessing_result& result);
} // stations::preprocessing