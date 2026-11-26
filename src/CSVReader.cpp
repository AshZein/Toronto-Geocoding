#include "CSVReader.hpp"
#include <iostream>
#include <vector>
#include <mutex>
#include <fstream>
#include <sstream>

CSVReader::CSVReader(const std::string& filename, char delimiter)
    : filename_(filename), delimiter_(delimiter), current_line_(0), line_count_(0) {
    // Constructor implementation
    std::ifstream file(filename_);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename_ << std::endl;
        return;
    }
    // Count the number of lines in the file
    std::string line;
    while (std::getline(file, line)) {
        line_count_++;
    }
    file.close();
    file_.open(filename_);
}

std::vector<std::vector<std::string>> CSVReader::readChunk(){
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::vector<std::string>> chunk;

    std::ifstream file(filename_);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename_ << std::endl;
        return chunk;
    }

    while (current_line_ < line_count_ && chunk.size() < chunk_size_) {
        std::string line;
        if (!std::getline(file, line)) {
            break; // End of file or error
        }

        std::vector<std::string> row;
        std::string cell;
        std::istringstream lineStream(line);
        while (std::getline(lineStream, cell, delimiter_)) {
            row.push_back(cell);
        }
        chunk.push_back(row);
        current_line_++;
    } 
    
    return chunk;
}
