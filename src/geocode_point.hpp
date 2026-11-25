#pragma once

#include "GeocodingEngine.hpp"

#include <string>
#include <utility>
#include <vector>

//std::pair<double, double> getCentroid(const std::vector<std::pair<double, double>>& coordinates);

// returns centreline_id that matches the given street name and house number, or -1 if not found
Centreline find_centre_line(const std::vector<Centreline>& centrelines, int house_number);

bool is_centreline_intersecting(const Centreline& centreline1, const Centreline& centreline2);
std::pair<Centreline, Centreline> find_intersecting_centre_lines(const std::vector<Centreline>& centrelines1, const std::vector<Centreline>& centrelines2);
std::vector<Centreline> find_street_segments(const std::string& street_name, const GeocodingEngine& engine);

// Find the lon and lat for a target address number using two bounding address points.
std::pair<AddressPoint, AddressPoint> find_closest_address_points(const std::vector<AddressPoint>& addressPoints, int target_number);
std::pair<double, double> find_lon_lat(const AddressPoint& lowerPoint, const AddressPoint& upperPoint, int target_number);

// Find the lon and lat for a target address number using a centreline entry.
std::pair<double, double> find_lon_lat(const Centreline& centreline, int target_number);

std::pair<double, double> geocode_address(const std::string& address, const GeocodingEngine& engine);
// Find the lon and lat for a target address number using two address strings and a geocoding engine.
std::pair<double, double> geocode_address(const std::string& address1, Direction direction,const std::string& address2, const GeocodingEngine& engine);