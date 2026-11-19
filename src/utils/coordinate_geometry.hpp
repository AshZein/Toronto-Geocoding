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