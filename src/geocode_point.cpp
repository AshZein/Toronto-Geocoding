#include <vector>
#include <string>
#include <iostream>
#include "GeocodingEngine.hpp"

std::pair<double, double> getCentroid(const std::vector<std::pair<double, double>>& coordinates) {
    if (coordinates.empty()) {
        return {0.0, 0.0};
    }

    double sumLon = 0.0;
    double sumLat = 0.0;

    for (const auto& coord : coordinates) {
        sumLon += coord.first;
        sumLat += coord.second;
    }

    size_t count = coordinates.size();
    return {sumLon / count, sumLat / count};
}


int find_centre_line(std::vector<Centreline> centrelines){
    return 0;
}

std::vector<std::string> find_street_segments(std::string street_name){
    return {};
}

std::pair<double, double> geocode_address(std::string address){
    return {0.0, 0.0};
}