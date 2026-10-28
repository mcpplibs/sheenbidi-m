# sheenbidi

> SheenBidi, the Unicode Bidirectional Algorithm, through C++23 modules.

**English** · [简体中文](README.zh.md) · [Upstream](https://github.com/Tehreer/SheenBidi) · [Releases](https://github.com/mcpplibs/sheenbidi-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![SheenBidi](https://img.shields.io/badge/SheenBidi-Apache--2.0-blue.svg)](upstream/sheenbidi/LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/sheenbidi-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/sheenbidi-m/actions/workflows/ci.yml)

SheenBidi turns text stored in logical order into the visual order a display
needs for Arabic, Hebrew and mixed-direction text (UAX #9, Unicode 17.0.0). It
is Apache-2.0 C with no dependencies, so it links statically without copyleft
terms. The upstream C API is unchanged: `SBAlgorithmCreate`,
`SBParagraphGetLevelsPtr`, `SBLineGetRunsPtr` and `SBMirrorLocatorMoveNext`
are the upstream globals of `<SheenBidi/SheenBidi.h>`. The library is
`compat.sheenbidi` 3.0.0. This package maps SheenBidi's macros and the
enumerators of its unnamed enums to constants and constexpr functions, each
checked against the header. Unicode's conformance tests pass on every CI run:
all 770,241 cases of BidiTest.txt and all 91,707 of BidiCharacterTest.txt, on
programs written against the module.

## Modules

- `import mcpplibs.mwrap.sheenbidi;`: the whole API of `<SheenBidi/SheenBidi.h>` (SBAlgorithm, SBParagraph, SBLine, SBRun, SBMirrorLocator, SBScriptLocator, SBCodepoint, SBCodepointSequence, SBAllocator, SBVersion), re-exporting the module below
- `import mcpplibs.mwrap.sheenbidi.macros;`: `SBBidiTypeR`, `SBScriptARAB`, `SBStringEncodingUTF8`, `SBLevelDefaultLTR`, `SBBidiTypeIsStrong` … in `mcpplibs::mwrap::sheenbidi::macros`

> More Modules API -> [Usage and API](docs/usage.md)

## Getting started

```sh
mcpp new bidi --template mcpplibs.mwrap.sheenbidi@3.0.0
cd bidi
mcpp run
```

The default template lays out UTF-8 lines as a renderer does: it resolves a
paragraph's embedding levels, then draws the line's runs in visual order with
mirrored brackets. Run `mcpp run -- "your text"` for your own lines.

```toml
[dependencies]
"mcpplibs.mwrap.sheenbidi" = "3.0.0"
```

```cpp
import std;
import mcpplibs.mwrap.sheenbidi;
using namespace mcpplibs::mwrap::sheenbidi::macros;  // SBStringEncodingUTF8, SBLevelDefaultLTR …

int main() {
    std::string_view text = "abc אבג (123)";
    SBCodepointSequence seq{SBStringEncodingUTF8, text.data(), text.size()};
    SBAlgorithmRef algorithm = SBAlgorithmCreate(&seq);
    SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, 0, text.size(), SBLevelDefaultLTR);
    SBLineRef line = SBParagraphCreateLine(paragraph, 0, SBParagraphGetLength(paragraph));
    const SBRun* runs = SBLineGetRunsPtr(line);  // in visual order, offsets in UTF-8 bytes
    for (SBUInteger i = 0; i < SBLineGetRunCount(line); ++i)
        std::println("[{}, {}) level {}", runs[i].offset, runs[i].offset + runs[i].length, runs[i].level);
    SBLineRelease(line);
    SBParagraphRelease(paragraph);
    SBAlgorithmRelease(algorithm);
}
```

## Macros and enumerators

Constants such as `SBLevelMax` keep the type their macro has (`int`).
Computations such as `SBBidiTypeIsStrong` and `SBCodepointIsValid` are
constexpr functions whose body is the macro itself, matching its value and
type for every argument type. SheenBidi's enumerators (`SBBidiTypeL`,
`SBScriptARAB` …) belong to unnamed enums, which a module cannot export, so
they are constants of the typedef the header gives them (`SBBidiType`,
`SBScript` …). `SB_PUBLIC`, `SB_DEPRECATED` and the `extern "C"` brackets stay
macros in `<mwrap/sheenbidi/macros.hpp>`, included before the import. Details:
[docs/usage.md](docs/usage.md).

## Examples

- [examples/sheenbidi](examples/sheenbidi): a layout tool (logical to visual order, levels, runs and
  script runs of each paragraph) and Unicode's two conformance runners, written against the module.
  [tools/run_conformance.py](tools/run_conformance.py) runs BidiTest.txt and BidiCharacterTest.txt on them.

## Build and testing

Linux (x86_64 GCC and LLVM, arm64 GCC), macOS and Windows CI build the
modules, then run the tests and the export audit
([tools/check_exports.py](tools/check_exports.py)). The audit checks that the
generated sources are fresh, that every declaration is exported and that
every macro is mapped or listed with its reason. CI also runs Unicode 17.0.0's
conformance tests from the pinned release, each file checked against the one
Unicode publishes: BidiTest.txt (770,241 cases) and BidiCharacterTest.txt
(91,707 cases, with mirroring). A separate job scaffolds the default template
as an installed package and runs it.

## License

[Apache-2.0](LICENSE) for the wrapper, by mcpp-community ([NOTICE](NOTICE)).
SheenBidi is Apache-2.0 too ([upstream/sheenbidi/LICENSE](upstream/sheenbidi/LICENSE)).
Its tables come from the Unicode Character Database
([Unicode License v3](upstream/unicode/license.txt)). Both notices are
deployed beside every program that depends on this package.
