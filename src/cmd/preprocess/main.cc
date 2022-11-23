#include "stations/preprocessing/preprocessing.h"
#include <iostream>

using namespace stations::preprocessing;
int main() {
    preprocessing_result result;
    auto success = map_stations(result);
    std::cout << result.stations_.count("Frankfurt (Main) Hauptbahnhof");
    std::cout << result.stations_.at("Frankfurt (Main) Hauptbahnhof").platforms_.at("16;17");
    if(!result.successful()) {
        std::cerr << "ERROR: Mapping stations unsuccessful!" << std::endl;
    }
}