#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct ParsedAddress {
    std::optional<int> house_number;
    std::string unit;          // optional: e.g., "4B", "PH1"
    std::string street_name;   // normalized, e.g., "KING ST W"
    std::string leftover;      // optional: city/postal if included
};

inline std::string_view trimView(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_back(1);
    }
    return sv;
}

inline bool isAllDigits(std::string_view sv) {
    if (sv.empty()) return false;
    for (unsigned char c : sv) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

inline ParsedAddress parseStreetAddress(std::string_view rawAddress) {
    ParsedAddress out;
    std::string_view rest = trimView(rawAddress);
    if (rest.empty()) return out;

    // 1. Optional leading unit designator like "#4B-" or "APT 2, 100 KING ST W"
    // If there is a comma, split off trailing city/province/postal for now
    size_t firstComma = rest.find(',');
    if (firstComma != std::string_view::npos) {
        out.leftover = std::string(trimView(rest.substr(firstComma + 1)));
        rest = trimView(rest.substr(0, firstComma));
    }

    // 2. Extract leading tokens using your whitespace tokenizer
    auto tokens = tokenizeWhitespace(rest);
    if (tokens.empty()) return out;

    size_t idx = 0;

    // Handle unit-first formats like "APT 5 100 KING ST W"
    std::string upperFirst = toUpperAscii(tokens[0]);
    if ((upperFirst == "APT" || upperFirst == "UNIT" || upperFirst == "SUITE" || upperFirst == "STE")
        && tokens.size() >= 3 && isAllDigits(tokens[1])) {
        out.unit = std::string(tokens[1]);
        idx = 2;
    } else if (!tokens[0].empty() && tokens[0].front() == '#') {
        out.unit = std::string(tokens[0].substr(1));
        idx = 1;
    }

    // 3. Parse house number (also handles hyphenated/range numbers like "100-102" by taking "100")
    if (idx < tokens.size()) {
        std::string_view numTok = tokens[idx];
        size_t dashPos = numTok.find('-');
        std::string_view primaryNum = (dashPos != std::string_view::npos)
            ? numTok.substr(0, dashPos)
            : numTok;

        if (isAllDigits(primaryNum)) {
            int parsedNum = 0;
            for (unsigned char c : primaryNum) {
                parsedNum = parsedNum * 10 + (c - '0');
            }
            out.house_number = parsedNum;
            ++idx;
        }
    }

    // 4. Reassemble remaining tokens into street name and normalize
    std::string rawStreet;
    for (; idx < tokens.size(); ++idx) {
        if (!rawStreet.empty()) rawStreet.push_back(' ');
        rawStreet.append(tokens[idx].data(), tokens[idx].size());
    }

    out.street_name = normalizeStreetName(rawStreet);
    return out;
}