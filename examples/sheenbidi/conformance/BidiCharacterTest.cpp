// BidiCharacterTest — Unicode's BidiCharacterTest.txt through import
// mcpplibs.mwrap.sheenbidi, every case checked.
//
//   BidiCharacterTest <BidiCharacterTest.txt> <BidiMirroring.txt>
//
// Each line is one paragraph: code points; the paragraph direction (0 LTR,
// 1 RTL, 2 auto, the first strong character deciding and LTR without one);
// the resolved paragraph level; the resolved level of each character after
// rule L1 (x for the characters X9 removes); and the visual order of the
// characters that are not removed. A case passes when SheenBidi gives
//
//   - the paragraph: one paragraph covering the whole line (P1) at the
//     expected level (P2, P3);
//   - the levels: the level of each run of the line (SBLineGetRunsPtr),
//     which is the level after L1, on every character that is not x;
//   - the order: the runs in visual order, an odd run reversed (L2), the x
//     characters skipped;
//   - the mirrors: SBMirrorLocator reports exactly the characters at an odd
//     level that BidiMirroring.txt maps, each with that mapping (L4).
//
// Output: one line per failing case (its line number and what differed), then
// "BidiCharacterTest: <cases> tests, <failures> errors". Exit 0 when nothing
// failed.
import std;
import mcpplibs.mwrap.sheenbidi;

using namespace mcpplibs::mwrap::sheenbidi::macros;  // SBLevelDefaultLTR

namespace {

std::vector<std::string_view> split(std::string_view s, char sep) {
    std::vector<std::string_view> out;
    for (std::size_t start = 0;;) {
        const std::size_t end = s.find(sep, start);
        out.push_back(s.substr(start, end - start));
        if (end == std::string_view::npos) return out;
        start = end + 1;
    }
}

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

std::map<std::uint32_t, std::uint32_t> mirroring(const char* path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error(std::string("cannot read ") + path);
    std::map<std::uint32_t, std::uint32_t> out;
    for (std::string line; std::getline(in, line);) {
        std::string_view data = line;
        data = data.substr(0, data.find('#'));
        const auto fields = split(data, ';');
        if (fields.size() != 2) continue;
        const auto from = words(fields[0]), to = words(fields[1]);
        if (from.size() == 1 && to.size() == 1) out[number(from[0], 16)] = number(to[0], 16);
    }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::println(std::cerr, "usage: BidiCharacterTest <BidiCharacterTest.txt> <BidiMirroring.txt>");
        return 2;
    }
    const auto mirrors = mirroring(argv[2]);
    std::ifstream in(argv[1]);
    if (!in) {
        std::println(std::cerr, "cannot read {}", argv[1]);
        return 2;
    }
    std::size_t cases = 0, failures = 0, number_of_line = 0;
    SBMirrorLocatorRef locator = SBMirrorLocatorCreate();
    bool loaded = false;
    for (std::string line; std::getline(in, line);) {
        ++number_of_line;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto fields = split(line, ';');
        if (fields.size() != 5) throw std::runtime_error("line " + std::to_string(number_of_line) + ": not 5 fields");
        ++cases;

        std::vector<std::uint32_t> text;
        for (auto w : words(fields[0])) text.push_back(number(w, 16));
        const std::uint32_t direction = number(words(fields[1]).at(0), 10);
        const SBLevel base = direction == 0 ? 0 : direction == 1 ? 1 : SBLevelDefaultLTR;
        const SBLevel expected_paragraph = static_cast<SBLevel>(number(words(fields[2]).at(0), 10));
        std::vector<int> expected_levels;
        for (auto w : words(fields[3])) expected_levels.push_back(w == "x" ? removed : static_cast<int>(number(w, 10)));
        std::vector<std::size_t> expected_order;
        for (auto w : words(fields[4])) expected_order.push_back(number(w, 10));

        std::string problem;
        const SBCodepointSequence seq{SBStringEncodingUTF32, text.data(), text.size()};
        SBAlgorithmRef algorithm = SBAlgorithmCreate(&seq);
        SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, 0, text.size(), base);
        if (SBParagraphGetLength(paragraph) != text.size()) {
            problem = std::format("paragraph length {} of {}", SBParagraphGetLength(paragraph), text.size());
        } else if (SBParagraphGetBaseLevel(paragraph) != expected_paragraph) {
            problem = std::format("paragraph level {} != {}", SBParagraphGetBaseLevel(paragraph), expected_paragraph);
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
            if (problem.empty()) {
                std::map<std::size_t, std::uint32_t> expected_mirrors, found_mirrors;
                for (std::size_t i = 0; i < text.size(); ++i) {
                    const auto m = mirrors.find(text[i]);
                    if (expected_levels[i] != removed && (expected_levels[i] & 1) && m != mirrors.end())
                        expected_mirrors[i] = m->second;
                }
                SBMirrorLocatorLoadLine(locator, sbline, text.data());
                loaded = true;
                while (SBMirrorLocatorMoveNext(locator)) {
                    const SBMirrorAgent* agent = SBMirrorLocatorGetAgent(locator);
                    found_mirrors[agent->index] = agent->mirror;
                }
                if (found_mirrors != expected_mirrors) problem = "mirrors differ";
            }
            SBLineRelease(sbline);
        }
        SBParagraphRelease(paragraph);
        SBAlgorithmRelease(algorithm);
        if (!problem.empty()) {
            ++failures;
            if (failures <= 50) std::println("line {}: {}", number_of_line, problem);
        }
    }
    // A mirror locator that never loaded a line must not be released in
    // SheenBidi 3.0.0 (its finalizer releases a NULL line).
    if (loaded) SBMirrorLocatorRelease(locator);
    std::println("BidiCharacterTest: {} tests, {} errors", cases, failures);
    return failures == 0 && cases > 0 ? 0 : 1;
}
