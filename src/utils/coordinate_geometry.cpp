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

/* 
 * Calculates the orientation of three points.
 * @param a The first point.
 * @param b The second point.
 * @param c The third point.
 * @return The orientation of the three points.
 */
double orient(Pt a, Pt b, Pt c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/**
 * Checks if a point is on a line segment.
 * @param p The first endpoint of the line segment.
 * @param q The point to check.
 * @param r The second endpoint of the line segment.
 * @param eps The epsilon value for floating-point comparison.
 * @return True if the point is on the line segment, false otherwise.
 */
bool onSegment(Pt p, Pt q, Pt r, double eps) {
    return q.x <= std::max(p.x, r.x) + eps && q.x + eps >= std::min(p.x, r.x) &&
           q.y <= std::max(p.y, r.y) + eps && q.y + eps >= std::min(p.y, r.y);
}

/**
 * Checks if two line segments intersect.
 * @param p1 The first endpoint of the first line segment.
 * @param p2 The second endpoint of the first line segment.
 * @param q1 The first endpoint of the second line segment.
 * @param q2 The second endpoint of the second line segment.
 * @param eps The epsilon value for floating-point comparison.
 * @return True if the line segments intersect, false otherwise.
 */
bool segmentsIntersect(Pt p1, Pt p2, Pt q1, Pt q2, double eps) {
    double o1 = orient(p1, p2, q1);
    double o2 = orient(p1, p2, q2);
    double o3 = orient(q1, q2, p1);
    double o4 = orient(q1, q2, p2);

    if (((o1 > eps && o2 < -eps) || (o1 < -eps && o2 > eps)) &&
        ((o3 > eps && o4 < -eps) || (o3 < -eps && o4 > eps))) {
        return true;
    }

    if (std::abs(o1) <= eps && onSegment(p1, q1, p2, eps)) return true;
    if (std::abs(o2) <= eps && onSegment(p1, q2, p2, eps)) return true;
    if (std::abs(o3) <= eps && onSegment(q1, p1, q2, eps)) return true;
    if (std::abs(o4) <= eps && onSegment(q1, p2, q2, eps)) return true;

    return false;
}