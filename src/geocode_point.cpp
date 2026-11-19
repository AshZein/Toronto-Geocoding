#include "geocode_point.hpp"
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

// returns centreline_id that matches the given street name and house number, or -1 if not found
int find_centre_line(const std::vector<Centreline>& centrelines, int address_number) {
    for (size_t i = 0; i < centrelines.size(); ++i) {
        const Centreline& entry = centrelines[i];
        std::cout << "Centreline ID: " << entry.centreline_id << "\n";
        std::cout << "Street Name: " << entry.street_name << "\n";
        std::cout << "Left Range: [" << entry.lo_num_l << ", " << entry.hi_num_l << "]\n";
        std::cout << "Right Range: [" << entry.lo_num_r << ", " << entry.hi_num_r << "]\n";
        std::cout << "Coordinates (" << entry.coordinates.size() << " points):\n";
        for (const auto& coord : entry.coordinates) {
            std::cout << "  Lon: " << coord.first << ", Lat: " << coord.second << "\n";
        }
        std::cout << "----------------------------------------\n";

        // Check if the house number falls within either the left or right range
        if ((address_number >= entry.lo_num_l && address_number <= entry.hi_num_l) ||
            (address_number >= entry.lo_num_r && address_number <= entry.hi_num_r)) {
            std::cout << "Match found for house number " << address_number << " in centreline ID " << entry.centreline_id << "\n";
            return entry.centreline_id;
        }
    }
    return -1;
}

std::vector<Centreline> find_street_segments(const std::string& street_name, const GeocodingEngine& engine){
    std::vector<Centreline> results;
    results = engine.findStreetSegments(street_name);

    return results;
}

/**
 * Finds the closest address points that bound a target address number.
 * @param addressPoints The list of address points to search through.
 * @param target_number The target address number.
 * @return A pair of the closest lower and higher address points.
 */
std::pair<AdressPoint, AddressPoint> find_closest_address_points(const std::vector<AdressPoint>& addressPoints, int target_number) {
    if (addressPoints.empty()) {
        return {AddressPoint(), AddressPoint()};
    }

    AddressPoint closestLower, closestHigher;
    int lowerNumber = std::numeric_limits<int>::min();
    int higherNumber = std::numeric_limits<int>::max();

    for (const auto& point : addressPoints) {
        if (point.address_number <= target_number && point.address_number > lowerNumber) {
            closestLower = point;
            lowerNumber = point.address_number;
        }
        if (point.address_number >= target_number && point.address_number < higherNumber) {
            closestHigher = point;
            higherNumber = point.address_number;
        }
    }
    return {closestLower, closestHigher};
}

std::pair<double, double> geocode_address(const std::string& address, const GeocodingEngine& engine) {
    ParsedAddress parsed = parseStreetAddress(address);
    std::cout << "Parsed Address:\n";
    if (parsed.address_number.has_value()) {
        std::cout << "  House Number: " << parsed.address_number.value() << "\n";
    } else {
        std::cout << "  House Number: (not found)\n";
    }
    std::cout << "  Unit: " << parsed.unit << "\n";
    std::cout << "  Street Name: " << parsed.street_name << "\n";
    std::cout << "  Leftover: " << parsed.leftover << "\n";

    int matched_centreline_id = -1;
    if (parsed.address_number.has_value()) {
        std::vector<Centreline> centrelines = find_street_segments(parsed.street_name, engine);
        matched_centreline_id = find_centre_line(centrelines, parsed.address_number.value());
    }  

    if (matched_centreline_id != -1) {
        std::cout << "Matched Centreline ID: " << matched_centreline_id << "\n";
        std::vector<AddressPoint> addressPoints = engine.findAddressPointsByCentreline(matched_centreline_id);
        if (!addressPoints.empty()) {
            std::cout << "Found " << addressPoints.size() << " address points for centreline ID " << matched_centreline_id << ":\n";
            for (const auto& point : addressPoints) {
                std::cout << "  Address Point: (" << point.lon << ", " << point.lat << ")\n";
                //TODO: Find closest address points to the found address number
            }
        } else {
            std::cout << "No address points found for centreline ID " << matched_centreline_id << ".\n";
            // TODO: If no address points are found, we must divide the entire line segment into equal parts and return the midpoint of the segment as the geocoded location
        }

    } else {
        std::cout << "No matching centreline found for the given address.\n";
    }
    return {0.0, 0.0}; // Placeholder for actual geocoded coordinates
}