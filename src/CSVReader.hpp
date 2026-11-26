#pragma once

class CSVReader {
    private:
        size_t chunk_size_;
        int current_line_;
        char delimiter_;
        std::string filename_;
        std::ifstream file_;
        int line_count_;
        std::mutex mutex_;
        
    public:
        CSVReader(const std::string& filename, char delimiter = ',');
        std::vector<std::vector<std::string>> readAll();
        std::vector<std::vector<std::string>> readChunk();
};