#pragma once

#include "GeocodingEngine.hpp"

#include <string>
#include <utility>
#include <vector>

std::pair<double, double> getCentroid(const std::vector<std::pair<double, double>>& coordinates);

// returns centreline_id that matches the given street name and house number, or -1 if not found
int find_centre_line(const std::vector<Centreline>& centrelines, int house_number);
std::vector<Centreline> find_street_segments(const std::string& street_name, const GeocodingEngine& engine);
std::pair<double, double> geocode_address(const std::string& address, const GeocodingEngine& engine);