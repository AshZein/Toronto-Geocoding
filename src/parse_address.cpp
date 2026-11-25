#include "parse_address.hpp"

Direction parse_direction(std::string_view token) {
    if (token == "N/O" || token == "N" || token == "NORTH") return Direction::North;
    if (token == "E/O" || token == "E" || token == "EAST")  return Direction::East;
    if (token == "S/O" || token == "S" || token == "SOUTH") return Direction::South;
    if (token == "W/O" || token == "W" || token == "WEST")  return Direction::West;
    return Direction::None;
}

std::string_view trimView(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

bool isAllDigits(std::string_view sv) {
    if (sv.empty()) return false;
    for (unsigned char c : sv) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

std::string toUpperAscii(std::string_view input) {
    std::string out(input);
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

std::vector<std::string_view> tokenizeWhitespace(std::string_view text) {
    std::vector<std::string_view> tokens;
    size_t i = 0;
    const size_t n = text.size();

    while (i < n) {
        while (i < n && std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
        }
        if (i >= n) break;

        size_t start = i;
        while (i < n && !std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
        }
        tokens.emplace_back(text.substr(start, i - start));
    }
    return tokens;
}

std::string_view canonicalizeToken(std::string_view rawToken) {
    static const std::unordered_map<std::string_view, std::string_view> kAliases = {
        // Street types
        {"STREET", "ST"},      {"ST.", "ST"},      {"STR", "ST"},
        {"AVENUE", "AVE"},     {"AVE.", "AVE"},    {"AV", "AVE"},
        {"BOULEVARD", "BLVD"}, {"BLVD.", "BLVD"},
        {"DRIVE", "DR"},       {"DR.", "DR"},
        {"ROAD", "RD"},        {"RD.", "RD"},
        {"COURT", "CT"},       {"CT.", "CT"},
        {"CRESCENT", "CRES"},  {"CRES.", "CRES"},
        {"PLACE", "PL"},       {"PL.", "PL"},
        {"PARKWAY", "PKWY"},   {"PKWY.", "PKWY"},
        {"TRAIL", "TRL"},      {"TRL.", "TRL"},
        {"LANE", "LN"},        {"LN.", "LN"},
        {"CIRCLE", "CIR"},     {"CIR.", "CIR"},

        // Directionals
            {"NORTH", "N"},        {"NORTH.", "N"},       {"N.", "N"},
            {"SOUTH", "S"},        {"SOUTH.", "S"},       {"S.", "S"},
            {"EAST", "E"},         {"EAST.", "E"},        {"E.", "E"},
            {"WEST", "W"},         {"WEST.", "W"},        {"W.", "W"},
            {"NORTHEAST", "NE"},   {"NORTHEAST.", "NE"},  {"NE.", "NE"},
            {"NORTHWEST", "NW"},   {"NORTHWEST.", "NW"},  {"NW.", "NW"},
            {"SOUTHEAST", "SE"},   {"SOUTHEAST.", "SE"},  {"SE.", "SE"},
            {"SOUTHWEST", "SW"},   {"SOUTHWEST.", "SW"},  {"SW.", "SW"},
        // Common abbreviations
        {"ST", "ST"},          {"AVE", "AVE"},       {"BLVD", "BLVD"},
        {"DR", "DR"},          {"RD", "RD"},         {"CT", "CT"},
        {"CRES", "CRES"},      {"PL", "PL"},         {"PKWY", "PKWY"},
        {"TRL", "TRL"},        {"LN", "LN"},         {"CIR", "CIR"},
        {"N", "N"},            {"S", "S"},           {"E", "E"},
        {"W", "W"},            {"NE", "NE"},         {"NW", "NW"},
        {"SE", "SE"},          {"SW", "SW"}
    };

    auto it = kAliases.find(rawToken);
    return (it != kAliases.end()) ? it->second : rawToken;
}

std::string normalizeStreetName(std::string_view rawStreetName) {
    std::string upper = toUpperAscii(rawStreetName);
    auto rawTokens = tokenizeWhitespace(upper);

    std::string normalized;
    for (size_t i = 0; i < rawTokens.size(); ++i) {
        std::string_view canon = canonicalizeToken(rawTokens[i]);
        if (canon.empty()) continue;
        if (!normalized.empty()) {
            normalized.push_back(' ');
        }
        normalized.append(canon.data(), canon.size());
    }
    return normalized;
}

ParsedAddress parseStreetAddress(std::string_view rawAddress) {
    ParsedAddress out;
    std::string_view rest = trimView(rawAddress);
    if (rest.empty()) return out;

    // 1. If there is a comma, split off trailing city/province/postal for now
    size_t firstComma = rest.find(',');
    if (firstComma != std::string_view::npos) {
        out.leftover = std::string(trimView(rest.substr(firstComma + 1)));
        rest = trimView(rest.substr(0, firstComma));
    }

    // 2. Extract leading tokens using whitespace tokenizer
    auto tokens = tokenizeWhitespace(rest);
    if (tokens.empty()) return out;

    size_t idx = 0;

    // Handle unit-first formats like "APT 5 100 KING ST W" or "#4B 100 KING ST W"
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
            out.address_number = parsedNum;
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