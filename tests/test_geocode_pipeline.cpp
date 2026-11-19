// tests/test_geocode_pipeline.cpp
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "GeocodingEngine.hpp"
#include "geocode_point.hpp"
#include "parse_address.hpp"
#include "utils/coordinate_geometry.hpp"

static bool approxEqual(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

static void testAddressParsing() {
    ParsedAddress p1 = parseStreetAddress("123 King St W");
    assert(p1.address_number.has_value() && *p1.address_number == 123);
    assert(p1.street_name == "KING ST W");

    ParsedAddress p2 = parseStreetAddress("  #4B 100 King Street West., Toronto, ON ");
    assert(p2.address_number.has_value() && *p2.address_number == 100);
    assert(p2.unit == "4B");
    assert(p2.street_name == "KING ST W");
    assert(p2.leftover == "Toronto, ON");

    ParsedAddress p3 = parseStreetAddress("200-204 Yonge Avenue");
    assert(p3.address_number.has_value() && *p3.address_number == 200);
    assert(p3.street_name == "YONGE AVE");

    ParsedAddress p4 = parseStreetAddress("Yonge St");
    assert(!p4.address_number.has_value());
    assert(p4.street_name == "YONGE ST");

    std::cout << "[PASS] testAddressParsing\n";
}

static void testClosestPointsAndInterpolation() {
    std::vector<AddressPoint> pts = {
        {10, -79.3900, 43.6500, 100},
        {10, -79.3880, 43.6520, 120},
    };

    auto [lo, hi] = find_closest_address_points(pts, 110);
    assert(lo.address_number == 100);
    assert(hi.address_number == 120);

    auto mid = find_lon_lat(lo, hi, 110);
    assert(approxEqual(mid.first, -79.3890));
    assert(approxEqual(mid.second, 43.6510));

    // Out-of-range high side -> closestHigher stays -1
    auto [lo2, hi2] = find_closest_address_points(pts, 150);
    assert(lo2.address_number == 120);
    assert(hi2.address_number == -1);

    std::cout << "[PASS] testClosestPointsAndInterpolation\n";
}

static void testCentrelineSelectionAndFallback() {
    Centreline seg1{
        1001, "KING ST W", 100, 198, 101, 199,
        {{-79.3900, 43.6500}, {-79.3880, 43.6500}}
    };
    Centreline seg2{
        1002, "KING ST W", 200, 298, 201, 299,
        {{-79.3880, 43.6500}, {-79.3860, 43.6500}}
    };

    Centreline matched = find_centre_line({seg1, seg2}, 250);
    assert(matched.centreline_id == 1002);

    Centreline missing = find_centre_line({seg1, seg2}, 999);
    assert(missing.centreline_id == -1);

    // Midpoint along horizontal segment at fraction 0.5
    auto pt = find_lon_lat(seg1, 149);
    assert(approxEqual(pt.first, -79.3890, 1e-4));
    assert(approxEqual(pt.second, 43.6500, 1e-4));

    std::cout << "[PASS] testCentrelineSelectionAndFallback\n";
}

int main() {
    testAddressParsing();
    testClosestPointsAndInterpolation();
    testCentrelineSelectionAndFallback();

    // Optional integration check against real GeoPackage data if present
    GeocodingEngine engine;
    if (engine.initialize("data/toronto_centreline.gpkg", "data/address_points.gpkg")) {
        const char* sampleQueries[] = {
            "123 King St W",
            "100 King Street West",
            "Apt 5 250 Yonge St",
            "King St W",
            "999999 Fake St"
        };
        for (const char* q : sampleQueries) {
            auto coord = geocode_address(q, engine);
            std::cout << "Query \"" << q << "\" -> (" << coord.first << ", " << coord.second << ")\n";
        }
    }

    return 0;
}