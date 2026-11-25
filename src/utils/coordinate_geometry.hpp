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
 double calculate_distance(double lon1, double lat1, double lon2, double lat2);

struct Pt { double x, y; };
/* 
 * Calculates the orientation of three points.
 * @param a The first point.
 * @param b The second point.
 * @param c The third point.
 * @return The orientation of the three points.
 */
double orient(Pt a, Pt b, Pt c);

/**
 * Checks if a point is on a line segment.
 * @param p The first endpoint of the line segment.
 * @param q The point to check.
 * @param r The second endpoint of the line segment.
 * @param eps The epsilon value for floating-point comparison.
 * @return True if the point is on the line segment, false otherwise.
 */
bool onSegment(Pt p, Pt q, Pt r, double eps = 1e-12);

/**
 * Checks if two line segments intersect.
 * @param p1 The first endpoint of the first line segment.
 * @param p2 The second endpoint of the first line segment.
 * @param q1 The first endpoint of the second line segment.
 * @param q2 The second endpoint of the second line segment.
 * @param eps The epsilon value for floating-point comparison.
 * @return True if the line segments intersect, false otherwise.
 */
bool segmentsIntersect(Pt p1, Pt p2, Pt q1, Pt q2, double eps = 1e-12);