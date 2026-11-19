#include <iostream>
#include <cmath>    
#include <numbers>  
#include "coordinate_geometry.hpp"
/*
 * Calculates the distance between two points on the Earth's surface, we use the haversine distance 
 as the centreline dataset uses geographic coordinates (longitude, latitude) in degrees. The haversine f
 ormula calculates the great-circle distance between two points on a sphere given their longitudes and latitudes.
 * @param lon1 The longitude of the first point.
 * @param lat1 The latitude of the first point.
 * @param lon2 The longitude of the second point.
 * @param lat2 The latitude of the second point.
 * @return The distance in meters.
 */
double calculate_distance(double lon1, double lat1, double lon2, double lat2) {
    // Convert degrees to radians
    const double degToRad = M_PI / 180.0;
    double lat1Rad = lat1 * degToRad;
    double lat2Rad = lat2 * degToRad;
    double deltaLat = (lat2 - lat1) * degToRad;
    double deltaLon = (lon2 - lon1) * degToRad;

    // Haversine formula
    double a = sin(deltaLat / 2) * sin(deltaLat / 2) +
               cos(lat1Rad) * cos(lat2Rad) *
               sin(deltaLon / 2) * sin(deltaLon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    // Earth's radius in meters
    const double earthRadius = 6371000.0;
    return earthRadius * c;
}