// BidiTest — Unicode's BidiTest.txt through import mcpplibs.mwrap.sheenbidi,
// every case checked.
//
//   BidiTest <BidiTest.txt>
//
// Each data line is a sequence of bidi classes and a bitset of paragraph
// directions (1 auto, 2 LTR, 4 RTL); the @Levels and @Reorder lines before it
// give the expected level of each class after rule L1 (x for what X9 removes)
// and the visual order of the classes not removed. Every class is given as a
// code point of that class (the table below, each checked against
// SBCodepointGetBidiType first), and each listed direction is one case. A
// case passes when SheenBidi gives one paragraph covering the sequence (B is
// only ever last), the levels of the line's runs on every class that is not
// x, and the runs' visual order (an odd run reversed, x skipped).
//
// Output: one line per failing case, then "BidiTest: <cases> tests, <failures>
// errors". Exit 0 when nothing failed.
import std;
import mcpplibs.mwrap.sheenbidi;

using namespace mcpplibs::mwrap::sheenbidi::macros;  // SBBidiTypeL …, SBLevelDefaultLTR

namespace {

struct Class {
    std::string_view name;
    std::uint32_t codepoint;
    SBBidiType type;
};

// One code point of each of the 23 classes.
constexpr Class classes[] = {
    {"L", 0x0041, SBBidiTypeL},     {"R", 0x05D0, SBBidiTypeR},     {"AL", 0x0627, SBBidiTypeAL},
    {"EN", 0x0030, SBBidiTypeEN},   {"ES", 0x002B, SBBidiTypeES},   {"ET", 0x0023, SBBidiTypeET},
    {"AN", 0x0660, SBBidiTypeAN},   {"CS", 0x002C, SBBidiTypeCS},   {"NSM", 0x0300, SBBidiTypeNSM},
    {"BN", 0x00AD, SBBidiTypeBN},   {"B", 0x2029, SBBidiTypeB},     {"S", 0x0009, SBBidiTypeS},
    {"WS", 0x0020, SBBidiTypeWS},   {"ON", 0x0021, SBBidiTypeON},   {"LRE", 0x202A, SBBidiTypeLRE},
    {"LRO", 0x202D, SBBidiTypeLRO}, {"RLE", 0x202B, SBBidiTypeRLE}, {"RLO", 0x202E, SBBidiTypeRLO},
    {"PDF", 0x202C, SBBidiTypePDF}, {"LRI", 0x2066, SBBidiTypeLRI}, {"RLI", 0x2067, SBBidiTypeRLI},
    {"FSI", 0x2068, SBBidiTypeFSI}, {"PDI", 0x2069, SBBidiTypePDI},
};

std::vector<std::string_view> words(std::string_view s) {
    std::vector<std::string_view> out;
    for (std::size_t i = 0; i < s.size();) {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
        const std::size_t start = i;
        while (i < s.size() && s[i] != ' ' && s[i] != '\t') ++i;
        if (i > start) out.push_back(s.substr(start, i - start));
    }
    return out;
}

std::uint32_t number(std::string_view s, int base) {
    std::uint32_t value = 0;
    const auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), value, base);
    if (error != std::errc{} || end != s.data() + s.size()) throw std::runtime_error("not a number: " + std::string(s));
    return value;
}

constexpr int removed = -1;  // an "x" level

std::uint32_t codepoint_of(std::string_view name) {
    for (const Class& c : classes)
        if (c.name == name) return c.codepoint;
    throw std::runtime_error("unknown bidi class " + std::string(name));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::println(std::cerr, "usage: BidiTest <BidiTest.txt>");
        return 2;
    }
    for (const Class& c : classes) {
        if (SBCodepointGetBidiType(c.codepoint) != c.type) {
            std::println("U+{:04X} is not of class {}", c.codepoint, c.name);
            return 1;
        }
    }
    std::ifstream in(argv[1]);
    if (!in) {
        std::println(std::cerr, "cannot read {}", argv[1]);
        return 2;
    }
    std::vector<int> expected_levels;
    std::vector<std::size_t> expected_order;
    std::size_t cases = 0, failures = 0, number_of_line = 0;
    for (std::string line; std::getline(in, line);) {
        ++number_of_line;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string_view data = line;
        data = data.substr(0, data.find('#'));
        if (words(data).empty()) continue;
        if (data.starts_with("@Levels:")) {
            expected_levels.clear();
            for (auto w : words(data.substr(8))) expected_levels.push_back(w == "x" ? removed : static_cast<int>(number(w, 10)));
            continue;
        }
        if (data.starts_with("@Reorder:")) {
            expected_order.clear();
            for (auto w : words(data.substr(9))) expected_order.push_back(number(w, 10));
            continue;
        }
        if (data.starts_with('@')) continue;  // forward compatibility, as the file asks
        const std::size_t semicolon = data.find(';');
        if (semicolon == std::string_view::npos) throw std::runtime_error("line " + std::to_string(number_of_line) + ": no bitset");
        std::vector<std::uint32_t> text;
        for (auto w : words(data.substr(0, semicolon))) text.push_back(codepoint_of(w));
        const std::uint32_t bitset = number(words(data.substr(semicolon + 1)).at(0), 16);
        if (text.size() != expected_levels.size()) throw std::runtime_error("line " + std::to_string(number_of_line) + ": levels do not match");

        const SBCodepointSequence seq{SBStringEncodingUTF32, text.data(), text.size()};
        SBAlgorithmRef algorithm = SBAlgorithmCreate(&seq);
        for (const auto [bit, base, name] : {std::tuple{1u, SBLevel{SBLevelDefaultLTR}, "auto"},
                                             std::tuple{2u, SBLevel{0}, "LTR"}, std::tuple{4u, SBLevel{1}, "RTL"}}) {
            if (!(bitset & bit)) continue;
            ++cases;
            std::string problem;
            SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, 0, text.size(), base);
            if (SBParagraphGetLength(paragraph) != text.size()) {
                problem = std::format("paragraph length {} of {}", SBParagraphGetLength(paragraph), text.size());
            } else {
                SBLineRef sbline = SBParagraphCreateLine(paragraph, 0, text.size());
                std::vector<int> levels(text.size(), removed);
                std::vector<std::size_t> order;
                const SBRun* runs = SBLineGetRunsPtr(sbline);
                for (SBUInteger r = 0; r < SBLineGetRunCount(sbline); ++r) {
                    for (SBUInteger k = 0; k < runs[r].length; ++k) {
                        const std::size_t i = runs[r].level & 1 ? runs[r].offset + runs[r].length - 1 - k : runs[r].offset + k;
                        levels.at(i) = runs[r].level;
                        if (expected_levels.at(i) != removed) order.push_back(i);
                    }
                }
                for (std::size_t i = 0; i < text.size() && problem.empty(); ++i) {
                    if (expected_levels[i] != removed && levels[i] != expected_levels[i])
                        problem = std::format("level of {} is {}, expected {}", i, levels[i], expected_levels[i]);
                }
                if (problem.empty() && order != expected_order) problem = "visual order differs";
                SBLineRelease(sbline);
            }
            SBParagraphRelease(paragraph);
            if (!problem.empty()) {
                ++failures;
                if (failures <= 50) std::println("line {} ({}): {}", number_of_line, name, problem);
            }
        }
        SBAlgorithmRelease(algorithm);
    }
    std::println("BidiTest: {} tests, {} errors", cases, failures);
    return failures == 0 && cases > 0 ? 0 : 1;
}
