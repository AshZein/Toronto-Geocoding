std::pair<double, double> getCentroid(const std::vector<std::pair<double, double>>& coordinates);

// returns centreline_id that matches the given street name and house number, or -1 if not found
int find_centre_line(std::vector<Centreline> centrelines, int house_number);
std::vector<Centreline> find_street_segments(std::string street_name, const GeocodingEngine& engine);
std::pair<double, double> geocode_address(std::string address, const GeocodingEngine& engine)