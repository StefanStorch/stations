#include "stations/preprocessing/preprocessing.h"
#include "stations/preprocessing/handle_stations.h"

#include <osmium/io/file.hpp>
#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include "osmium/area/assembler.hpp"
#include "osmium/handler/node_locations_for_ways.hpp"
#include "osmium/relations/relations_manager.hpp"
#include "osmium/visitor.hpp"
#include "osmium/area/multipolygon_manager.hpp"
#include "osmium/io/pbf_input.hpp"
#include "osmium/index/map/sparse_mem_array.hpp"
#include "osmium/handler/node_locations_for_ways.hpp"
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/path_traits.hpp"
#include "boost/filesystem/operations.hpp"
#include <iostream>

using namespace boost;
namespace stations::preprocessing {

struct my_handler : public osmium::handler::Handler {
public:
    void area(const osmium::Area& area) {
        if (osmium::tags::match_any_of(area.tags(), filter_)) {
            areaofstream << area_counter++ << ". ";
            for (auto const& tag : area.tags()) {
                areaofstream << tag << "\n";
            }
            areaofstream << "data:\n";
            areaofstream << area.id() << " ";
            for(auto const& ring : area.outer_rings()) {
                for(auto const node : ring) {
                    areaofstream << node.ref() << " ";
                }
            }
            areaofstream << "\n";
        }
    }

    void relation(const osmium::Relation& relation) {
        if (osmium::tags::match_any_of(relation.tags(), filter_)) {
            relationofstream << relation_counter++ << ". ";
            for (auto const& tag : relation.tags()) {
                relationofstream << tag << "\n";
                //std::cout << tag << "\n";
            }
            relationofstream << "data:\n";
            //std::cout << "data:\n";
            relationofstream << relation.id() << " ";
            //std::cout << relation.id() << " ";
            for(auto const& member : relation.members()) {
                auto const& obj = member.get_object();
                relationofstream << member.type() << member.ref() << " ";
                //std::cout << member.type() << member.ref() << " ";
                //std::cout << obj.id();
                /*for (auto const& tag: obj.tags()) {
                    relationofstream << tag << "\n";
                    std::cout << tag << "\n";
                }*/
            }
            relationofstream << "\n";
        }
    }

    void node(const osmium::Node& node) {
        if (osmium::tags::match_any_of(node.tags(), filter_)) {
            nodeofstream << node_counter++ << ". ";
            for(auto const& tag : node.tags()) {
                nodeofstream << tag << "\n";
            }
            nodeofstream << "\n";
        }
    }

    void way(const osmium::Way& way) {
        if (osmium::tags::match_any_of(way.tags(), filter_)) {
            wayofstream << way_counter++ << ". ";
            for(auto const& tag : way.tags()) {
                wayofstream << tag << "\n";
            }
            wayofstream << "data:\n";
            for(auto const& node : way.nodes()) {
                wayofstream << node.ref() << " ";
            }
            wayofstream << "\n";
        }
    }

    my_handler(osmium::TagsFilter const& filter) : filter_(filter) {}

