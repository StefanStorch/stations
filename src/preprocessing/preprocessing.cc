#include "stations/preprocessing/preprocessing.h"
//#include "stations/preprocessing/handle_stations.h"

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
#include <utility>

using namespace boost;
using namespace osmium;
namespace stations::preprocessing {

struct my_handler : public handler::Handler {
public:
    void area(const Area& area) {
        if (tags::match_any_of(area.tags(), filter_)) {
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

    void relation(const Relation& relation) {
        if (platforms_->count(relation.id()) > 0) {
            auto const& name = platforms_->at(relation.id());
            if (result_->stations_.count(name) > 0) {
                auto station = result_->stations_.find(name);
                if (relation.tags().has_key("ref")) {
                    station->second.add_platform(relation.get_value_by_key("ref"), relation.id());
                } else {
                    station->second.add_platform("-1", relation.id());
                }
            } else {
                auto temp_station = station{};
                if (relation.tags().has_key("ref")) {
                    temp_station.add_platform(relation.get_value_by_key("ref"), relation.id());
                } else {
                    temp_station.add_platform("-1", relation.id());
                }
                result_->stations_.insert({name, temp_station});
            }
            platforms_->erase(relation.id());
        }
        if (tags::match_any_of(relation.tags(), filter_)) {
            relationofstream << relation_counter++ << ". ";
            for (auto const& tag : relation.tags()) {
                relationofstream << tag << "\n";
            }
            relationofstream << "data:\n";
            relationofstream << relation.id() << " ";
            for(auto const& member : relation.members()) {
                auto const& obj = member.get_object();
                relationofstream << member.type() << member.ref() << " ";
            }
            relationofstream << "\n";
        }
    }

    void node(const Node& node) {
        if (platforms_->count(node.id()) > 0) {
            auto const& name = platforms_->at(node.id());
            if (result_->stations_.count(name) > 0) {
                auto station = result_->stations_.find(name);
                if (node.tags().has_key("ref")) {
                    station->second.add_platform(node.get_value_by_key("ref"), node.id());
                } else {
                    station->second.add_platform("-1", node.id());
                }
            } else {
                auto temp_station = station{};
                if (node.tags().has_key("ref")) {
                    temp_station.add_platform(node.get_value_by_key("ref"), node.id());
                } else {
                    temp_station.add_platform("-1", node.id());
                }
                result_->stations_.insert({name, temp_station});
            }
            platforms_->erase(node.id());
        }
        if (tags::match_any_of(node.tags(), filter_)) {
            nodeofstream << node_counter++ << ". ";
            for(auto const& tag : node.tags()) {
                nodeofstream << tag << "\n";
            }
            nodeofstream << "\n";
        }
    }

    void way(const Way& way) {
        if (platforms_->count(way.id()) > 0) {
            auto const& name = platforms_->at(way.id());
            if (result_->stations_.count(name) > 0) {
                auto station = result_->stations_.find(name);
                if (way.tags().has_key("ref")) {
                    station->second.add_platform(way.get_value_by_key("ref"), way.id());
                } else {
                    station->second.add_platform("-1", way.id());
                }
            } else {
                auto temp_station = station{};
                if (way.tags().has_key("ref")) {
                    temp_station.add_platform(way.get_value_by_key("ref"), way.id());
                } else {
                    temp_station.add_platform("-1", way.id());
                }
                result_->stations_.insert({name, temp_station});
            }
            platforms_->erase(way.id());
        }
        if (tags::match_any_of(way.tags(), filter_)) {
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

    explicit my_handler(TagsFilter const& filter,
                        std::shared_ptr<preprocessing_result> result,
                        std::shared_ptr<std::unordered_map<int64_t, std::string>> platforms) :
                        filter_(filter), result_(std::move(result)), platforms_(std::move(platforms)) {}

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
    TagsFilter const& filter_;
    std::shared_ptr<std::unordered_map<int64_t, std::string>> platforms_{};
    std::shared_ptr<preprocessing_result> result_{};
};

struct multipolygon_way_manager
        : public relations::RelationsManager<multipolygon_way_manager,
                true, true, true> {

    multipolygon_way_manager(TagsFilter const& filter,
                             std::shared_ptr<preprocessing_result> result,
                             std::shared_ptr<std::unordered_map<int64_t, std::string>> platforms)
            : filter_(filter), result_(std::move(result)), platforms_(std::move(platforms)) {}

    bool new_relation(Relation const& relation) noexcept {
        //return relation.tags().has_tag("type", "multipolygon") &&
        //           tags::match_any_of(relation.tags(), filter_);
        return tags::match_any_of(relation.tags(), filter_);
    }

    bool new_member(const Relation& relation,
                    const RelationMember& member,
                    std::size_t /*n*/) noexcept {
        // TODO: maybe expand upon this
        if (strcmp(member.role(), "platform") == 0) {
            platforms_->insert({member.ref(), relation.get_value_by_key("name")});
        }
        return true;
    }

    void complete_relation(const Relation& relation) {
        relationofstream_ << "Relation ID: " << relation.id() << "\n";
        relationofstream_ << "Relation TAGS:\n";
        for (auto const& tag : relation.tags()) {
            relationofstream_ << tag << "\n";
        }
        auto success = handle_stations(relation);
        result_->num_entries_ += success;
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

    int handle_stations(Relation const& relation) {
        int count = 0;
        const auto tags = &relation.tags();
        // TODO: not so sure whether this handles all stations correctly
        if (tags->has_tag("public_transport", "stop_area_group")) {
            count += handle_stop_area_group(relation);
        }
        /*if (tags->has_key("name") && tags->has_key("ref")) {
            if (result_->stations_.count(relation.get_value_by_key("name"))) {
                auto station = result_->stations_.find(relation.get_value_by_key("name"));
                station->second.platforms_.insert({relation.get_value_by_key("ref"), relation.id()});
            } else {
                auto temp_station = station{};
                temp_station.platforms_.insert({relation.get_value_by_key("ref"), relation.id()});
                result_->stations_.insert({relation.get_value_by_key("name"), temp_station});
            }
            return ++count;
        }*/
        return count;
    }

    int handle_stop_area_group(Relation const& relation) {
        int count = 0;
        for (auto const& member : relation.members()) {
            if (strcmp(member.role(), "railway_station") == 0) {
                count += handle_railway_station(member);
            }
            if (strcmp(member.role(), "bus_station") == 0) {
                count += handle_bus_station(member);
            }
        }
        return count;
    }

    int handle_railway_station(RelationMember const& sub_relation) {
        int count = 0;
        const auto obj = this->get_member_object(sub_relation);
        station station{};
        if (result_->stations_.count(obj->get_value_by_key("name")) > 0) {
            station = result_->stations_.find(obj->get_value_by_key("name"))->second;
        } else {
            result_->stations_.insert({obj->get_value_by_key("name"), station});
        }
        if (obj->type() == item_type::relation) {
            const auto relation = this->get_member_relation(sub_relation.ref());
            for (auto const &member: relation->members()) {
                if (strcmp(member.role(), "platform") == 0) {
                    count += handle_platform(*relation, member, station);
                }
            }
        } else {
            if (obj->type() == item_type::way) {
                station.add_platform(obj->get_value_by_key("ref"), obj->id());
            } else {
                station.add_platform("-1", obj->id());
            }
            count++;
        }
        return count;
    }

    int handle_bus_station(RelationMember const& sub_relation) {
        // TODO: implement
        return 0;
    }

    int handle_platform(Relation const& relation, RelationMember const& platform_member, station& station) {
        if (platform_member.ref() != 0) {
            //platforms_->insert({platform_member.ref(), relation.get_value_by_key("name")});
            //can't access members of sub relations so this does not work
            /*auto obj = member_relations_database().get_object(platform_member.ref());
            station.platforms_.insert({obj->get_value_by_key("ref"), obj->id()});*/
            return 1;
        }
        return 0;
    }

    TagsFilter const& filter_;
    filesystem::path relationFile_{filesystem::current_path() / "relation.dat"};
    filesystem::ofstream relationofstream_{relationFile_};
    std::shared_ptr<preprocessing_result> result_{};
    std::shared_ptr<std::unordered_map<int64_t, std::string>> platforms_{};
};

int map_stations(preprocessing_result& result) {
    auto const infile = osmium::io::File("Frankfurt.osm.pbf");

    using index_type = index::map::SparseMemArray<unsigned_object_id_type, Location>;
    using location_handler_type = handler::NodeLocationsForWays<index_type>;

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();
    TagsFilter filter{false};
    filter.add_rule(true, "public_transport", "platform");
    filter.add_rule(true, "railway", "platform");
    filter.add_rule(true, "highway", "platform");
    filter.add_rule(true, "public_transport", "stop_area");
    filter.add_rule(true, "public_transport", "stop_area_group");
    filter.add_rule(true, "type", "public_transport");

    auto platforms_ = std::make_shared<std::unordered_map<int64_t, std::string>>();
    auto result_ = std::make_shared<preprocessing_result>();

    my_handler handler{filter, result_, platforms_};
    area::Assembler::config_type assembler_config;
    //area::MultipolygonManager<area::Assembler> mp_manager{assembler_config, filter};
    multipolygon_way_manager mp_way_manager{filter, result_, platforms_};
    //relations::read_relations(infile, mp_way_manager);
    {
        osmium::io::Reader reader{infile, osm_entity_bits::way | osm_entity_bits::node | osm_entity_bits::area | osm_entity_bits::relation};
        //apply(reader, mp_way_manager.handler());
        while(auto buffer = reader.read()) {
            apply(buffer, mp_way_manager);
            //apply(buffer, location_handler, handler);
        }
        reader.close();
        //mp_manager.prepare_for_lookup();
        mp_way_manager.prepare_for_lookup();
    }

    {
        osmium::io::Reader reader{infile, osm_entity_bits::way | osm_entity_bits::node | osm_entity_bits::area | osm_entity_bits::relation};
        while(auto buffer = reader.read()) {
            //apply(buffer, mp_manager, mp_way_manager);
            //apply(buffer, location_handler, handler);
            apply(
                    buffer,
                    mp_way_manager.handler([&handler](memory::Buffer&& buffer) {
                        apply(buffer, handler);
                    }),
                    location_handler, handler);
            //apply(buffer, location_handler, handler);
        }
        reader.close();
        //mp_manager.prepare_for_lookup();
        //mp_way_manager.prepare_for_lookup();
    }
    result = *mp_way_manager.result_;
    return 0;
}
} // stations::preprocessing