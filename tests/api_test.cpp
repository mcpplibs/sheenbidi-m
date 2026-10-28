// The SheenBidi API through the module, spelled as <SheenBidi/SheenBidi.h>
// documents it: code point properties (bidi types, general categories,
// mirrors, scripts and their tags), UTF-8 / UTF-16 decoding and code point
// sequences, paragraph boundaries and base levels, embedding levels (with the
// UAX #9 N0 bracket pair example and explicit isolates), lines and their runs
// in visual order, the mirror locator, the script locator, retain/release, a
// custom allocator, and the version. Only the module is imported; no
// SheenBidi header is included.
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.sheenbidi;

using namespace mcpplibs::mwrap::sheenbidi::macros;  // SBBidiTypeL, SBLevelDefaultLTR, SBBidiTypeIsStrong …

namespace {

SBCodepointSequence utf32(const std::u32string& s) {
    return {SBStringEncodingUTF32, s.data(), s.size()};
}

// One paragraph and its first line, released together.
struct Laid {
    SBAlgorithmRef algorithm;
    SBParagraphRef paragraph;
    SBLineRef line;

    Laid(const SBCodepointSequence& seq, SBLevel base)
        : algorithm(SBAlgorithmCreate(&seq)),
          paragraph(SBAlgorithmCreateParagraph(algorithm, 0, seq.stringLength, base)),
          line(SBParagraphCreateLine(paragraph, 0, SBParagraphGetLength(paragraph))) {}
    ~Laid() {
        SBLineRelease(line);
        SBParagraphRelease(paragraph);
        SBAlgorithmRelease(algorithm);
    }
    Laid(const Laid&) = delete;
    Laid& operator=(const Laid&) = delete;

    std::vector<SBLevel> levels() const {
        const SBLevel* p = SBParagraphGetLevelsPtr(paragraph);
        return {p, p + SBParagraphGetLength(paragraph)};
    }
    std::vector<SBRun> runs() const {
        const SBRun* p = SBLineGetRunsPtr(line);
        return {p, p + SBLineGetRunCount(line)};
    }
    // The logical indices in visual order, as a renderer draws them.
    std::vector<std::size_t> visual() const {
        std::vector<std::size_t> out;
        for (const SBRun& r : runs()) {
            for (SBUInteger i = 0; i < r.length; ++i)
                out.push_back(r.level & 1 ? r.offset + r.length - 1 - i : r.offset + i);
        }
        return out;
    }
};

std::string digits(const std::vector<SBLevel>& levels) {
    std::string out;
    for (SBLevel l : levels) out += static_cast<char>('0' + l);
    return out;
}

}  // namespace

TEST(Api, Version) {
    EXPECT_STREQ(SBVersionGetString(), "3.0.0");
    EXPECT_STREQ(SBVersionGetString(), SHEENBIDI_VERSION_STRING);
    EXPECT_EQ(SHEENBIDI_VERSION_MAJOR, 3);
}

TEST(Api, BidiTypes) {
    EXPECT_EQ(SBCodepointGetBidiType(U'a'), SBBidiTypeL);
    EXPECT_EQ(SBCodepointGetBidiType(U'א'), SBBidiTypeR);
    EXPECT_EQ(SBCodepointGetBidiType(U'ا'), SBBidiTypeAL);
    EXPECT_EQ(SBCodepointGetBidiType(U'1'), SBBidiTypeEN);
    EXPECT_EQ(SBCodepointGetBidiType(U'١'), SBBidiTypeAN);
    EXPECT_EQ(SBCodepointGetBidiType(U' '), SBBidiTypeWS);
    EXPECT_EQ(SBCodepointGetBidiType(U'⁧'), SBBidiTypeRLI);
    EXPECT_EQ(SBCodepointGetBidiType(U'̀'), SBBidiTypeNSM);
    EXPECT_TRUE(SBBidiTypeIsStrong(SBCodepointGetBidiType(U'א')));
    EXPECT_TRUE(SBBidiTypeIsWeak(SBBidiTypeEN));
    EXPECT_TRUE(SBBidiTypeIsNeutral(SBBidiTypeON));
    EXPECT_TRUE(SBBidiTypeIsFormat(SBBidiTypePDI));
    // Unicode 17.0: U+10940 SIDETIC LETTER N01 is right to left.
    EXPECT_EQ(SBCodepointGetBidiType(0x10940), SBBidiTypeR);
}

