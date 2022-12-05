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
#include "osmium/index/map/flex_mem.hpp"
#include "boost/filesystem/fstream.hpp"
#include <iostream>
#include <utility>

using namespace boost;
using namespace osmium;
namespace stations::preprocessing {
std::vector<std::pair<stations::location, platform>> extract_stations(std::string const& osm_file);

    osmium::geom::Coordinates calc_center(osmium::NodeRefList const& nr_list) {
        osmium::geom::Coordinates c{0.0, 0.0};

        for (auto const& nr : nr_list) {
            c.x += nr.lon();
            c.y += nr.lat();
        }

        c.x /= nr_list.size();
        c.y /= nr_list.size();

        return c;
    }

    class station_handler : public osmium::handler::Handler {
    public:
        explicit station_handler(std::vector<std::pair<stations::location, platform>>& platforms, TagsFilter const& filter)
                : platforms_{platforms}, filter_{filter} {}

        void node(osmium::Node const& node) {
            auto const& tags = node.tags();
            if (tags::match_any_of(tags, filter_)) {
                add_platform(osm_type::NODE, node.id(), node.location(), tags);
            }
            /*auto const& tags = node.tags();
            if (tags.has_tag("amenity", "parking")) {
                add_parking(osm_type::NODE, node.id(), node.location(), tags);
            }*/
        }

        void area(osmium::Area const& area) {
            auto const& tags = area.tags();
            if (tags::match_any_of(tags, filter_)) {
                add_platform(area.from_way() ? osm_type::WAY : osm_type::RELATION,
                             area.orig_id(),
                             calc_center(*area.cbegin<osmium::OuterRing>()), tags);
            }
            /*auto const& tags = area.tags();
            if (tags.has_tag("amenity", "parking")) {
                add_parking(area.from_way() ? osm_type::WAY : osm_type::RELATION,
                            area.orig_id(),
                            calc_center(*area.cbegin<osmium::OuterRing>()), tags);
            }*/
        }

    private:
        void add_platform(osm_type const ot, osmium::object_id_type const id,
                         osmium::geom::Coordinates const& coord,
                         osmium::TagList const& tags) {
            std::string in_name;
            if (tags.has_key("name")) {
                in_name = tags.get_value_by_key("name");
            }
            if (tags.has_key("ref_name")) {
                in_name = tags.get_value_by_key("ref_name");
            }
            if (tags.has_key("ref")) {
                in_name = tags.get_value_by_key("ref");
            }
            std::vector<std::string> names{};
            boost::split(names, in_name, [](char c) {
                return c == ';' || c == '-' || c == '/';
            });
            bool only_one = false;
            if (std::any_of(names.begin(), names.end(),[&] (std::string const& name) -> bool {
                return name.length() > 3;
            })) {
                only_one = true;
            }
            if (only_one) {
                platforms_.emplace_back(std::pair{make_location(coord.x, coord.y),
                                                  platform{id, in_name, ot}});
                return;
            }
            for (auto const& name : names) {
                platforms_.emplace_back(std::pair{make_location(coord.x, coord.y),
                                                  platform{id, name, ot}});
            }
        }

        std::vector<std::pair<stations::location, platform>>& platforms_;
        TagsFilter const& filter_;
    };

int map_stations(std::shared_ptr<preprocessing_result>& result) {
    auto file_name = "Frankfurt.osm.pbf";
    //auto const infile = osmium::io::File(R"(E:\motis\data\germany-latest.osm.pbf)");
    auto const infile = osmium::io::File("Frankfurt.osm.pbf");
    auto platforms = extract_stations(file_name);
    result->platforms_ = platforms;
    result->insert();
    std::vector<preprocessing_result::station_rtree_value_type> results;
    result->station_tree_->query(bgi::nearest(make_location(9.0268444, 50.1804567), 2),
                                 std::back_inserter(results));
    if (results.empty()) {
        std::cout << "none found\n";
    }
    for (auto const& result_ : results) {
        std::cout << result_.second << result_.first;
    }
    /*using index_type = index::map::SparseMemArray<unsigned_object_id_type, Location>;
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
    //auto result_ = std::make_shared<preprocessing_result>();

    my_handler handler{filter, result, platforms_};
    area::Assembler::config_type assembler_config;
    area::MultipolygonManager<area::Assembler> mp_manager{assembler_config, filter};
    multipolygon_way_manager mp_way_manager{filter, result, platforms_};
    //relations::read_relations(infile, mp_way_manager);
    {
        osmium::io::Reader reader{infile, osm_entity_bits::way | osm_entity_bits::node | osm_entity_bits::area | osm_entity_bits::relation};
        //apply(reader, mp_way_manager.handler());
        while(auto buffer = reader.read()) {
            apply(buffer, mp_manager, mp_way_manager);
            //apply(buffer, location_handler, handler);
        }
        reader.close();
        mp_manager.prepare_for_lookup();
        mp_way_manager.prepare_for_lookup();
    }

    {
        osmium::io::Reader reader{infile, osm_entity_bits::way | osm_entity_bits::node | osm_entity_bits::area | osm_entity_bits::relation};
        while(auto buffer = reader.read()) {
            //apply(buffer, mp_manager, mp_way_manager);
            //apply(buffer, location_handler, handler);
            apply(
                    buffer, location_handler, handler,
                    mp_way_manager.handler([&handler](memory::Buffer&& buffer) {
                        apply(buffer, handler);
                    }));
            //apply(buffer, location_handler, handler);
        }
        reader.close();
        //mp_manager.prepare_for_lookup();
        //mp_way_manager.prepare_for_lookup();
    }
    //result = mp_way_manager.result_.get();*/
    return 0;
}

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

    std::vector<std::pair<stations::location, platform>> extract_stations(std::string const& osm_file) {
        osmium::io::File const input_file{osm_file};

        osmium::area::Assembler::config_type assembler_config;
        assembler_config.create_empty_areas = false;
        osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{
                assembler_config};
        TagsFilter filter{false};
        filter.add_rule(true, "public_transport", "platform");
        filter.add_rule(true, "railway", "platform");
        filter.add_rule(true, "highway", "platform");
        filter.add_rule(true, "public_transport", "stop_area");
        //filter.add_rule(true, "public_transport", "stop_area_group");
        filter.add_rule(true, "type", "public_transport");

        std::clog << "Extract OSM platforms: Pass 1..." << std::endl;
        osmium::relations::read_relations(input_file, mp_manager);

        index_type index;
        location_handler_type location_handler{index};
        std::vector<std::pair<stations::location, platform>> platforms;
        station_handler data_handler{platforms, filter};

        std::clog << "Extract OSM platforms: Pass 2..." << std::endl;
        osmium::io::Reader reader{input_file, osmium::io::read_meta::no};
        osmium::apply(reader, location_handler, data_handler,
                mp_manager.handler([&data_handler](const osmium::memory::Buffer& area_buffer) {
                osmium::apply(area_buffer, data_handler);}));

        reader.close();
        return platforms;
    }
} // stations::preprocessing