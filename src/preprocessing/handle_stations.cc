#include "stations/preprocessing/handle_stations.h"

namespace stations::preprocessing {
    int handle_stop_area_group(osmium::Relation const& relation, std::unordered_map<std::string, station> &map);
    int handle_railway_station(osmium::Relation const& relation, std::unordered_map<std::string, station> &map,
                               osmium::RelationMember const& member);
    int handle_bus_station(osmium::Relation const& relation, std::unordered_map<std::string, station> &map,
                               osmium::RelationMember const& member);

    int handle_stations(osmium::Relation const& relation, std::unordered_map<std::string, station>& map) {
        int count = 0;
        const auto tags = &relation.tags();
        if (tags->has_key("stop_area_group")) {
            count += handle_stop_area_group(relation, map);
        }
        if (tags->has_key("name") && tags->has_key("ref")) {
            if (map.count(relation.get_value_by_key("name"))) {
                auto station = map.find(relation.get_value_by_key("name"));
                station->second.add_platform(relation.get_value_by_key("ref"), relation.id());
            } else {
                auto temp_station = station{};
                temp_station.add_platform(relation.get_value_by_key("ref"), relation.id());
                map.insert({relation.get_value_by_key("name"), temp_station});
            }
            return count;
        }
        return count;
    }

    int handle_stop_area_group(osmium::Relation const& relation, std::unordered_map<std::string, station> &map) {
        int count = 0;
        for (auto const& member : relation.members()) {
            if (strcmp(member.role(), "railway_station") == 0) {
                count += handle_railway_station(relation, map, member);
            }
            if (strcmp(member.role(), "bus_station") == 0) {
                count += handle_bus_station(relation, map, member);
            }
        }
        return count;
    }

    int handle_railway_station(osmium::Relation const& relation, std::unordered_map<std::string, station> &map,
                               osmium::RelationMember const& member) {
        int count = 0;

        return count;
    }

    int handle_bus_station(osmium::Relation const& relation, std::unordered_map<std::string, station> &map,
                           osmium::RelationMember const& member) {
        return 0;
    }
}