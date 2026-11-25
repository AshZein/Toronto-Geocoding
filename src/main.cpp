#include "GeocodingEngine.hpp"
#include "geocode_point.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <pthread.h>

struct ThreadData {
    GeocodingEngine* engine;
    std::string input_file;
    std::string output_file;
    int thread_id;
};

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
    if (!line.empty() && line.back() == ',') {
        fields.emplace_back();
    }
    return fields;
}

std::string trimCopy(const std::string& s) {
    const size_t first = s.find_first_not_of(" \t\r\n\"");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = s.find_last_not_of(" \t\r\n\"");
    return s.substr(first, last - first + 1);
}

std::string normalizeHeader(const std::string& raw) {
    std::string s = trimCopy(raw);
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
    return trimCopy(s);
}

std::string escapeCsv(const std::string& value) {
    const bool needsQuotes = value.find_first_of(",\"\r\n") != std::string::npos;
    if (!needsQuotes) {
        return value;
    }
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    for (char c : value) {
        if (c == '"') {
            out.push_back('"');
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}
void* workderThreadFunc(void* arg) {
    using clock = std::chrono::steady_clock;
    using ms_double = std::chrono::duration<double, std::milli>;
    // const std::string csvPath =
    //     "data/parking_ticket_example/clean_untouched/Parking_Tags_Data_2008_clean_utf8.csv";
    ThreadData* threadData = static_cast<ThreadData*>(arg);
    std::string csvPath = static_cast<ThreadData*>(arg)->input_file;
    std::string outCsvPath = static_cast<ThreadData*>(arg)->output_file;

    GeocodingEngine& engine = *static_cast<ThreadData*>(arg)->engine;


    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open CSV file: " << csvPath << "\n";
        return reinterpret_cast<void*>(1); // Or: return (void*)1;
    }

    std::string headerLine;
    if (!std::getline(file, headerLine)) {
        std::cerr << "CSV file is empty.\n";
       return reinterpret_cast<void*>(1); // Or: return (void*)1;
    }

    const auto headers = splitCsvLine(headerLine);
    int targetColIdx = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        if (normalizeHeader(headers[i]) == "location2") {
            targetColIdx = static_cast<int>(i);
            break;
        }
    }

    if (targetColIdx < 0) {
        std::cerr << "Could not find 'location2' column in header.\n";
        return reinterpret_cast<void*>(1); // Or: return (void*)1;
    }

    // const std::string outCsvPath =
    //     "data/parking_ticket_example/clean_untouched/Parking_Tags_Data_2008_geocoded.csv";
    std::ofstream outFile(outCsvPath);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open output CSV file: " << outCsvPath << "\n";
        return reinterpret_cast<void*>(1); // Or: return (void*)1;
    }

    outFile << "address,lon,lat,lookup_ms\n";

    size_t processedCount = 0;
    size_t nonZeroCount = 0;
    double totalGeocodeMs = 0.0;

    std::string line;
    while (std::getline(file, line)) {
        const auto cols = splitCsvLine(line);
        if (static_cast<size_t>(targetColIdx) >= cols.size()) {
            continue;
        }
        const std::string addr = trimCopy(cols[static_cast<size_t>(targetColIdx)]);
        if (addr.empty()) {
            continue;
        }

        const auto rowStart = clock::now();
        std::pair<double, double> coords = geocode_address(addr, engine);
        const auto rowEnd = clock::now();

        const double rowMs = ms_double(rowEnd - rowStart).count();
        totalGeocodeMs += rowMs;
        ++processedCount;
        if (coords.first != 0.0 || coords.second != 0.0) {
            ++nonZeroCount;
        }

        outFile << escapeCsv(addr) << ","
                << std::fixed << std::setprecision(6) << coords.first << ","
                << std::fixed << std::setprecision(6) << coords.second << ","
                << std::fixed << std::setprecision(4) << rowMs << "\n";

        if (processedCount % 100000 == 0) {
            std::cout << "Thread " << threadData->thread_id << ": Progress: " << processedCount << " addresses geocoded...\n";
        }
    }

    outFile.flush();

    std::cout << "\nThread " << threadData->thread_id << ": Wrote " << processedCount << " geocoded rows to " << outCsvPath << "\n";
    std::cout << "Processed " << processedCount << " addresses ("
              << nonZeroCount << " non-zero) in "
              << std::fixed << std::setprecision(4) << totalGeocodeMs << " ms";
    if (processedCount > 0) {
        std::cout << " (" << (totalGeocodeMs / static_cast<double>(processedCount)) << " ms/row avg)";
    }
    std::cout << "\n";
    return nullptr;
}


