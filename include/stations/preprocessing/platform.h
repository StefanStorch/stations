#include <utility>
#include "stations/preprocessing/preprocessing.h"
#include <iostream>

#pragma once

namespace stations::preprocessing {
    enum class osm_type : std::uint8_t { NODE, WAY, RELATION};
    struct platform {
        platform(int64_t id, std::string name, osm_type type) : osm_id_(id), name_(std::move(name)), type_(type) {}
        platform(int64_t id, std::string name) : osm_id_(id), name_(std::move(name)), type_(osm_type::NODE) {}

        int64_t osm_id_;
        std::string name_;
        osm_type type_;
    };

    inline std::ostream &operator<<(std::ostream &os, platform const &plat) {
        os << plat.name_ << " " << plat.osm_id_ << "\n";
        return os;
    }
} // namespace stations::preprocessing