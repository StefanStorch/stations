#include "stations/preprocessing/preprocessing.h"
#include <osmium/io/file.hpp>
#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/location.hpp>
#include "osmium/geom/coordinates.hpp"
#include "osmium/area/assembler.hpp"
#include "osmium/handler/node_locations_for_ways.hpp"
#include "osmium/relations/relations_manager.hpp"
#include "osmium/visitor.hpp"
#include "osmium/area/multipolygon_manager.hpp"
#include "osmium/io/pbf_input.hpp"
#include "osmium/index/map/sparse_mem_array.hpp"
#include "osmium/index/map/flex_mem.hpp"
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
        void area(const Area &area) {
            handle_object(area, area.envelope().bottom_left());
            std::cout << "JAMAN\n";
            if (tags::match_any_of(area.tags(), filter_)) {
                areaofstream << area_counter++ << ". ";
                for (auto const &tag: area.tags()) {
                    areaofstream << tag << "\n";
                }
                areaofstream << "data:\n";
                areaofstream << area.id() << " ";
                for (auto const &ring: area.outer_rings()) {
                    for (auto const node: ring) {
                        areaofstream << node.ref() << " ";
                    }
                }
                areaofstream << "\n";
            }
        }

        void relation(const Relation &relation) {
            //auto const& loc = handle_relation(relation);
            //handle_object(relation, loc);
            if (tags::match_any_of(relation.tags(), filter_)) {
                relationofstream << relation_counter++ << ". ";
                for (auto const &tag: relation.tags()) {
                    relationofstream << tag << "\n";
                }
                relationofstream << "data:\n";
                relationofstream << relation.id() << " ";
                for (auto const &member: relation.members()) {
                    auto const &obj = member.get_object();
                    relationofstream << member.type() << member.ref() << " ";
                }
                relationofstream << "\n";
            }
        }

        void node(const Node &node) {
            handle_object(node, node.location());
            if (tags::match_any_of(node.tags(), filter_)) {
                nodeofstream << node_counter++ << ". ";
                for (auto const &tag: node.tags()) {
                    nodeofstream << tag << "\n";
                }
                nodeofstream << "\n";
            }
        }

        void way(const Way &way) {
            handle_object(way, way.nodes().begin()->location());
            if (tags::match_any_of(way.tags(), filter_)) {
                wayofstream << way_counter++ << ". ";
                for (auto const &tag: way.tags()) {
                    wayofstream << tag << "\n";
                }
                wayofstream << "data:\n";
                for (auto const &node: way.nodes()) {
                    wayofstream << node.ref() << " ";
                }
                wayofstream << "\n";
            }
        }

        void handle_object(OSMObject const &obj, osmium::Location const &loc) const {
            if (platforms_->count(obj.id()) > 0 || obj.type() == osmium::item_type::area) {
                auto &name = platforms_->at(obj.id());
                if (strcmp(name.c_str(), "") == 0) {
                    if (obj.tags().has_key("description")) {
                        name = obj.get_value_by_key("description");
                    }
                    if (obj.tags().has_key("name")) {
                        name = obj.get_value_by_key("name");
                    }
                    if (obj.tags().has_key("ref_name")) {
                        name = obj.get_value_by_key("ref_name");
                    }
                }
                if (result_->stations_.count(name) > 0) {
                    auto station = result_->stations_.find(name);
                    if (obj.tags().has_key("ref")) {
                        result_->platforms_.emplace_back(
                                std::pair{make_location(loc.x(), loc.y()), platform{obj.id(),
                                                                                    obj.get_value_by_key("ref")}});
                        station->second.add_platform(obj.get_value_by_key("ref"), obj.id());
                    } else {
                        result_->platforms_.emplace_back(
                                std::pair{make_location(loc.x(), loc.y()), platform{obj.id(), "-1"}});
                        station->second.add_platform("-1", obj.id());
                    }
                } else {
                    auto temp_station = station{};
                    if (obj.tags().has_key("ref")) {
                        result_->platforms_.emplace_back(
                                std::pair{make_location(loc.x(), loc.y()), platform{obj.id(),
                                                                                    obj.get_value_by_key("ref")}});
                        temp_station.add_platform(obj.get_value_by_key("ref"), obj.id());
                    } else {
                        result_->platforms_.emplace_back(
                                std::pair{make_location(loc.x(), loc.y()), platform{obj.id(), "-1"}});
                        temp_station.add_platform("-1", obj.id());
                    }
                    result_->stations_.insert({name, temp_station});
                }
                platforms_->erase(obj.id());
            }
        }

        osmium::Location handle_relation(Relation const &rel) {
            osmium::Location to_return{};
            for (auto const &mem: rel.cmembers()) {
                switch (mem.type()) {
                    case osmium::item_type::node:
                        break;
                    case osmium::item_type::way:
                        break;
                    case osmium::item_type::relation:
                        break;
                    default:
                        break;
                }
            }
            return to_return;
        }

        explicit my_handler(TagsFilter const &filter,
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
        TagsFilter const &filter_;
        std::shared_ptr<std::unordered_map<int64_t, std::string>> platforms_{};
        std::shared_ptr<preprocessing_result> result_{};
    };

    struct multipolygon_way_manager
            : public relations::RelationsManager<multipolygon_way_manager,
                    true, true, true> {

        multipolygon_way_manager(TagsFilter const &filter,
                                 std::shared_ptr<preprocessing_result> result,
                                 std::shared_ptr<std::unordered_map<int64_t, std::string>> platforms)
                : filter_(filter), result_(std::move(result)), platforms_(std::move(platforms)) {}

        bool new_relation(Relation const &relation) noexcept {
            //return relation.tags().has_tag("type", "multipolygon") &&
            //           tags::match_any_of(relation.tags(), filter_);
            return tags::match_any_of(relation.tags(), filter_);
        }

        bool new_member(const Relation &relation,
                        const RelationMember &member,
                        std::size_t /*n*/) const noexcept {
            // TODO: maybe expand upon this
            if (strcmp(member.role(), "platform") == 0) {
                // TODO: name is not always correct multiple stations where name=Bahnhof 12421950, 11503383
                // ref_name could be used
                if (relation.tags().has_key("ref_name")) {
                    platforms_->insert({member.ref(), relation.get_value_by_key("ref_name")});
                } else {
                    if (relation.tags().has_key("name")) {
                        platforms_->insert({member.ref(), relation.get_value_by_key("name")});
                    } else {
                        platforms_->insert({member.ref(), ""});
                    }
                }
            }
            return true;
        }

        void complete_relation(const Relation &relation) {
            relationofstream_ << "Relation ID: " << relation.id() << "\n";
            relationofstream_ << "Relation TAGS:\n";
            for (auto const &tag: relation.tags()) {
                relationofstream_ << tag << "\n";
            }
            auto success = handle_stations(relation);
            result_->num_entries_ += success;
            for (auto const &member: relation.members()) {
                if (member.ref() != 0) {
                    auto const obj = this->get_member_object(member);
                    if (obj->tags().has_key("public_transport")) {
                        relationofstream_ << "object ID: " << relation.id() << "\n";
                        relationofstream_ << "object TAGS:\n";
                        for (auto const &tag: obj->tags()) {
                            relationofstream_ << tag << "\n";
                        }
                    }
                }
            }
        }

        int handle_stations(Relation const &relation) {
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

        int handle_stop_area_group(Relation const &relation) {
            int count = 0;
            for (auto const &member: relation.members()) {
                if (strcmp(member.role(), "railway_station") == 0) {
                    count += handle_railway_station(member);
                }
                if (strcmp(member.role(), "bus_station") == 0) {
                    count += handle_bus_station(member);
                }
            }
            return count;
        }

        int handle_railway_station(RelationMember const &sub_relation) {
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

        int handle_bus_station(RelationMember const &sub_relation) {
            // TODO: implement
            return 0;
        }

        int handle_platform(Relation const &relation, RelationMember const &platform_member, station &station) {
            if (platform_member.ref() != 0) {
                //platforms_->insert({platform_member.ref(), relation.get_value_by_key("name")});
                //can't access members of sub relations so this does not work
                /*auto obj = member_relations_database().get_object(platform_member.ref());
                station.platforms_.insert({obj->get_value_by_key("ref"), obj->id()});*/
                return 1;
            }
            return 0;
        }

        TagsFilter const &filter_;
        filesystem::path relationFile_{filesystem::current_path() / "relation.dat"};
        filesystem::ofstream relationofstream_{relationFile_};
        std::shared_ptr<preprocessing_result> result_{};
        std::shared_ptr<std::unordered_map<int64_t, std::string>> platforms_{};
    };
}