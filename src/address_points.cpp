#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct AddressPoint {
    int centreline_id;
    double lon;
    double lat;
    int address_number;
};

// One giant flat vector of all address points in Toronto, sorted by centreline_id
std::vector<AddressPoint> all_addresses;

void ingestAddressPoints(const std::string& gpkgPath) {
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
                all_addresses.push_back(addrPoint);
            }
        }
        OGRFeature::DestroyFeature(poFeature);
    }

    GDALClose(poDS);
}

int main() {
    std::string gpkgPath = "data/address_points.gpkg"; // Path to your GeoPackage file
    ingestAddressPoints(gpkgPath);

    // Example: Print the first 5 address points
    for (size_t i = 0; i < std::min(all_addresses.size(), size_t(5)); ++i) {
        const AddressPoint& addr = all_addresses[i];
        std::cout << "Centreline ID: " << addr.centreline_id
                  << ", Lon: " << addr.lon
                  << ", Lat: " << addr.lat
                  << ", Address Number: " << addr.address_number << std::endl;
    }

    return 0;
}