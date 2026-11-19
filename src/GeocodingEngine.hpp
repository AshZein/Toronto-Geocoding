#pragma once

#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <iomanip>

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

    public:
        GeocodingEngine() = default;
        // Initialize and load both datasets
        bool initialize(const std::string& centrelineGpkg, const std::string& addressGpkg) {
            std::cout << "Initializing Geocoding Engine...\n";
            
            loadCentrelines(centrelineGpkg);
            loadAddressPoints(addressGpkg);
            
            if (net_.street_names.empty()) {
                std::cerr << "Error: Street network failed to load.\n";
                return false;
            }

            buildStreetIndex();
            std::cout << "Engine ready! Loaded " << net_.street_names.size() 
                    << " segments and " << addresses_.size() << " address points.\n";
            return true;
        }

        void findStreetSegments(const std::string& rawStreetName) const {
            // Normalize input query to uppercase
            // Normalize input query to uppercase
    std::string query = rawStreetName;
    std::transform(query.begin(), query.end(), query.begin(),
        [](unsigned char c) { return std::toupper(c); });

    auto it = street_index_.find(query);
    if (it == street_index_.end()) {
        std::cout << "Street not found: " << query << "\n";
        return;
    }

    const std::vector<int>& segmentIndices = it->second;
    std::cout << "Found " << segmentIndices.size()
              << " matching segments for \"" << query << "\"\n";

    for (size_t matchIdx = 0; matchIdx < segmentIndices.size(); ++matchIdx) {
        int segIdx = segmentIndices[matchIdx];

        if (segIdx < 0 || static_cast<size_t>(segIdx) >= net_.centreline_ids.size()) {
            std::cout << "  [" << matchIdx << "] Invalid SoA index: " << segIdx << "\n";
            continue;
        }

        size_t uSegIdx = static_cast<size_t>(segIdx);

        int64_t cid = net_.centreline_ids[uSegIdx];
        const std::string& name = (uSegIdx < net_.street_names.size())
            ? net_.street_names[uSegIdx]
            : query;

        int loL = (uSegIdx < net_.lo_num_l.size()) ? net_.lo_num_l[uSegIdx] : 0;
        int hiL = (uSegIdx < net_.hi_num_l.size()) ? net_.hi_num_l[uSegIdx] : 0;
        int loR = (uSegIdx < net_.lo_num_r.size()) ? net_.lo_num_r[uSegIdx] : 0;
        int hiR = (uSegIdx < net_.hi_num_r.size()) ? net_.hi_num_r[uSegIdx] : 0;

        size_t startCoord = 0;
        size_t endCoord = net_.coords_lon.size();

        if (uSegIdx < net_.segment_offsets.size() && net_.segment_offsets[uSegIdx] >= 0) {
            startCoord = std::min(static_cast<size_t>(net_.segment_offsets[uSegIdx]), net_.coords_lon.size());
        }
        if (uSegIdx + 1 < net_.segment_offsets.size() && net_.segment_offsets[uSegIdx + 1] >= 0) {
            endCoord = std::min(static_cast<size_t>(net_.segment_offsets[uSegIdx + 1]), net_.coords_lon.size());
        }
        if (endCoord < startCoord) {
            endCoord = startCoord;
        }

        size_t numPoints = endCoord - startCoord;

        std::cout << "\n--- Match #" << (matchIdx + 1) << " (SoA index: " << segIdx << ") ---\n"
                  << "  CENTRELINE_ID : " << cid << "\n"
                  << "  STREET_NAME   : " << name << "\n"
                  << "  LEFT  Range   : [" << loL << " .. " << hiL << "]\n"
                  << "  RIGHT Range   : [" << loR << " .. " << hiR << "]\n"
                  << "  Coord Slice   : offset [" << startCoord << " .. " << endCoord
                  << ") (" << numPoints << " vertices)\n";

        std::cout << std::fixed << std::setprecision(6);
        for (size_t v = startCoord; v < endCoord; ++v) {
            double lon = net_.coords_lon[v];
            double lat = (v < net_.coords_lat.size()) ? net_.coords_lat[v] : 0.0;
            std::cout << "    v[" << v << "] (local " << (v - startCoord) << "): "
                      << "lon=" << lon << ", lat=" << lat << "\n";
        }
    }
        }

    private:
        void loadCentrelines(const std::string& gpkgPath) {
            GDALAllRegister();
            GDALDataset* poDS = (GDALDataset*) GDALOpenEx(gpkgPath.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL);
            
            if (poDS == nullptr) {
                std::cerr << "Error: Failed to open GeoPackage: " << gpkgPath << std::endl;
                return;
            }

            OGRLayer* poLayer = poDS->GetLayer(0);
            if (poLayer == nullptr) {
                std::cerr << "Error: Failed to get layer from GeoPackage." << std::endl;
                GDALClose(poDS);
                return;
            }

            poLayer->ResetReading();

            // Cache field indices
            int idx_id     = poLayer->GetLayerDefn()->GetFieldIndex("CENTRELINE_ID");
            int idx_street = poLayer->GetLayerDefn()->GetFieldIndex("LINEAR_NAME_FULL");
            int idx_lo_l   = poLayer->GetLayerDefn()->GetFieldIndex("LO_NUM_L");
            int idx_hi_l   = poLayer->GetLayerDefn()->GetFieldIndex("HI_NUM_L");
            int idx_lo_r   = poLayer->GetLayerDefn()->GetFieldIndex("LO_NUM_R");
            int idx_hi_r   = poLayer->GetLayerDefn()->GetFieldIndex("HI_NUM_R");

            OGRFeature* poFeature;
            int current_offset = 0;

            while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
                int64_t segId = (idx_id >= 0 && !poFeature->IsFieldNull(idx_id)) ? poFeature->GetFieldAsInteger64(idx_id) : 0;
                std::string streetNameStorage = (idx_street >= 0) ? poFeature->GetFieldAsString(idx_street) : "";
                char* streetName = streetNameStorage.data();
                std::transform(streetName, streetName + streetNameStorage.size(), streetName,
                    [](unsigned char c){return std::toupper(c);} );

                int loL = (idx_lo_l >= 0 && !poFeature->IsFieldNull(idx_lo_l)) ? poFeature->GetFieldAsInteger(idx_lo_l) : 0;
                int hiL = (idx_hi_l >= 0 && !poFeature->IsFieldNull(idx_hi_l)) ? poFeature->GetFieldAsInteger(idx_hi_l) : 0;
                int loR = (idx_lo_r >= 0 && !poFeature->IsFieldNull(idx_lo_r)) ? poFeature->GetFieldAsInteger(idx_lo_r) : 0;
                int hiR = (idx_hi_r >= 0 && !poFeature->IsFieldNull(idx_hi_r)) ? poFeature->GetFieldAsInteger(idx_hi_r) : 0;

                OGRGeometry* poGeom = poFeature->GetGeometryRef();
                if (poGeom != nullptr) {
                    OGRwkbGeometryType geomType = wkbFlatten(poGeom->getGeometryType());
                    
                    auto extractLinePoints = [&](OGRLineString* poLine, int64_t id, const char* name, int ll, int hl, int lr, int hr) {
                        int numPoints = poLine->getNumPoints();
                        if (numPoints > 0) {
                            net_.centreline_ids.push_back(id);
                            net_.street_names.push_back(name ?name : "");
                            net_.lo_num_l.push_back(ll);
                            net_.hi_num_l.push_back(hl);
                            net_.lo_num_r.push_back(lr);
                            net_.hi_num_r.push_back(hr);
                            net_.segment_offsets.push_back(current_offset);

                            for (int i = 0; i < numPoints; ++i) {
                                net_.coords_lon.push_back(poLine->getX(i));
                                net_.coords_lat.push_back(poLine->getY(i));
                            }
                            current_offset += numPoints;
                        }
                    };

                    if (geomType == wkbLineString) {
                        extractLinePoints((OGRLineString*)poGeom, segId, streetName, loL, hiL, loR, hiR);
                    }
                    else if (geomType == wkbMultiLineString) {
                        OGRMultiLineString* poMultiLine = (OGRMultiLineString*)poGeom;
                        for (int j = 0; j < poMultiLine->getNumGeometries(); ++j) {
                            OGRGeometry* subGeom = poMultiLine->getGeometryRef(j);
                            if (subGeom != nullptr && wkbFlatten(subGeom->getGeometryType()) == wkbLineString) {
                                extractLinePoints((OGRLineString*)subGeom, segId, streetName, loL, hiL, loR, hiR);
                            }
                        }
                    }
                }

                OGRFeature::DestroyFeature(poFeature);
            }

            GDALClose(poDS);
            return;
        }
        void loadAddressPoints(const std::string& gpkgPath) {
            GDALAllRegister();
            GDALDataset* poDS = (GDALDataset*)GDALOpenEx(gpkgPath.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
            if (poDS == nullptr) {
                std::cerr << "Failed to open GeoPackage: " << gpkgPath << std::endl;
                return;
            }

            OGRLayer* poLayer = poDS->GetLayer(0);
            if (poLayer == nullptr) {
                std::cerr << "Failed to get layer 'address_points' from GeoPackage." << std::endl;
                GDALClose(poDS);
                return;
            }

            poLayer->ResetReading();
            OGRFeature* poFeature;
            while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
                AddressPoint addrPoint;
                addrPoint.centreline_id = poFeature->GetFieldAsInteger("CENTRELINE_ID");
                addrPoint.address_number = poFeature->GetFieldAsInteger("ADDRESS_NUMBER");

                OGRGeometry* poGeometry = poFeature->GetGeometryRef();
                if (poGeometry != nullptr) {
                    OGRwkbGeometryType geomType = wkbFlatten(poGeometry->getGeometryType());
                    bool extracted = false;

                    if (geomType == wkbPoint) {
                        OGRPoint* poPoint = poGeometry->toPoint();
                        if (poPoint != nullptr) {
                            addrPoint.lon = poPoint->getX();
                            addrPoint.lat = poPoint->getY();
                            extracted = true;
                        }
                    }
                    else if (geomType == wkbMultiPoint) {
                        OGRMultiPoint* poMultiPoint = (OGRMultiPoint*)poGeometry;
                        if (poMultiPoint->getNumGeometries() > 0) {
                            OGRGeometry* subGeom = poMultiPoint->getGeometryRef(0);
                            if (subGeom != nullptr && wkbFlatten(subGeom->getGeometryType()) == wkbPoint) {
                                OGRPoint* poPoint = (OGRPoint*)subGeom;
                                addrPoint.lon = poPoint->getX();
                                addrPoint.lat = poPoint->getY();
                                extracted = true;
                            }
                        }
                    }

                    if (extracted) {
                        addresses_.push_back(addrPoint);
                    }
                }
                OGRFeature::DestroyFeature(poFeature);
            }

            GDALClose(poDS);
        }

        void buildStreetIndex() {
            for (size_t i = 0; i < net_.street_names.size(); ++i) {
                street_index_[net_.street_names[i]].push_back(static_cast<int>(i));
            }
        }
};