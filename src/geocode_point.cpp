#include "geocode_point.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <limits>
#include "GeocodingEngine.hpp"
#include "parse_address.hpp"
#include "utils/coordinate_geometry.hpp"

// returns centreline that matches the given street name and house number, or -1 if not found
Centreline find_centre_line(const std::vector<Centreline>& centrelines, int address_number) {
    for (size_t i = 0; i < centrelines.size(); ++i) {
        const Centreline& entry = centrelines[i];
        // std::cout << "Centreline ID: " << entry.centreline_id << "\n";
        // std::cout << "Street Name: " << entry.street_name << "\n";
        // std::cout << "Left Range: [" << entry.lo_num_l << ", " << entry.hi_num_l << "]\n";
        // std::cout << "Right Range: [" << entry.lo_num_r << ", " << entry.hi_num_r << "]\n";
        // std::cout << "Coordinates (" << entry.coordinates.size() << " points):\n";
        // for (const auto& coord : entry.coordinates) {
        //     std::cout << "  Lon: " << coord.first << ", Lat: " << coord.second << "\n";
        // }
        // std::cout << "----------------------------------------\n";

        // Check if the house number falls within either the left or right range
        if ((address_number >= entry.lo_num_l && address_number <= entry.hi_num_l) ||
            (address_number >= entry.lo_num_r && address_number <= entry.hi_num_r)) {
            //std::cout << "Match found for house number " << address_number << " in centreline ID " << entry.centreline_id << "\n";
            return entry;
        }
    }
    Centreline notFound;
    notFound.centreline_id = -1; // Indicate not found
    return notFound;
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
std::pair<AddressPoint, AddressPoint> find_closest_address_points(const std::vector<AddressPoint>& addressPoints, int target_number) {
    if (addressPoints.empty()) {
        return {AddressPoint(), AddressPoint()};
    }

    AddressPoint closestLower, closestHigher;
    closestLower.address_number = -1; // Initialize to an invalid number
    closestHigher.address_number = -1; // Initialize to an invalid number
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

/**
 * Finds the longitude and latitude for a target address number between two address points.
 * @param lowerPoint The lower address point.
 * @param upperPoint The upper address point.
 * @param target_number The target address number.
 * @return A pair of the longitude and latitude.
 */
std::pair<double, double> find_lon_lat(const AddressPoint& lowerPoint, const AddressPoint& upperPoint, int target_number) {
    // Simple linear interpolation
    double fraction = static_cast<double>(target_number - lowerPoint.address_number) / (upperPoint.address_number - lowerPoint.address_number);
    double lon = lowerPoint.lon + fraction * (upperPoint.lon - lowerPoint.lon);
    double lat = lowerPoint.lat + fraction * (upperPoint.lat - lowerPoint.lat);
    return {lon, lat};
}

/**
 * Finds the longitude and latitude for a target address number using a centreline entry.
 * @param centreline The centreline entry.
 * @param target_number The target address number.
 * @return A pair of the longitude and latitude.
 */
std::pair<double, double> find_lon_lat(const Centreline& centreline, int target_number) {
    double total_segment_length = 0.0;
    std::vector<double> segment_lengths;

    for (size_t i = 1; i < centreline.coordinates.size(); ++i) {
        double lon1 = centreline.coordinates[i - 1].first;
        double lat1 = centreline.coordinates[i - 1].second;
        double lon2 = centreline.coordinates[i].first;
        double lat2 = centreline.coordinates[i].second;

        double segment_length = calculate_distance(lon1, lat1, lon2, lat2);
        segment_lengths.push_back(segment_length);
        total_segment_length += segment_length;
    }

    int lo_num = std::min(centreline.lo_num_l, centreline.lo_num_r);
    int hi_num = std::max(centreline.hi_num_l, centreline.hi_num_r);
    double fraction = static_cast<double>(target_number - lo_num) / (hi_num - lo_num);

    // Need to find the point along the total line segment that corresponds to this fraction of the total length
    double target_distance = fraction * total_segment_length;
    double accumulated_distance = 0.0;
    for (size_t i = 0; i < segment_lengths.size(); ++i) {
        if (accumulated_distance + segment_lengths[i] >= target_distance) {
            double remaining_distance = target_distance - accumulated_distance;
            double segment_fraction = remaining_distance / segment_lengths[i];

            double lon1 = centreline.coordinates[i].first;
            double lat1 = centreline.coordinates[i].second;
            double lon2 = centreline.coordinates[i + 1].first;
            double lat2 = centreline.coordinates[i + 1].second;

            double lon = lon1 + segment_fraction * (lon2 - lon1);
            double lat = lat1 + segment_fraction * (lat2 - lat1);
            return {lon, lat};
        }
        accumulated_distance += segment_lengths[i];
    }
    return {0.0, 0.0};
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

    Centreline matched_centreline;;
    if (parsed.address_number.has_value()) {
        std::vector<Centreline> centrelines = find_street_segments(parsed.street_name, engine);
        matched_centreline = find_centre_line(centrelines, parsed.address_number.value());
        //matched_centreline = centrelines.empty() ? -1 : centrelines[0].centreline_id;
    }else{
        std::cout << "No house number provided in the address.\n";
        return {0.0, 0.0}; // Placeholder for actual geocoded coordinates
    }



    if (matched_centreline.centreline_id != -1) {
        std::cout << "Matched Centreline ID: " << matched_centreline.centreline_id << "\n";
        std::vector<AddressPoint> addressPoints = engine.findAddressPointsByCentreline(matched_centreline.centreline_id);
        if (!addressPoints.empty()) {
            std::cout << "Found " << addressPoints.size() << " address points for centreline ID " << matched_centreline.centreline_id << ":\n";
            std::pair<AddressPoint, AddressPoint> closestPoints = find_closest_address_points(addressPoints, parsed.address_number.value());

            std::pair<double, double> geocodedLocation;
            if (closestPoints.first.address_number != -1 && closestPoints.second.address_number != -1) {
                std::cout << "Closest Lower Address Point: " << closestPoints.first.address_number << " at (" << closestPoints.first.lon << ", " << closestPoints.first.lat << ")\n";
                std::cout << "Closest Higher Address Point: " << closestPoints.second.address_number << " at (" << closestPoints.second.lon << ", " << closestPoints.second.lat << ")\n";

                geocodedLocation = find_lon_lat(closestPoints.first, closestPoints.second, parsed.address_number.value());
                std::cout << "Geocoded Location: (" << geocodedLocation.first << ", " << geocodedLocation.second << ")\n";
                return geocodedLocation;
            } else {
                std::cout << "Could not find valid closest address points for interpolation.\n";
                geocodedLocation = find_lon_lat(matched_centreline, parsed.address_number.value());
            }
        } else {
            std::cout << "No address points found for centreline ID " << matched_centreline.centreline_id << ".\n";
            // TODO: If no address points are found, we must divide the entire line segment into equal parts and return the midpoint of the segment as the geocoded location
        }

    } else {
        std::cout << "No matching centreline found for the given address.\n";
    }
    return {0.0, 0.0}; // Placeholder for actual geocoded coordinates
}