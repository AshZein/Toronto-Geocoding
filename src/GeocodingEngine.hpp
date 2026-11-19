#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct StreetNetworkSoA {
    std::vector<int64_t> centreline_ids;
    std::vector<std::string> street_names;
    std::vector<int> lo_num_l;
    std::vector<int> hi_num_l;
    std::vector<int> lo_num_r;
    std::vector<int> hi_num_r;
    
    // Flat coordinate arrays and offset markers for line segments
    std::vector<double> coords_lon;
    std::vector<double> coords_lat;
    std::vector<int> segment_offsets;
};

struct AddressPoint {
    int centreline_id;
    double lon;
    double lat;
    int address_number;
};

struct Centreline {
    int64_t centreline_id;
    std::string street_name;
    int lo_num_l;
    int hi_num_l;
    int lo_num_r;
    int hi_num_r;
    std::vector<std::pair<double, double>> coordinates; // list of segments, pairs of (lon, lat)
};

class GeocodingEngine {
private:
    StreetNetworkSoA net_;
    std::vector<AddressPoint> addresses_;
    std::unordered_map<std::string, std::vector<int>> street_index_;

    std::vector<int64_t> unique_cids_;          // Sorted unique CENTRELINE_IDs (size K)
    std::vector<uint32_t> cid_addr_offsets_;    // CSR offsets into cid_addr_indices_ (size K + 1)
    std::vector<uint32_t> cid_addr_indices_;    // Contiguous indices into addresses_ (size N)

public:
    GeocodingEngine() = default;

    // Initialize and load both datasets
    bool initialize(const std::string& centrelineGpkg, const std::string& addressGpkg);

    std::vector<Centreline> findStreetSegments(const std::string& rawStreetName) const;

    std::vector<AddressPoint> findAddressPointsByCentreline(int64_t targetCentrelineId) const;

private:
    void loadCentrelines(const std::string& gpkgPath);
    void loadAddressPoints(const std::string& gpkgPath);
    void buildStreetIndex();
    void buildAddressIndex();
};