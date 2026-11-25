#include "geocode_point.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "GeocodingEngine.hpp"
#include "parse_address.hpp"
#include "utils/coordinate_geometry.hpp"

// returns centreline that matches the given street name and house number, or -1 if not found
Centreline find_centre_line(const std::vector<Centreline>& centrelines, int address_number) {
    if (address_number < 0) {
        // If address_number is negative, return the first centreline (or handle as needed)
        if (!centrelines.empty()) {
            return centrelines.front();
        } else {
            Centreline notFound;
            notFound.centreline_id = -1; // Indicate not found
            return notFound;
        }
    }

    for (size_t i = 0; i < centrelines.size(); ++i) {
        const Centreline& entry = centrelines[i];

        // Check if the house number falls within either the left or right range
        if ((address_number >= entry.lo_num_l && address_number <= entry.hi_num_l) ||
            (address_number >= entry.lo_num_r && address_number <= entry.hi_num_r)) {
            return entry;
        }
    }
    Centreline notFound;
    notFound.centreline_id = -1; // Indicate not found
    return notFound;
}
 /**
 * Checks if two centre lines intersect using the segment intersection test.
 * @param centreline1 The first centre line.
 * @param centreline2 The second centre line.
 * @return True if the centre lines intersect, false otherwise.
 */
bool is_centreline_intersecting(const Centreline& centreline1, const Centreline& centreline2) {
    for (size_t i = 1; i < centreline1.coordinates.size(); ++i) {
        Pt p1{centreline1.coordinates[i - 1].first, centreline1.coordinates[i - 1].second};
        Pt p2{centreline1.coordinates[i].first, centreline1.coordinates[i].second};

        for (size_t j = 1; j < centreline2.coordinates.size(); ++j) {
            Pt q1{centreline2.coordinates[j - 1].first, centreline2.coordinates[j - 1].second};
            Pt q2{centreline2.coordinates[j].first, centreline2.coordinates[j].second};

            if (segmentsIntersect(p1, p2, q1, q2)) {
                return true;
            }
        }
    }
    return false;
}
 /**
 * Finds the first pair of intersecting centre lines between two sets of centre lines.
 * @param centrelines1 The first set of centre lines.
 * @param centrelines2 The second set of centre lines.
 * @return A pair of the intersecting centre lines, or a pair of invalid centre lines if none found.
 */
std::pair<Centreline, Centreline> find_intersecting_centre_lines(const std::vector<Centreline>& centrelines1, const std::vector<Centreline>& centrelines2) {
    for (const auto& centreline1 : centrelines1) {
        for (const auto& centreline2 : centrelines2) {
            
            if (is_centreline_intersecting(centreline1, centreline2)) {
                return {centreline1, centreline2};
            }
        }
    }
    return {Centreline{-1}, Centreline{-1}}; // Return invalid centrelines if no intersection found
}