TEST(Api, CategoriesMirrorsScripts) {
    EXPECT_EQ(SBCodepointGetGeneralCategory(U'A'), SBGeneralCategoryLU);
    EXPECT_EQ(SBCodepointGetGeneralCategory(U'٣'), SBGeneralCategoryND);
    EXPECT_TRUE(SBGeneralCategoryIsLetter(SBCodepointGetGeneralCategory(U'ب')));
    EXPECT_TRUE(SBGeneralCategoryIsPunctuation(SBCodepointGetGeneralCategory(U'(')));
    EXPECT_EQ(SBCodepointGetMirror(U'('), U')');
    EXPECT_EQ(SBCodepointGetMirror(U'«'), U'»');
    EXPECT_EQ(SBCodepointGetMirror(U'a'), 0u);
    EXPECT_EQ(SBCodepointGetScript(U'ب'), SBScriptARAB);
    EXPECT_EQ(SBCodepointGetScript(U'א'), SBScriptHEBR);
    EXPECT_EQ(SBCodepointGetScript(U' '), SBScriptZYYY);
    EXPECT_EQ(SBCodepointGetScript(0x10940), SBScriptSIDT);
    EXPECT_EQ(SBScriptGetUnicodeTag(SBScriptARAB), 0x41726162u);  // 'Arab'
    EXPECT_EQ(SBScriptGetUnicodeTag(SBScriptSIDT), 0x53696474u);  // 'Sidt'
}

TEST(Api, Decoding) {
    const std::string utf8 = "a\xD7\x90\xF0\x90\xA5\x80";  // a, U+05D0, U+10940
    const auto* bytes = reinterpret_cast<const SBUInt8*>(utf8.data());
    SBUInteger i = 0;
    EXPECT_EQ(SBCodepointDecodeNextFromUTF8(bytes, utf8.size(), &i), U'a');
    EXPECT_EQ(SBCodepointDecodeNextFromUTF8(bytes, utf8.size(), &i), U'א');
    EXPECT_EQ(SBCodepointDecodeNextFromUTF8(bytes, utf8.size(), &i), 0x10940u);
    EXPECT_EQ(i, utf8.size());
    EXPECT_EQ(SBCodepointDecodeNextFromUTF8(bytes, utf8.size(), &i), SBCodepointInvalid);
    EXPECT_EQ(SBCodepointDecodePreviousFromUTF8(bytes, utf8.size(), &i), 0x10940u);

    const SBUInt16 utf16[] = {0xD802, 0xDD40, 0x05D0, 0xDC00};  // U+10940, U+05D0, a lone surrogate
    i = 0;
    EXPECT_EQ(SBCodepointDecodeNextFromUTF16(utf16, 4, &i), 0x10940u);
    EXPECT_EQ(SBCodepointDecodeNextFromUTF16(utf16, 4, &i), U'א');
    EXPECT_EQ(SBCodepointDecodeNextFromUTF16(utf16, 4, &i), static_cast<SBCodepoint>(SBCodepointFaulty));
    i = 4;
    EXPECT_EQ(SBCodepointDecodePreviousFromUTF16(utf16, 4, &i), static_cast<SBCodepoint>(SBCodepointFaulty));
    EXPECT_FALSE(SBCodepointIsValid(0xD800u));
    EXPECT_TRUE(SBCodepointIsSurrogate(0xDFFFu));

    const SBCodepointSequence seq{SBStringEncodingUTF8, utf8.data(), utf8.size()};
    i = 0;
    EXPECT_EQ(SBCodepointSequenceGetCodepointAt(&seq, &i), U'a');
    i = utf8.size();
    EXPECT_EQ(SBCodepointSequenceGetCodepointBefore(&seq, &i), 0x10940u);
    EXPECT_EQ(i, 3u);
}

