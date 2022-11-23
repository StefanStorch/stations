#pragma once
#include "stations/preprocessing/station.h"
#include "osmium/osm/relation.hpp"

namespace stations::preprocessing {
    int handle_stations(osmium::Relation const& relation, std::unordered_map<std::string, station>& map);
} // stations::preprocessing