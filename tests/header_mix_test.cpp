// The raw header and the import in one translation unit (the side header
// first, the canonical order): the module's names are the very same entities
// as the header's; the macros a C++ entity cannot be work beside them — the
// extern "C" brackets and SB_PUBLIC on a declaration of one's own, and
// SB_DEPRECATED.
#include <mwrap/sheenbidi/macros.hpp>
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.sheenbidi;

// A declaration of one's own in SheenBidi's style, inside its extern "C" brackets.
SB_EXTERN_C_BEGIN
SB_PUBLIC int mwrap_sheenbidi_test_entry(int value);
SB_EXTERN_C_END
int mwrap_sheenbidi_test_entry(int value) { return value + 1; }

SB_DEPRECATED int mwrap_sheenbidi_old(int value);
int mwrap_sheenbidi_old(int value) { return value; }

TEST(HeaderMix, SameEntities) {
    // One declaration each, whichever way it was introduced.
    static_assert(std::is_same_v<decltype(&::SBAlgorithmCreate), decltype(&SBAlgorithmCreate)>);
    static_assert(std::is_same_v<::SBLevel, std::uint8_t> && std::is_same_v<::SBUInteger, std::uintptr_t>);
    static_assert(std::is_same_v<::SBRun, ::_SBRun> && sizeof(SBRun) == sizeof(::SBRun));
    EXPECT_EQ(&::SBCodepointGetBidiType, &SBCodepointGetBidiType);
    EXPECT_EQ(::SBCodepointGetBidiType(0x05D0), SBBidiTypeR);  // the header's enumerator
    EXPECT_EQ(mwrap_sheenbidi_test_entry(41), 42);
}

TEST(HeaderMix, EnumeratorsBesideTheConstants) {
    namespace m = mcpplibs::mwrap::sheenbidi::macros;
    // The header's enumerators (of an unnamed enum) and the module's constants
    // have the same values; the constants have the header's typedef.
    static_assert(m::SBBidiTypeAL == SBBidiTypeAL && m::SBScriptHEBR == SBScriptHEBR);
    static_assert(std::is_same_v<decltype(m::SBBidiTypeAL), const SBBidiType>);
    for (SBCodepoint c : {U'a', U'א', U'ا', U'1', U'١'}) {
        const SBBidiType t = SBCodepointGetBidiType(c);
        EXPECT_EQ(SBBidiTypeIsStrong(t), (m::SBBidiTypeIsStrong)(t));
        EXPECT_EQ(SBBidiTypeIsWeak(t), (m::SBBidiTypeIsWeak)(t));
    }
}

TEST(HeaderMix, MacrosBesideTheMappings) {
    namespace m = mcpplibs::mwrap::sheenbidi::macros;
    // An object-like macro expands even in a qualified name: set it aside to
    // name the constant.
    constexpr auto raw = SBLevelDefaultLTR;
#pragma push_macro("SBLevelDefaultLTR")
#undef SBLevelDefaultLTR
    static_assert(m::SBLevelDefaultLTR == raw);
#pragma pop_macro("SBLevelDefaultLTR")
    // A function-like macro is not expanded when its name is followed by `)`.
    EXPECT_EQ(SBCodepointIsValid(0x10FFFFu), (m::SBCodepointIsValid)(0x10FFFFu));
    // The experimental text API is compiled out, in the header as in the module.
#if SB_TEXT_API_SUPPORTED
    FAIL() << "compat.sheenbidi is built without SB_CONFIG_EXPERIMENTAL_TEXT_API";
#endif
}

TEST(HeaderMix, DeprecatedFunction) {
    // SBScriptGetOpenTypeTag is SB_DEPRECATED upstream; the module exports it
    // as it is. Its replacement is SBScriptGetUnicodeTag.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT_EQ(SBScriptGetOpenTypeTag(SBScriptARAB), 0x61726162u);  // 'arab'
    EXPECT_EQ(mwrap_sheenbidi_old(7), 7);
#pragma GCC diagnostic pop
    EXPECT_EQ(SBScriptGetUnicodeTag(SBScriptARAB), 0x41726162u);  // 'Arab'
}