TEST(Api, ParagraphBoundaries) {
    const std::u32string text = U"abc\r\nאבג x";
    const auto seq = utf32(text);
    SBAlgorithmRef algorithm = SBAlgorithmCreate(&seq);
    const SBBidiType* types = SBAlgorithmGetBidiTypesPtr(algorithm);
    EXPECT_EQ(types[3], SBBidiTypeB);
    EXPECT_EQ(types[5], SBBidiTypeR);
    SBUInteger length = 0, separator = 0;
    SBAlgorithmGetParagraphBoundary(algorithm, 0, text.size(), &length, &separator);
    EXPECT_EQ(length, 5u);  // "abc\r\n": CR LF is one separator
    EXPECT_EQ(separator, 2u);
    SBAlgorithmGetParagraphBoundary(algorithm, 5, text.size() - 5, &length, &separator);
    EXPECT_EQ(length, 4u);
    EXPECT_EQ(separator, 1u);

    SBParagraphRef second = SBAlgorithmCreateParagraph(algorithm, 5, text.size() - 5, SBLevelDefaultLTR);
    EXPECT_EQ(SBParagraphGetOffset(second), 5u);
    EXPECT_EQ(SBParagraphGetLength(second), 4u);
    EXPECT_EQ(SBParagraphGetBaseLevel(second), 1);  // first strong is Hebrew
    SBParagraphRelease(second);
    SBAlgorithmRelease(algorithm);
}

TEST(Api, LevelsAndVisualOrder) {
    // "abc אבג" in an RTL paragraph: the Hebrew run first and reversed.
    const std::u32string text = U"abc אבג";
    Laid rtl(utf32(text), 1);
    EXPECT_EQ(digits(rtl.levels()), "2221111");
    EXPECT_EQ(rtl.visual(), (std::vector<std::size_t>{6, 5, 4, 3, 0, 1, 2}));
    Laid ltr(utf32(text), SBLevelDefaultLTR);
    EXPECT_EQ(SBParagraphGetBaseLevel(ltr.paragraph), 0);
    EXPECT_EQ(digits(ltr.levels()), "0000111");
    EXPECT_EQ(ltr.visual(), (std::vector<std::size_t>{0, 1, 2, 3, 6, 5, 4}));
    // No strong character: the default decides.
    Laid neutral_rtl(utf32(U"123 !"), SBLevelDefaultRTL);
    EXPECT_EQ(SBParagraphGetBaseLevel(neutral_rtl.paragraph), 1);
}

TEST(Api, BracketPairs) {
    // UAX #9's N0 example "AB(CD[&ef]!)gh" (uppercase right to left; here
    // Hebrew letters for A–D) in an RTL paragraph: both bracket pairs follow
    // the R before them, so they resolve to level 1 with "&" and "!", and only
    // "ef" and "gh" are at level 2 — the levels the example lists.
    const std::u32string text = U"אב(גד[&ef]!)gh";
    Laid laid(utf32(text), 1);
    EXPECT_EQ(digits(laid.levels()), "11111112211122");
    // Every bracket is at an odd level, so all four are drawn mirrored. The
    // locator reports them in the line's visual order.
    SBMirrorLocatorRef locator = SBMirrorLocatorCreate();
    SBMirrorLocatorLoadLine(locator, laid.line, text.data());
    std::vector<std::pair<SBUInteger, SBCodepoint>> mirrored;
    while (SBMirrorLocatorMoveNext(locator)) {
        const SBMirrorAgent* agent = SBMirrorLocatorGetAgent(locator);
        EXPECT_EQ(agent->codepoint, static_cast<SBCodepoint>(text[agent->index]));
        mirrored.emplace_back(agent->index, agent->mirror);
    }
    EXPECT_EQ(mirrored, (std::vector<std::pair<SBUInteger, SBCodepoint>>{{9, U'['}, {11, U'('}, {2, U')'}, {5, U']'}}));
    SBMirrorLocatorReset(locator);
    EXPECT_TRUE(SBMirrorLocatorMoveNext(locator));
    EXPECT_EQ(SBMirrorLocatorGetAgent(locator)->index, mirrored.front().first);
    SBMirrorLocatorRelease(locator);
}

TEST(Api, Isolates) {
    // An RLI … PDI isolate inside an LTR paragraph keeps "abc" outside it at 0.
    const std::u32string text = U"abc ⁧אב 12⁩ def";
    Laid laid(utf32(text), 0);
    const auto levels = laid.levels();
    EXPECT_EQ(levels[0], 0);
    EXPECT_EQ(levels[5], 1);   // Hebrew inside the isolate
    EXPECT_EQ(levels[8], 2);   // the digits after it, EN in an RTL isolate
    EXPECT_EQ(levels[text.size() - 1], 0);
}