int main() {
    using clock = std::chrono::steady_clock;
    using ms_double = std::chrono::duration<double, std::milli>;

    GeocodingEngine engine;
    if (!engine.initialize("data/toronto_centreline.gpkg", "data/address_points.gpkg")) {
        std::cerr << "Failed to initialize GeocodingEngine.\n";
        return 1;
    }
    const auto Start = clock::now();
    

    
    ThreadData threadData[4];
    pthread_t threads[4];
    for (int i = 0; i < 4; ++i) {
        threadData[i].engine = &engine;
        threadData[i].input_file = "data/parking_ticket_example/2008_partitions/Parking_Tags_Data_2008_clean_utf8_part" + std::to_string(i + 1) + "_of_4.csv";
        threadData[i].output_file = "data/parking_ticket_example/2008_partitions/Parking_Tags_Data_2008_geocoded_part" + std::to_string(i + 1) + "_of_4.csv";
        threadData[i].thread_id = i;
        if (pthread_create(&threads[i], nullptr, workderThreadFunc, &threadData[i]) != 0) {
            std::cerr << "Failed to create thread " << i << "\n";
            return 1;
        }
    }   
    pthread_join(threads[0], nullptr);
    pthread_join(threads[1], nullptr);
    pthread_join(threads[2], nullptr);
    pthread_join(threads[3], nullptr);

    const auto End = clock::now();
    const double Ms = ms_double(End - Start).count();
    std::cout << "Total geocoding time for all threads: " << std::fixed << std::setprecision(4) << Ms << " ms\n";
    return 0;
}

// #include "GeocodingEngine.hpp"
// #include "geocode_point.hpp"
// #include <iostream>
// #include <chrono>

// int main() {
//     GeocodingEngine engine;
//     if (!engine.initialize("data/toronto_centreline.gpkg", "data/address_points.gpkg")) {
//         return 1;
//     }

    

//     // Test a quick query lookup
//     // std::vector<Centreline> results = engine.findStreetSegments("King St W");
//     // std::cout << "Found " << results.size() << " matching segments for \"King St W\"\n";
//     // std::cout << "----------------------------------------\n";
//     // std::cout << "Centreline ID | Street Name | Lo Num L | Hi Num L | Lo Num R | Hi Num R | Coordinates\n";
//     // std::cout << "----------------------------------------\n";
//     // for (const auto& entry : results) {
//     //     std::cout << entry.centreline_id << " | "
//     //               << entry.street_name << " | "
//     //               << entry.lo_num_l << " | "
//     //               << entry.hi_num_l << " | "
//     //               << entry.lo_num_r << " | "
//     //               << entry.hi_num_r << " | "
//     //               << entry.coordinates.size() << " points\n";
//     // }


//     using clock = std::chrono::steady_clock;
//     using ms_double = std::chrono::duration<double, std::milli>;
//     const auto start = clock::now();
//     std::pair<double, double> location = geocode_address("123 King St W", engine);
//     const auto end = clock::now();
//     const double elapsed_ms = ms_double(end - start).count();
   
//     std::cout << "Geocoded location: (" << location.first << ", " << location.second << ")\n";
//      std::cout << "Geocoding took " << elapsed_ms << " ms\n";
//     return 0;
// }