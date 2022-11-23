#include "stations/preprocessing/handle_stations.h"

namespace stations::preprocessing {
    int handle_stations(osmium::Relation const& relation, std::unordered_map<std::string, station>& map) {
        if (relation.tags().has_key("name") && relation.tags().has_key("ref")) {
            if (map.count(relation.get_value_by_key("name"))) {
                auto station = map.find(relation.get_value_by_key("name"));
                station->second.platforms_.insert({relation.get_value_by_key("ref"), relation.id()});
            } else {
                auto temp_station = station{};
                temp_station.platforms_.insert({relation.get_value_by_key("ref"), relation.id()});
                map.insert({relation.get_value_by_key("name"), temp_station});
            }
            return 1;
        }
        return 0;
    }
}