#include "GeocodingEngine.hpp"

int main() {
    GeocodingEngine engine;
    if (!engine.initialize("data/toronto_centreline.gpkg", "data/address_points.gpkg")) {
        return 1;
    }

    // Test a quick query lookup
    engine.findStreetSegments("King St W");

    return 0;
}