TEST(Api, ScriptLocator) {
    const std::string utf8 = "abc \xD8\xB9\xD8\xB1\xD8\xA8\xD9\x8A \xD7\x90";  // "abc عربي א"
    const SBCodepointSequence seq{SBStringEncodingUTF8, utf8.data(), utf8.size()};
    SBScriptLocatorRef locator = SBScriptLocatorCreate();
    SBScriptLocatorLoadCodepoints(locator, &seq);
    std::vector<SBScriptAgent> runs;
    while (SBScriptLocatorMoveNext(locator)) runs.push_back(*SBScriptLocatorGetAgent(locator));
    ASSERT_EQ(runs.size(), 3u);
    EXPECT_EQ(runs[0].script, SBScriptLATN);
    EXPECT_EQ(runs[1].script, SBScriptARAB);
    EXPECT_EQ(runs[2].script, SBScriptHEBR);
    EXPECT_EQ(runs[2].offset + runs[2].length, utf8.size());
    SBScriptLocatorReset(locator);
    EXPECT_TRUE(SBScriptLocatorMoveNext(locator));
    EXPECT_EQ(SBScriptLocatorGetAgent(locator)->offset, 0u);
    SBScriptLocatorRelease(SBScriptLocatorRetain(locator));
    SBScriptLocatorRelease(locator);
}

TEST(Api, RetainRelease) {
    const std::u32string text = U"abc";
    const auto seq = utf32(text);
    SBAlgorithmRef algorithm = SBAlgorithmCreate(&seq);
    EXPECT_EQ(SBAlgorithmRetain(algorithm), algorithm);
    SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, 0, 3, 0);
    SBAlgorithmRelease(algorithm);
    SBAlgorithmRelease(algorithm);  // the paragraph keeps what it needs
    EXPECT_EQ(SBParagraphRetain(paragraph), paragraph);
    SBParagraphRelease(paragraph);
    SBLineRef line = SBParagraphCreateLine(paragraph, 0, 3);
    SBParagraphRelease(paragraph);
    EXPECT_EQ(SBLineGetOffset(line), 0u);
    EXPECT_EQ(SBLineGetLength(line), 3u);
    EXPECT_EQ(SBLineRetain(line), line);
    SBLineRelease(line);
    // The locator retains the line it loads, so it outlives this release.
    // (Release a mirror locator only after it has loaded a line: in 3.0.0 its
    // finalizer releases the line unconditionally, and releasing NULL crashes.)
    SBMirrorLocatorRef mirrors = SBMirrorLocatorCreate();
    SBMirrorLocatorLoadLine(mirrors, line, text.data());
    SBLineRelease(line);
    EXPECT_EQ(SBMirrorLocatorRetain(mirrors), mirrors);
    SBMirrorLocatorRelease(mirrors);
    EXPECT_FALSE(SBMirrorLocatorMoveNext(mirrors));  // "abc": nothing mirrored
    SBMirrorLocatorRelease(mirrors);
}

namespace {

struct Counting {
    int blocks = 0;
    int finalized = 0;
};

}  // namespace

TEST(Api, CustomAllocator) {
    static Counting counting;
    SBAllocatorProtocol protocol{};
    protocol.allocateBlock = [](SBUInteger size, void* info) -> void* {
        ++static_cast<Counting*>(info)->blocks;
        return std::malloc(size);
    };
    protocol.reallocateBlock = [](void* p, SBUInteger size, void*) -> void* { return std::realloc(p, size); };
    protocol.deallocateBlock = [](void* p, void*) { std::free(p); };
    protocol.finalize = [](void* info) { ++static_cast<Counting*>(info)->finalized; };
    SBAllocatorRef allocator = SBAllocatorCreate(&protocol, &counting);
    ASSERT_NE(allocator, nullptr);
    SBAllocatorRef previous = SBAllocatorGetDefault();
    SBAllocatorSetDefault(allocator);
    {
        Laid laid(utf32(U"abc אבג"), 1);
        EXPECT_EQ(digits(laid.levels()), "2221111");
    }
    EXPECT_GT(counting.blocks, 0);
    SBAllocatorSetDefault(previous);
    EXPECT_EQ(SBAllocatorRetain(allocator), allocator);
    SBAllocatorRelease(allocator);
    SBAllocatorRelease(allocator);
    EXPECT_EQ(counting.finalized, 1);
}
