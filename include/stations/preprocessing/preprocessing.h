#pragma once
#include <unordered_map>
#include "stations/preprocessing/station.h"
#include "stations/common/mlock.h"
#include "boost/geometry.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "boost/geometry/geometries/geometries.hpp"
#include "boost/geometry/geometries/point_xy.hpp"
#include "boost/interprocess/managed_mapped_file.hpp"
#include "geo/point_rtree.h"
#include <vector>
#include <functional>
#include <memory>
#include "stations/preprocessing/platform.h"
#include "boost/geometry/geometries/register/point.hpp"
#include "stations/common/location.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(
        stations::location, double,
        bg::cs::geographic<bg::degree>,
        stations::location::lon, stations::location::lat, stations::location::set_lon,
        stations::location::set_lat)

namespace stations::preprocessing {


template <typename Value>
struct r_tree_data {
    using params_t = bgi::rstar<16>;
    using indexable_t = bgi::indexable<Value>;
    using equal_to_t = bgi::equal_to<Value>;
    using allocator_t = boost::interprocess::allocator<
            Value, boost::interprocess::managed_mapped_file::segment_manager>;
    using rtree_t = bgi::rtree<Value, params_t, indexable_t,
            equal_to_t, allocator_t>;

    void open(std::string const& filename, std::size_t size) {
        file_ = boost::interprocess::managed_mapped_file(
                boost::interprocess::open_or_create, filename.c_str(), size);
        alloc_ = std::make_unique<allocator_t>(file_.get_segment_manager());
        filename_ = filename;
        max_size_ = size;
    }

    void load_or_construct(
            std::function<std::vector<Value>()> const& create_entries) {
        auto r = file_.find_no_lock<rtree_t>("rtree");
        if (r.first == nullptr) {
            rtree_ = file_.construct<rtree_t>("rtree")(
                    create_entries(), params_t(), indexable_t(), equal_to_t(), *alloc_);
            file_.flush();
            file_ = {};
            boost::interprocess::managed_mapped_file::shrink_to_fit(
                    filename_.c_str());
            open(filename_, max_size_);
            load_or_construct(create_entries);
        } else {
            rtree_ = r.first;
        }
    }

    bool lock() { return lock_memory(file_.get_address(), file_.get_size()); }

    bool unlock() { return unlock_memory(file_.get_address(), file_.get_size()); }

    void prefetch() {
        if (lock()) {
            unlock();
        } else {
            char c = 0;
            auto const* base = reinterpret_cast<char*>(file_.get_address());
            for (std::size_t i = 0U; i < file_.get_size(); i += 4096) {
                c += base[i];
            }
            volatile char cs = c;
            (void)cs;
        }
    }

    bool initialized() const { return rtree_ != nullptr && !rtree_->empty(); }

    rtree_t const* operator->() const { return rtree_; }
    rtree_t* operator->() { return rtree_; }

    boost::interprocess::managed_mapped_file file_;
    std::unique_ptr<allocator_t> alloc_;
    rtree_t* rtree_{nullptr};
    std::string filename_;
    std::size_t max_size_{};
};

struct preprocessing_result {
    using rtree_point_type = stations::location;
    //using rtree_box_type = bg::model::box<rtree_point_type>;
    using station_rtree_value_type = std::pair<rtree_point_type, platform>;

    bool successful() {
        return successful_ || !stations_.empty() || !station_tree_->empty();
    }

    int64_t get_id(std::string const& station, std::string const& platform) {
        if (stations_.count(station) > 0) {
            return stations_.find(station)->second.get_id(platform);
        }
        return -1;
    }

    void insert() {
        if (station_tree_.initialized()) {
            return;
        }
        station_tree_.open("station-graph.srt", platforms_.size());
        station_tree_.load_or_construct([&] () {return platforms_;});
    }

    /*void create_area_rtree(std::string const& filename, std::size_t size) {
        if (station_tree_.initialized()) {
            return;
        }
        station_tree_.open(filename, size);
        station_tree_.load_or_construct(
                [&]() { return create_area_rtree_entries(); });
    }

    std::vector<station_rtree_value_type> create_area_rtree_entries() const {
        std::vector<station_rtree_value_type> values;
        values.reserve(platforms_.size());
        for (auto const& platform_ : platforms_) {
            auto box = platform_.first;
                    //boost::geometry::return_envelope<rtree_box_type>(platform_.first);
            values.emplace_back(box, platform_);
        }
        return values;
    }*/

    bool successful_{false};
    int num_entries_{0};
    std::unordered_map<std::string, station> stations_{};
    r_tree_data<station_rtree_value_type> station_tree_{};
    std::vector<std::pair<stations::location, platform>> platforms_;
};

int map_stations(std::shared_ptr<preprocessing_result>& result);
} // stations::preprocessing