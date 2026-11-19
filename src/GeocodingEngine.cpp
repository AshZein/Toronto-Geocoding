#include "GeocodingEngine.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <utility>

#include <gdal_priv.h>
#include <ogrsf_frmts.h>

bool GeocodingEngine::initialize(const std::string& centrelineGpkg, const std::string& addressGpkg) {
    std::cout << "Initializing Geocoding Engine...\n";

    loadCentrelines(centrelineGpkg);
    loadAddressPoints(addressGpkg);
    buildAddressIndex();

    if (net_.street_names.empty()) {
        std::cerr << "Error: Street network failed to load.\n";
        return false;
    }

    buildStreetIndex();
    std::cout << "Engine ready! Loaded " << net_.street_names.size()
              << " segments and " << addresses_.size() << " address points.\n";
    return true;
}

std::vector<Centreline> GeocodingEngine::findStreetSegments(const std::string& rawStreetName) const {
    std::vector<Centreline> results;

    // Normalize input query to uppercase
    std::string query = rawStreetName;
    std::transform(query.begin(), query.end(), query.begin(),
        [](unsigned char c) { return std::toupper(c); });

    auto it = street_index_.find(query);
    if (it == street_index_.end()) {
        std::cout << "Street not found: " << query << "\n";
        return results;
    }

    const std::vector<int>& segmentIndices = it->second;
    results.reserve(segmentIndices.size());

    // std::cout << "Found " << segmentIndices.size()
    //           << " matching segments for \"" << query << "\"\n";

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

        // std::cout << "\n--- Match #" << (matchIdx + 1) << " (SoA index: " << segIdx << ") ---\n"
        //           << "  CENTRELINE_ID : " << cid << "\n"
        //           << "  STREET_NAME   : " << name << "\n"
        //           << "  LEFT  Range   : [" << loL << " .. " << hiL << "]\n"
        //           << "  RIGHT Range   : [" << loR << " .. " << hiR << "]\n"
        //           << "  Coord Slice   : offset [" << startCoord << " .. " << endCoord
        //           << ") (" << numPoints << " vertices)\n";

        Centreline entry;
        entry.centreline_id = cid;
        entry.street_name   = name;
        entry.lo_num_l      = loL;
        entry.hi_num_l      = hiL;
        entry.lo_num_r      = loR;
        entry.hi_num_r      = hiR;
        entry.coordinates.reserve(numPoints);

       // std::cout << std::fixed << std::setprecision(6);
        for (size_t v = startCoord; v < endCoord; ++v) {
            double lon = net_.coords_lon[v];
            double lat = (v < net_.coords_lat.size()) ? net_.coords_lat[v] : 0.0;
            entry.coordinates.emplace_back(lon, lat);
            // std::cout << "    v[" << v << "] (local " << (v - startCoord) << "): "
            //           << "lon=" << lon << ", lat=" << lat << "\n";
        }

        results.push_back(std::move(entry));
    }

    return results;
}

std::vector<AddressPoint> GeocodingEngine::findAddressPointsByCentreline(int64_t targetCentrelineId) const {
    std::vector<AddressPoint> matches;
    if (targetCentrelineId <= 0 || unique_cids_.empty()) {
        return matches;
    }

    auto it = std::lower_bound(unique_cids_.begin(), unique_cids_.end(), targetCentrelineId);
    if (it == unique_cids_.end() || *it != targetCentrelineId) {
        return matches;
    }

    size_t bucketIdx = static_cast<size_t>(std::distance(unique_cids_.begin(), it));
    uint32_t start = cid_addr_offsets_[bucketIdx];
    uint32_t end   = cid_addr_offsets_[bucketIdx + 1];

    matches.reserve(end - start);
    for (uint32_t pos = start; pos < end; ++pos) {
        uint32_t addrIdx = cid_addr_indices_[pos];
        if (addrIdx < addresses_.size()) {
            matches.push_back(addresses_[addrIdx]);
        }
    }

    return matches;
}

