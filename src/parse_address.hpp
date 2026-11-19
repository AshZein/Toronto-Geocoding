#pragma once

struct ParsedAddress {
    std::optional<int> house_number;
    std::string unit;          // optional: e.g., "4B", "PH1"
    std::string street_name;   // normalized, e.g., "KING ST W"
    std::string leftover;      // optional: city/postal if included
};



inline std::string_view trimView(std::string_view sv);
inline bool isAllDigits(std::string_view sv);
inline std::string toUpperAscii(std::string_view input);
inline std::vector<std::string_view> tokenizeWhitespace(std::string_view text);
inline std::string_view canonicalizeToken(std::string_view rawToken);
inline std::string normalizeStreetName(std::string_view rawStreetName);
inline ParsedAddress parseStreetAddress(std::string_view rawAddress);
