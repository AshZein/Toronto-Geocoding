// tests/test_geocode_address.cpp
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "GeocodingEngine.hpp"
#include "geocode_point.hpp"
#include "parse_address.hpp"

namespace {

bool approxEqual(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

void runGeocodeAddressTests(const GeocodingEngine& engine) {
    // 1. Exact address point match: 32 Nineteenth St (CENTRELINE_ID 1020891)
    {
        const std::string query = "32 Nineteenth St";
        std::pair<double, double> location = geocode_address(query, engine);
        std::cout << "Query \"" << query << "\" -> ("
                  << location.first << ", " << location.second << ")\n";

        // Note: if lower == upper == 32 in find_lon_lat(AddressPoint, AddressPoint, int),
        // division by zero can yield NaN unless guarded; verify !std::isnan first.
        assert(!std::isnan(location.first) && !std::isnan(location.second));
        assert(approxEqual(location.first, -79.5190367341825, 1e-5));
        assert(approxEqual(location.second, 43.5996761490216, 1e-5));
    }

    // 2. Exact address point match: 5 Nineteenth St (CENTRELINE_ID 1020865)
    {
        const std::string query = "5 Nineteenth St";
        std::pair<double, double> location = geocode_address(query, engine);
        std::cout << "Query \"" << query << "\" -> ("
                  << location.first << ", " << location.second << ")\n";

        assert(!std::isnan(location.first) && !std::isnan(location.second));
        assert(approxEqual(location.first, -79.5180804246363, 1e-5));
        assert(approxEqual(location.second, 43.5987633869987, 1e-5));
    }

    // 3. Normalized/cased street variant
    {
        const std::string query = "  32 nineteenth street  ";
        std::pair<double, double> location = geocode_address(query, engine);
        std::cout << "Query \"" << query << "\" -> ("
                  << location.first << ", " << location.second << ")\n";

        assert(!std::isnan(location.first) && !std::isnan(location.second));
        assert(approxEqual(location.first, -79.5190367341825, 1e-5));
        assert(approxEqual(location.second, 43.5996761490216, 1e-5));
    }

    // 4. Missing house number returns {0.0, 0.0}
    {
        const std::string query = "Nineteenth St";
        std::pair<double, double> location = geocode_address(query, engine);
        std::cout << "Query \"" << query << "\" -> ("
                  << location.first << ", " << location.second << ")\n";

        assert(approxEqual(location.first, 0.0));
        assert(approxEqual(location.second, 0.0));
    }

    std::cout << "[PASS] All geocode_address test cases\n";
}

} // namespace

int main() {
    GeocodingEngine engine;
    if (!engine.initialize("data/toronto_centreline.gpkg", "data/address_points.gpkg")) {
        std::cerr << "Failed to initialize GeocodingEngine\n";
        return 1;
    }

    runGeocodeAddressTests(engine);
    return 0;
}