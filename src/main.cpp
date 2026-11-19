#include "GeocodingEngine.hpp"
#include "geocode_point.hpp"
#include <iostream>

int main() {
    GeocodingEngine engine;
    if (!engine.initialize("data/toronto_centreline.gpkg", "data/address_points.gpkg")) {
        return 1;
    }

    // Test a quick query lookup
    // std::vector<Centreline> results = engine.findStreetSegments("King St W");
    // std::cout << "Found " << results.size() << " matching segments for \"King St W\"\n";
    // std::cout << "----------------------------------------\n";
    // std::cout << "Centreline ID | Street Name | Lo Num L | Hi Num L | Lo Num R | Hi Num R | Coordinates\n";
    // std::cout << "----------------------------------------\n";
    // for (const auto& entry : results) {
    //     std::cout << entry.centreline_id << " | "
    //               << entry.street_name << " | "
    //               << entry.lo_num_l << " | "
    //               << entry.hi_num_l << " | "
    //               << entry.lo_num_r << " | "
    //               << entry.hi_num_r << " | "
    //               << entry.coordinates.size() << " points\n";
    // }

    std::pair<double, double> location = geocode_address("123 King St W", engine);
    std::cout << "Geocoded location: (" << location.first << ", " << location.second << ")\n";
    return 0;
}