// sheenbidi — lay out bidirectional text as a renderer does, through import
// mcpplibs.mwrap.sheenbidi: each UTF-8 line in logical (storage) order is
// split into paragraphs, its embedding levels resolved (UAX #9), and printed
// in visual order with mirrored brackets, with its levels, its runs and its
// script runs. A terminal without bidi support shows the visual line as it
// should be read.
//
//   sheenbidi [--ltr | --rtl] [--runs] text ...      each argument
//   sheenbidi [--ltr | --rtl] [--runs] -             lines from stdin
//
// The base direction is the first strong character's (P2, P3) unless --ltr or
// --rtl says otherwise. Offsets are in UTF-8 code units, as SheenBidi reports
// them for UTF-8 input.
import std;
import mcpplibs.mwrap.sheenbidi;

using namespace mcpplibs::mwrap::sheenbidi::macros;  // SBStringEncodingUTF8, SBLevelDefaultLTR …

namespace {

void append_utf8(std::string& out, SBCodepoint c) {
    if (c < 0x80) {
        out += static_cast<char>(c);
    } else if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += static_cast<char>(0xE0 | (c >> 12));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (c >> 18));
        out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    }
}

std::string tag(SBUInt32 t) {
    return {static_cast<char>(t >> 24), static_cast<char>(t >> 16), static_cast<char>(t >> 8), static_cast<char>(t)};
}

struct Options {
    SBLevel base = SBLevelDefaultLTR;
    bool runs = false;
};

void lay_out(std::string_view text, const Options& options) {
    const auto* bytes = reinterpret_cast<const SBUInt8*>(text.data());
    const SBCodepointSequence seq{SBStringEncodingUTF8, text.data(), text.size()};
    SBAlgorithmRef algorithm = SBAlgorithmCreate(&seq);
    for (SBUInteger offset = 0; offset < text.size();) {
        SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, offset, text.size() - offset, options.base);
        const SBUInteger length = SBParagraphGetLength(paragraph);
        SBLineRef line = SBParagraphCreateLine(paragraph, offset, length);

        // Mirrored glyphs: the characters at odd levels that have a mirror (L4).
        std::map<SBUInteger, SBCodepoint> mirrored;
        SBMirrorLocatorRef locator = SBMirrorLocatorCreate();
        SBMirrorLocatorLoadLine(locator, line, text.data());
        while (SBMirrorLocatorMoveNext(locator)) {
            const SBMirrorAgent* agent = SBMirrorLocatorGetAgent(locator);
            mirrored[agent->index] = agent->mirror;
        }
        SBMirrorLocatorRelease(locator);

        // The runs in visual order, an odd one read backwards (L2).
        std::string visual, runs;
        const SBRun* run = SBLineGetRunsPtr(line);
        for (SBUInteger r = 0; r < SBLineGetRunCount(line); ++r) {
            runs += std::format(" [{},{}) L{}", run[r].offset, run[r].offset + run[r].length, run[r].level);
            if (run[r].level & 1) {
                SBUInteger i = run[r].offset + run[r].length;
                while (i > run[r].offset) {
                    const SBCodepoint c = SBCodepointDecodePreviousFromUTF8(bytes, run[r].offset + run[r].length, &i);
                    const auto m = mirrored.find(i);
                    append_utf8(visual, m != mirrored.end() ? m->second : c);
                }
            } else {
                visual.append(text.substr(run[r].offset, run[r].length));
            }
        }

        std::string levels;
        const SBLevel* level = SBParagraphGetLevelsPtr(paragraph);
        for (SBUInteger i = 0; i < length;) {  // one digit per code point
            const SBUInteger at = i;
            SBCodepointDecodeNextFromUTF8(bytes + offset, length, &i);
            levels += static_cast<char>('0' + level[at]);
        }

        std::println("logical  {}", text.substr(offset, length));
        std::println("visual   {}", visual);
        std::println("levels   {} (base {}; odd levels run right to left)", levels, SBParagraphGetBaseLevel(paragraph));
        if (options.runs) {
            std::println("runs    {}", runs);
            const SBCodepointSequence part{SBStringEncodingUTF8, text.data() + offset, length};
            SBScriptLocatorRef scripts = SBScriptLocatorCreate();
            SBScriptLocatorLoadCodepoints(scripts, &part);
            std::string script_runs;
            while (SBScriptLocatorMoveNext(scripts)) {
                const SBScriptAgent* agent = SBScriptLocatorGetAgent(scripts);
                script_runs += std::format(" [{},{}) {}", offset + agent->offset, offset + agent->offset + agent->length,
                                           tag(SBScriptGetUnicodeTag(agent->script)));
            }
            SBScriptLocatorRelease(scripts);
            std::println("scripts {}", script_runs);
        }
        std::println();
        SBLineRelease(line);
        SBParagraphRelease(paragraph);
        offset += length;
    }
    SBAlgorithmRelease(algorithm);
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    std::vector<std::string> texts;
    bool from_stdin = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--ltr") options.base = 0;
        else if (arg == "--rtl") options.base = 1;
        else if (arg == "--runs") options.runs = true;
        else if (arg == "-") from_stdin = true;
        else if (arg == "-h" || arg == "--help") {
            std::println("usage: sheenbidi [--ltr | --rtl] [--runs] (text ... | -)");
            return 0;
        } else texts.emplace_back(arg);
    }
    std::println("SheenBidi {} (Unicode 17.0.0) through import mcpplibs.mwrap.sheenbidi\n", SBVersionGetString());
    if (from_stdin) {
        for (std::string line; std::getline(std::cin, line);) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            texts.push_back(line);
        }
    }
    if (texts.empty()) texts = {"The title is מזל טוב in Hebrew.", "הספר (book) עולה 120 ש״ח.", "مرحبا [world] 2026!"};
    for (const auto& text : texts) lay_out(text, options);
}
