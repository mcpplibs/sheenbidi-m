// The macros module on its own (no header, no root module): the constants,
// enumerators and functions are constant expressions, as the macros and
// enumerators are, so they work in constant-initialised tables, case labels
// and static_asserts.
import std;
import mcpplibs.mwrap.sheenbidi.macros;

namespace macros = mcpplibs::mwrap::sheenbidi::macros;
using namespace mcpplibs::mwrap::sheenbidi::macros;

// Every constant has the type its macro expands to.
static_assert(std::is_same_v<decltype(SBLevelMax), const int>);              // 125
static_assert(std::is_same_v<decltype(SBLevelInvalid), const int>);          // 0xFF
static_assert(std::is_same_v<decltype(SBCodepointInvalid), const std::uint32_t>);  // UINT32_MAX
static_assert(std::is_same_v<decltype(SHEENBIDI_VERSION_STRING), const char (&)[6]>);
static_assert(SBLevelMax == 125 && SBLevelInvalid == 0xFF);
static_assert(SBLevelDefaultLTR == 0xFE && SBLevelDefaultRTL == 0xFD);
static_assert(SBCodepointMax == 0x10FFFF && SBCodepointFaulty == 0xFFFD);
static_assert(std::string_view(SHEENBIDI_VERSION_STRING) == "3.0.0" && SHEENBIDI_VERSION_MINOR == 0);
static_assert(SB_TEXT_API_SUPPORTED == 0);  // compat.sheenbidi: the experimental text API is off

// Every enumerator is a constant of the typedef the header gives its values.
static_assert(std::is_same_v<decltype(SBBidiTypeL), const std::uint8_t>);       // SBBidiType
static_assert(std::is_same_v<decltype(SBStringEncodingUTF16), const std::uint32_t>);  // SBStringEncoding
static_assert(std::is_same_v<decltype(SBTrue), const std::uint8_t>);            // SBBoolean
static_assert(SBBidiTypeNil == 0 && SBBidiTypeL == 1 && SBBidiTypeAL == 3 && SBBidiTypePDF == 0x17);
static_assert(SBGeneralCategoryLU == 1 && SBScriptARAB == 4 && SBScriptTOLS == 0xAF);  // Tolong Siki: Unicode 17.0
static_assert(SBStringEncodingUTF8 == 0 && SBStringEncodingUTF32 == 2 && SBTrue == 1 && SBFalse == 0);

// The functions, at compile time.
static_assert(SBBidiTypeIsStrong(SBBidiTypeR) && !SBBidiTypeIsStrong(SBBidiTypeEN));
static_assert(SBBidiTypeIsWeak(SBBidiTypeCS) && SBBidiTypeIsNeutral(SBBidiTypeWS) && SBBidiTypeIsFormat(SBBidiTypeLRI));
static_assert(SBCodepointIsSurrogate(0xD800u) && !SBCodepointIsValid(0xDFFFu) && !SBCodepointIsValid(0x110000u));
static_assert(SBCodepointIsValid(0x10FFFFu) && SBCodepointIsValid(0u));
static_assert(SBGeneralCategoryIsLetter(SBGeneralCategoryLO) && SBGeneralCategoryIsMark(SBGeneralCategoryMN));
static_assert(SBGeneralCategoryIsNumber(SBGeneralCategoryND) && SBGeneralCategoryIsOther(SBGeneralCategoryCN));
static_assert(SBUInt8InRange(5, 3, 7) && !SBUInt8InRange(2, 3, 7) && SBUInt32InRange(0x10940u, 0x10940u, 0x1095Fu));
static_assert(std::is_same_v<decltype(SBUInt16InRange(1, 0, 2)), bool>);

// A constant-initialised table and a switch, as the enumerators allow.
struct Class {
    std::uint8_t type;
    bool strong;
};
constexpr Class classes[] = {
    {SBBidiTypeL, SBBidiTypeIsStrong(SBBidiTypeL)},
    {SBBidiTypeAN, SBBidiTypeIsStrong(SBBidiTypeAN)},
};
static_assert(classes[0].strong && !classes[1].strong);

constexpr const char* name(std::uint8_t type) {
    switch (type) {
    case SBBidiTypeL: return "L";
    case SBBidiTypeR: return "R";
    case SBBidiTypeAL: return "AL";
    default: return "other";
    }
}
static_assert(std::string_view(name(SBBidiTypeAL)) == "AL");

int main() {
    return macros::SHEENBIDI_VERSION_MAJOR == 3 && macros::SBLevelMax == 125 ? 0 : 1;
}