void GeocodingEngine::loadCentrelines(const std::string& gpkgPath) {
    GDALAllRegister();
    GDALDataset* poDS = static_cast<GDALDataset*>(
        GDALOpenEx(gpkgPath.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr));

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
        int64_t segId = (idx_id >= 0 && !poFeature->IsFieldNull(idx_id))
            ? poFeature->GetFieldAsInteger64(idx_id) : 0;
        std::string streetNameStorage = (idx_street >= 0)
            ? poFeature->GetFieldAsString(idx_street) : "";
        std::transform(streetNameStorage.begin(), streetNameStorage.end(), streetNameStorage.begin(),
            [](unsigned char c) { return std::toupper(c); });

        int loL = (idx_lo_l >= 0 && !poFeature->IsFieldNull(idx_lo_l)) ? poFeature->GetFieldAsInteger(idx_lo_l) : 0;
        int hiL = (idx_hi_l >= 0 && !poFeature->IsFieldNull(idx_hi_l)) ? poFeature->GetFieldAsInteger(idx_hi_l) : 0;
        int loR = (idx_lo_r >= 0 && !poFeature->IsFieldNull(idx_lo_r)) ? poFeature->GetFieldAsInteger(idx_lo_r) : 0;
        int hiR = (idx_hi_r >= 0 && !poFeature->IsFieldNull(idx_hi_r)) ? poFeature->GetFieldAsInteger(idx_hi_r) : 0;

        OGRGeometry* poGeom = poFeature->GetGeometryRef();
        if (poGeom != nullptr) {
            OGRwkbGeometryType geomType = wkbFlatten(poGeom->getGeometryType());

            auto extractLinePoints = [&](OGRLineString* poLine, int64_t id, const std::string& name,
                                         int ll, int hl, int lr, int hr) {
                int numPoints = poLine->getNumPoints();
                if (numPoints > 0) {
                    net_.centreline_ids.push_back(id);
                    net_.street_names.push_back(name);
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
                extractLinePoints(static_cast<OGRLineString*>(poGeom), segId, streetNameStorage, loL, hiL, loR, hiR);
            } else if (geomType == wkbMultiLineString) {
                OGRMultiLineString* poMultiLine = static_cast<OGRMultiLineString*>(poGeom);
                for (int j = 0; j < poMultiLine->getNumGeometries(); ++j) {
                    OGRGeometry* subGeom = poMultiLine->getGeometryRef(j);
                    if (subGeom != nullptr && wkbFlatten(subGeom->getGeometryType()) == wkbLineString) {
                        extractLinePoints(static_cast<OGRLineString*>(subGeom), segId, streetNameStorage, loL, hiL, loR, hiR);
                    }
                }
            }
        }

        OGRFeature::DestroyFeature(poFeature);
    }

    GDALClose(poDS);
}

void GeocodingEngine::loadAddressPoints(const std::string& gpkgPath) {
    GDALAllRegister();
    GDALDataset* poDS = static_cast<GDALDataset*>(
        GDALOpenEx(gpkgPath.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
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
            } else if (geomType == wkbMultiPoint) {
                OGRMultiPoint* poMultiPoint = static_cast<OGRMultiPoint*>(poGeometry);
                if (poMultiPoint->getNumGeometries() > 0) {
                    OGRGeometry* subGeom = poMultiPoint->getGeometryRef(0);
                    if (subGeom != nullptr && wkbFlatten(subGeom->getGeometryType()) == wkbPoint) {
                        OGRPoint* poPoint = static_cast<OGRPoint*>(subGeom);
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

void GeocodingEngine::buildStreetIndex() {
    for (size_t i = 0; i < net_.street_names.size(); ++i) {
        street_index_[net_.street_names[i]].push_back(static_cast<int>(i));
    }
}

void GeocodingEngine::buildAddressIndex() {
    unique_cids_.clear();
    cid_addr_offsets_.clear();
    cid_addr_indices_.clear();

    const size_t numAddrs = addresses_.size();
    if (numAddrs == 0) {
        cid_addr_offsets_.push_back(0);
        return;
    }

    struct CidAddrPair {
        int64_t cid;
        uint32_t addr_idx;
    };

    std::vector<CidAddrPair> pairs;
    pairs.reserve(numAddrs);

    for (uint32_t i = 0; i < static_cast<uint32_t>(numAddrs); ++i) {
        int64_t cid = static_cast<int64_t>(addresses_[i].centreline_id);
        if (cid > 0) { // Skip unlinked/zero CENTRELINE_IDs
            pairs.push_back({cid, i});
        }
    }

    if (pairs.empty()) {
        cid_addr_offsets_.push_back(0);
        return;
    }

    // Sort by centreline_id; std::sort is fast and keeps equal cids contiguous
    std::sort(pairs.begin(), pairs.end(),
        [](const CidAddrPair& a, const CidAddrPair& b) {
            if (a.cid != b.cid) return a.cid < b.cid;
            return a.addr_idx < b.addr_idx;
        });

    cid_addr_indices_.reserve(pairs.size());
    cid_addr_offsets_.reserve(pairs.size() / 2 + 1);
    cid_addr_offsets_.push_back(0);

    int64_t currentCid = pairs[0].cid;
    unique_cids_.push_back(currentCid);

    for (const auto& p : pairs) {
        if (p.cid != currentCid) {
            cid_addr_offsets_.push_back(static_cast<uint32_t>(cid_addr_indices_.size()));
            currentCid = p.cid;
            unique_cids_.push_back(currentCid);
        }
        cid_addr_indices_.push_back(p.addr_idx);
    }

    cid_addr_offsets_.push_back(static_cast<uint32_t>(cid_addr_indices_.size()));
}