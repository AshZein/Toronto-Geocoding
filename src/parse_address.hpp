#pragma once
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct ParsedAddress {
    std::optional<int> address_number;
    std::string unit;          // optional: e.g., "4B", "PH1"
    std::string street_name;   // normalized, e.g., "KING ST W"
    std::string leftover;      // optional: city/postal if included
};

enum class Direction : int {
    None = 0,
    North,
    East,
    South,
    West
};

Direction parse_direction(std::string_view token)
std::string_view trimView(std::string_view sv);
bool isAllDigits(std::string_view sv);
std::string toUpperAscii(std::string_view input);
std::vector<std::string_view> tokenizeWhitespace(std::string_view text);
std::string_view canonicalizeToken(std::string_view rawToken);
std::string normalizeStreetName(std::string_view rawStreetName);
ParsedAddress parseStreetAddress(std::string_view rawAddress);
