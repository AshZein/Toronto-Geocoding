#include <vector>
#include <string>
#include <iostream>
#include "GeocodingEngine.hpp"
#include "parse_address.hpp"

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

std::vector<std::Centreline> find_street_segments(std::string street_name){
    std::vector<Centreline> results;
    results = GeocodingEngine::findStreetSegments(street_name);

    return results;
}

std::pair<double, double> geocode_address(std::string address){
    ParsedAddress parsed = parseStreetAddress(address);
    std::cout << "Parsed Address:\n";
    if (parsed.house_number.has_value()) {
        std::cout << "  House Number: " << parsed.house_number.value() << "\n";
    } else {
        std::cout << "  House Number: (not found)\n";
    }
    std::cout << "  Unit: " << parsed.unit << "\n";
    std::cout << "  Street Name: " << parsed.street_name << "\n";
    std::cout << "  Leftover: " << parsed.leftover << "\n";

    find_street_segments(parsed.street_name);
}