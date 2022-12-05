#include "stations/preprocessing/preprocessing.h"
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"
#include <iostream>

using namespace stations::preprocessing;
int main() {
    auto result = std::make_shared<preprocessing_result>();
    auto success = map_stations(result);

    {
        boost::filesystem::path areaFile{boost::filesystem::current_path() / "area.dat"};
        boost::filesystem::ofstream ofstream{areaFile};
        /*for (const auto &item: result->stations_) {
            ofstream << item.first << ": ";
            for (auto const &platform: item.second.platforms_) {
                ofstream << platform.first << ": " << platform.second << " ";
            }
            ofstream << "\n";
        }*/
        for (const auto &item: result->platforms_) {
            ofstream << item.second.name_ << " " << item.second.osm_id_ << " " << item.first << "\n";
        }
    }

    //std::cout << result.stations_.count("Frankfurt (Main) Hauptbahnhof") << " ";
    //std::cout << result.stations_.at("Frankfurt (Main) Hauptbahnhof").platforms_.at("16;17");
    if(!result->successful()) {
        std::cerr << "ERROR: Mapping stations unsuccessful!" << std::endl;
    }
    while (true) {
        std::string name;
        std::string platform;
        std::cout << "name: ";
        std::getline(std::cin, name);
        if (name == "exit") {
            break;
        }
        std::cout << "\nplatform: ";
        std::getline(std::cin, platform);
        std::cout << "\n";
        std::cout << name << " " << platform << " " << result->get_id(name, platform) << "\n";
    }
}