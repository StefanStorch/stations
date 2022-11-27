#pragma once
#include <unordered_map>
#include "boost/algorithm/string.hpp"

namespace stations::preprocessing {

    struct station {
        // maybe do something with information on bus or railway
        station(bool bus, bool railway) : bus_(bus), railway_(railway) {}
        station() = default;

        bool add_platform(std::string const& in_name, std::int64_t const& id) {
            bool success = false;
            std::vector<std::string> names{};
            if (in_name.find(';') == std::string::npos) {
                if (platforms_.count(in_name) < 1) {
                    success = true;
                    platforms_.insert({in_name, id});
                }
                return success;
            }
            boost::split(names, in_name, [](char c) {
                return c == ';';
            });
            for (auto const& name : names) {
                if (platforms_.count(name) < 1) {
                    success = true;
                    platforms_.insert({name, id});
                }
            }
            return success;
        }

        int64_t get_id(std::string const& platform) {
            if (platforms_.count(platform) > 0) {
                return platforms_.at(platform);
            }
            return -1;
        }

        bool bus_{false};
        bool railway_{false};
        std::unordered_map<std::string, std::int64_t> platforms_{};
    };
} // stations::preprocessing
