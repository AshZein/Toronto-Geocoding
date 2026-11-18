#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <iostream>
#include <vector>
#include <string>

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

StreetNetworkSoA loadCentreline(const std::string& gpkgPath) {
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset*) GDALOpenEx(gpkgPath.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL);
    
    StreetNetworkSoA net;
    if (poDS == nullptr) {
        std::cerr << "Error: Failed to open GeoPackage: " << gpkgPath << std::endl;
        return net;
    }

    OGRLayer* poLayer = poDS->GetLayer(0);
    if (poLayer == nullptr) {
        std::cerr << "Error: Failed to get layer from GeoPackage." << std::endl;
        GDALClose(poDS);
        return net;
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
        const char* streetName = (idx_street >= 0) ? poFeature->GetFieldAsString(idx_street) : "";
        
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
                    net.centreline_ids.push_back(id);
                    net.street_names.push_back(name ? name : "");
                    net.lo_num_l.push_back(ll);
                    net.hi_num_l.push_back(hl);
                    net.lo_num_r.push_back(lr);
                    net.hi_num_r.push_back(hr);
                    net.segment_offsets.push_back(current_offset);

                    for (int i = 0; i < numPoints; ++i) {
                        net.coords_lon.push_back(poLine->getX(i));
                        net.coords_lat.push_back(poLine->getY(i));
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
    return net;
}

int main() {
    std::string gpkgPath = "data/toronto_centreline.gpkg"; // Path to your GeoPackage file
    StreetNetworkSoA streetNetwork = loadCentreline(gpkgPath);


    // Example: Print the first 15 centreline segments
    for (size_t i = 0; i < std::min(streetNetwork.centreline_ids.size(), size_t(15)); ++i) {
        std::cout << "Centreline ID: " << streetNetwork.centreline_ids[i]
                  << ", Street Name: " << streetNetwork.street_names[i]
                  << ", Lo Num L: " << streetNetwork.lo_num_l[i]
                  << ", Hi Num L: " << streetNetwork.hi_num_l[i]
                  << ", Lo Num R: " << streetNetwork.lo_num_r[i]
                  << ", Hi Num R: " << streetNetwork.hi_num_r[i] << std::endl;
    }

    return 0;
}