std::vector<Centreline> find_street_segments(const std::string& street_name, const GeocodingEngine& engine) {
    return engine.findStreetSegments(street_name);
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
    closestLower.address_number = -1;  // Initialize to an invalid number
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
    int span = upperPoint.address_number - lowerPoint.address_number;
    if (span == 0) {
        return {lowerPoint.lon, lowerPoint.lat};
    }

    double fraction = static_cast<double>(target_number - lowerPoint.address_number) /
                      static_cast<double>(span);
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
    if (centreline.coordinates.empty()) {
        return {0.0, 0.0};
    }
    if (centreline.coordinates.size() == 1) {
        return centreline.coordinates.front();
    }

    double total_segment_length = 0.0;
    std::vector<double> segment_lengths;
    segment_lengths.reserve(centreline.coordinates.size() - 1);

    // calculate the length of each segment and the total length of the centreline
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
    int span = hi_num - lo_num;
    double fraction = (span != 0)
        ? static_cast<double>(target_number - lo_num) / static_cast<double>(span)
        : 0.0;
    fraction = std::clamp(fraction, 0.0, 1.0);

    if (total_segment_length <= 0.0) {
        return centreline.coordinates.front();
    }

    double target_distance = fraction * total_segment_length;
    double accumulated_distance = 0.0;
    for (size_t i = 0; i < segment_lengths.size(); ++i) {
        if (segment_lengths[i] > 0.0 && accumulated_distance + segment_lengths[i] >= target_distance) {
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

    return centreline.coordinates.back();
}

namespace {

// Path 2 & Path 3: When both lower and upper bounding AddressPoints exist,
// check if either endpoint of the matched centreline provides a tighter bound
// on the lower or upper side before linearly interpolating.
std::pair<double, double> interpolateWithTightestBounds(
    const Centreline& centreline,
    const AddressPoint& lowerPoint,
    const AddressPoint& upperPoint,
    int target_number
) {
    int bound_lo = lowerPoint.address_number;
    double lon_lo = lowerPoint.lon;
    double lat_lo = lowerPoint.lat;

    int bound_hi = upperPoint.address_number;
    double lon_hi = upperPoint.lon;
    double lat_hi = upperPoint.lat;

    if (centreline.coordinates.size() >= 2) {
        int seg_lo = std::min(centreline.lo_num_l, centreline.lo_num_r);
        int seg_hi = std::max(centreline.hi_num_l, centreline.hi_num_r);

        const auto& start_coord = centreline.coordinates.front();
        const auto& end_coord   = centreline.coordinates.back();

        // Tighten lower bound if segment start is closer to target_number than lowerPoint
        if (seg_lo > bound_lo && seg_lo <= target_number) {
            bound_lo = seg_lo;
            lon_lo = start_coord.first;
            lat_lo = start_coord.second;
        }

        // Tighten upper bound if segment end is closer to target_number than upperPoint
        if (seg_hi < bound_hi && seg_hi >= target_number) {
            bound_hi = seg_hi;
            lon_hi = end_coord.first;
            lat_hi = end_coord.second;
        }
    }

    int span = bound_hi - bound_lo;
    if (span == 0) {
        return {lon_lo, lat_lo};
    }

    double fraction = static_cast<double>(target_number - bound_lo) / static_cast<double>(span);
    double lon = lon_lo + fraction * (lon_hi - lon_lo);
    double lat = lat_lo + fraction * (lat_hi - lat_lo);
    return {lon, lat};
}

} // namespace

/*
 * Geocode an address given two address strings and a geocoding engine.
 * adress1: The primary address string (e.g., "100 KING ST W").
 * engine: The geocoding engine to use for finding street segments and address points.
 * Returns a pair of longitude and latitude for the given address.
 */
std::pair<double, double> geocode_address(const std::string& address1, const GeocodingEngine& engine) {
    ParsedAddress parsed = parseStreetAddress(address1);

    // If address number is not provided, we can still attempt to find the centreline
    if (!parsed.address_number.has_value()) {
        return {0.0, 0.0}; // Return a default value or handle as needed
    }

    int target_number = parsed.address_number.value();
    std::vector<Centreline> centrelines = find_street_segments(parsed.street_name, engine);
    Centreline matched_centreline = find_centre_line(centrelines, target_number);

    if (matched_centreline.centreline_id == -1) {
        std::cout << "No matching centreline found for the given address.\n";
        return {0.0, 0.0};
    }

    std::vector<AddressPoint> addressPoints = engine.findAddressPointsByCentreline(matched_centreline.centreline_id);

    // Path 1: No address points on this centreline -> interpolate along centreline
    if (addressPoints.empty()) {
        return find_lon_lat(matched_centreline, target_number);
    }

    auto [closestLower, closestHigher] = find_closest_address_points(addressPoints, target_number);

    // Path 1: Missing lower or upper bounding address point -> fall back to centreline
    if (closestLower.address_number == -1 || closestHigher.address_number == -1) {
        return find_lon_lat(matched_centreline, target_number);
    }

    // Path 2 & Path 3: Both lower and upper address points exist; tighten either
    // bound using the segment endpoints if a segment endpoint is tighter.
    return interpolateWithTightestBounds(matched_centreline, closestLower, closestHigher, target_number);
}


/*
 * Geocode an address given two address strings and a geocoding engine.
 * 
 * adress1: The primary address string (e.g., "100 KING ST W").
 * address2: An optional secondary address string (e.g., "Eglington AVE").
 * engine: The geocoding engine to use for finding street segments and address points.
 * direction: The direction of the address (e.g., "N", "S", "E", "W").
 * address2: An optional secondary address string (e.g., "Eglington AVE").
 * engine: The geocoding engine to use for finding street segments and address points.
 * Returns a pair of longitude and latitude for the given address.
 */
std::pair<double, double> geocode_address(const std::string& address1, Direction direction, const std::string& address2, const GeocodingEngine& engine) {
    ParsedAddress parsed_1 = parseStreetAddress(address1);
    ParsedAddress parsed_2 = parseStreetAddress(address2);

    // address1 has an address number, so we can use it directly for geocoding
    if (parsed_1.address_number.has_value()) {
        return geocode_address(address1, engine);
    }

    if (parsed_2.address_number.has_value()) {
        return geocode_address(address2, engine);
    }

    std::vector<Centreline> centrelines_1 = find_street_segments(parsed_1.street_name, engine);
    std::vector<Centreline> centrelines_2 = find_street_segments(parsed_2.street_name, engine);

    std::pair<Centreline, Centreline> intersecting_centrelines = find_intersecting_centre_lines(centrelines_1, centrelines_2);

    if(intersecting_centrelines.first.centreline_id == -1 || intersecting_centrelines.second.centreline_id == -1) {
        std::cout << "No intersecting centrelines found for the given addresses.\n";
        return {0.0, 0.0};
    }

    // if (matched_centreline.centreline_id == -1) {
    //     std::cout << "No matching centreline found for the given address.\n";
    //     return {0.0, 0.0};
    // }

    // std::vector<AddressPoint> addressPoints = engine.findAddressPointsByCentreline(matched_centreline.centreline_id);

    // // Path 1: No address points on this centreline -> interpolate along centreline
    // if (addressPoints.empty()) {
    //     return find_lon_lat(matched_centreline, target_number);
    // }

    // auto [closestLower, closestHigher] = find_closest_address_points(addressPoints, target_number);

    // // Path 1: Missing lower or upper bounding address point -> fall back to centreline
    // if (closestLower.address_number == -1 || closestHigher.address_number == -1) {
    //     return find_lon_lat(matched_centreline, target_number);
    // }

    // // Path 2 & Path 3: Both lower and upper address points exist; tighten either
    // // bound using the segment endpoints if a segment endpoint is tighter.
    // return interpolateWithTightestBounds(matched_centreline, closestLower, closestHigher, target_number);

    return {0.0, 0.0}; // Return a default value or handle as needed
}