    filesystem::path wayFile{filesystem::current_path() / "way.dat"};
    filesystem::ofstream wayofstream{wayFile};
    filesystem::path nodeFile{filesystem::current_path() / "node.dat"};
    filesystem::ofstream nodeofstream{wayFile};
    filesystem::path relationFile{filesystem::current_path() / "relation2.dat"};
    filesystem::ofstream relationofstream{relationFile};
    filesystem::path areaFile{filesystem::current_path() / "area.dat"};
    filesystem::ofstream areaofstream{areaFile};
    unsigned way_counter{0};
    unsigned node_counter{0};
    unsigned relation_counter{0};
    unsigned area_counter{0};
    osmium::TagsFilter const& filter_;
};

struct multipolygon_way_manager
        : public osmium::relations::RelationsManager<multipolygon_way_manager,
                true, true, true> {

    multipolygon_way_manager(osmium::TagsFilter const& filter,
                             preprocessing_result& result)
            : filter_(filter), result_(result) {}

    bool new_relation(osmium::Relation const& relation) noexcept {
        return relation.tags().has_tag("type", "multipolygon") &&
                   osmium::tags::match_any_of(relation.tags(), filter_);
        //return osmium::tags::match_any_of(relation.tags(), filter_);
    }

    bool new_member(const osmium::Relation& /*relation*/,
                    const osmium::RelationMember& /*member*/,
                    std::size_t /*n*/) noexcept {
        return true;
    }

    void complete_relation(const osmium::Relation& relation) {
        relationofstream_ << "Relation ID: " << relation.id() << "\n";
        relationofstream_ << "Relation TAGS:\n";
        for (auto const& tag : relation.tags()) {
            relationofstream_ << tag << "\n";
        }
        auto success = handle_stations(relation, result_.stations_);
        /*if (relation.tags().has_key("name") && relation.tags().has_key("ref")) {
            if (result_.stations_.count(relation.get_value_by_key("name"))) {
                auto station = result_.stations_.find(relation.get_value_by_key("name"));
                station->second.platforms_.insert({relation.get_value_by_key("ref"), relation.id()});
            } else {
                auto temp_station = station{};
                temp_station.platforms_.insert({relation.get_value_by_key("ref"), relation.id()});
                result_.stations_.insert({relation.get_value_by_key("name"), temp_station});
            }
        }*/
        for (auto const& member : relation.members()) {
            if (member.ref() != 0) {
                auto const obj = this->get_member_object(member);
                if (obj->tags().has_key("public_transport")) {
                    relationofstream_ << "object ID: " << relation.id() << "\n";
                    relationofstream_ << "object TAGS:\n";
                    for (auto const& tag : obj->tags()) {
                        relationofstream_ << tag << "\n";
                    }
                }
            }
        }
    }

    osmium::TagsFilter const& filter_;
    filesystem::path relationFile_{filesystem::current_path() / "relation.dat"};
    filesystem::ofstream relationofstream_{relationFile_};
    preprocessing_result result_{};
};

int map_stations(preprocessing_result& result) {
    auto const infile = osmium::io::File("Frankfurt.osm.pbf");

    using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
    using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();
    osmium::TagsFilter filter{false};
    //filter.add_rule(true, "highway", "pedestrian");
    //filter.add_rule(true, "area:highway", "pedestrian");
    filter.add_rule(true, "public_transport", "platform");
    filter.add_rule(true, "highway", "platform");
    filter.add_rule(true, "public_transport", "stop_area");
    filter.add_rule(true, "type", "public_transport");
    my_handler handler{filter};

    osmium::area::Assembler::config_type assembler_config;
    osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{
            assembler_config, filter};
    multipolygon_way_manager mp_way_manager{filter, result};
    //osmium::relations::read_relations(infile, mp_way_manager);
    {
        osmium::io::Reader reader{infile, osmium::osm_entity_bits::way | osmium::osm_entity_bits::node | osmium::osm_entity_bits::area | osmium::osm_entity_bits::relation};
        //osmium::apply(reader, mp_way_manager.handler());
        while(auto buffer = reader.read()) {
            osmium::apply(buffer, mp_way_manager, mp_manager);
            //osmium::apply(buffer, location_handler, handler);
        }
        reader.close();
        mp_manager.prepare_for_lookup();
        mp_way_manager.prepare_for_lookup();
    }

    {
        osmium::io::Reader reader{infile, osmium::osm_entity_bits::way | osmium::osm_entity_bits::node | osmium::osm_entity_bits::area | osmium::osm_entity_bits::relation};
        while(auto buffer = reader.read()) {
            //osmium::apply(buffer, mp_manager, mp_way_manager);
            //osmium::apply(buffer, location_handler, handler);
            osmium::apply(
                    buffer, location_handler, handler,
                    mp_way_manager.handler([&handler](osmium::memory::Buffer&& buffer) {
                        osmium::apply(buffer, handler);
                    }));
        }
        reader.close();
        //mp_manager.prepare_for_lookup();
        //mp_way_manager.prepare_for_lookup();
    }
    result = mp_way_manager.result_;

    return 0;
}
} // stations::preprocessing