# Using mcpplibs.mwrap.sheenbidi

`import mcpplibs.mwrap.sheenbidi;` declares SheenBidi 3.0.0's API as `<SheenBidi/SheenBidi.h>` does. Types, struct tags and functions are the upstream globals, the very same entities as the header's, so a translation unit may include the header too. The library is `compat.sheenbidi` 3.0.0, built from the release's unity source with its Unicode 17.0.0 tables.

## Modules

| Module | What it holds |
|---|---|
| `mcpplibs.mwrap.sheenbidi` | every type (`SBCodepoint`, `SBLevel`, `SBBidiType`, `SBGeneralCategory`, `SBScript`, `SBStringEncoding`, `SBBoolean`, `SBUInteger` …, the structs `SBCodepointSequence`, `SBRun`, `SBMirrorAgent`, `SBScriptAgent`, `SBAllocatorProtocol`, the object references `SBAlgorithmRef`, `SBParagraphRef`, `SBLineRef`, `SBMirrorLocatorRef`, `SBScriptLocatorRef`, `SBAllocatorRef` and the allocator callback types), the 11 struct tags (`_SBRun` …; the opaque ones declared, not defined) and all 51 `SB_PUBLIC` functions; re-exports the module below |
| `mcpplibs.mwrap.sheenbidi.macros` | the macros and the enumerators, in `mcpplibs::mwrap::sheenbidi::macros` |

The public headers are the ones upstream installs (`SHEENBIDI_HEADERS` in CMakeLists.txt). `SBConfig.h` is not among them: no public header includes it, and its switches are the build's.

### The API in one paragraph

`SBAlgorithmCreate` takes an `SBCodepointSequence` (UTF-8, UTF-16 or UTF-32 code units; offsets everywhere are in those code units) and finds the bidi types and the paragraph boundaries (P1, `SBAlgorithmGetParagraphBoundary`). `SBAlgorithmCreateParagraph` resolves one paragraph at a base level, explicit (0, 1) or from its first strong character (`SBLevelDefaultLTR`, `SBLevelDefaultRTL`), and gives the level of every code unit (`SBParagraphGetLevelsPtr`). `SBParagraphCreateLine` applies L1 to a line and gives its runs in visual order (`SBLineGetRunsPtr`: draw a run with an odd level right to left). `SBMirrorLocator` walks the characters of a line that are drawn mirrored (L4); `SBScriptLocator` walks the script runs of a sequence (for shaping). Every object is reference counted (`…Retain`, `…Release`); a paragraph keeps its algorithm, a locator its line. `SBAllocatorCreate` / `SBAllocatorSetDefault` route the library's allocations.

### Not exported: the experimental text API

`compat.sheenbidi` does not define `SB_CONFIG_EXPERIMENTAL_TEXT_API` (upstream's default builds do not), so `SB_TEXT_API_SUPPORTED` is 0 and `SBText`, `SBTextConfig`, the attribute registry and the run iterators declare nothing: neither the header nor the module has them.

### Upstream behavior worth knowing

- **Release a mirror locator only after it has loaded a line.** In 3.0.0 its finalizer releases the loaded line unconditionally, and `SBLineRelease(NULL)` dereferences the null pointer. `SBMirrorLocatorLoadLine` also leaves no line when `stringBuffer` is not the buffer the line was created over.
- `SBScriptGetOpenTypeTag` is `SB_DEPRECATED` upstream (its replacement is `SBScriptGetUnicodeTag`); the module exports it as it is, deprecation included.

## Macros and enumerators

A named module cannot export a macro, nor an enumerator of an unnamed enum (the enumeration is TU-local). `mcpplibs::mwrap::sheenbidi::macros` holds both under their original names; `using namespace mcpplibs::mwrap::sheenbidi::macros;` makes code read as it does with the header.

- **Constants** as `inline constexpr`, each of the type its macro expands to: `SBLevelInvalid`, `SBLevelMax`, `SBLevelDefaultLTR`, `SBLevelDefaultRTL`, `SBCodepointFaulty`, `SBCodepointMax`, `SHEENBIDI_VERSION_MAJOR` / `_MINOR` / `_PATCH` and `SB_TEXT_API_SUPPORTED` are `int`; `SBCodepointInvalid` is `uint32_t` (`UINT32_MAX`). `SHEENBIDI_VERSION_STRING` is a reference to a constexpr char array.
- **Enumerators** as constants of the typedef the header declares right after their enum: the 24 `SBBidiType*` are `SBBidiType`, the 31 `SBGeneralCategory*` `SBGeneralCategory`, the 176 `SBScript*` `SBScript`, the 3 `SBStringEncoding*` `SBStringEncoding` and `SBTrue` / `SBFalse` `SBBoolean`. They work in constant expressions and case labels as the enumerators do.
- **Computations** as `constexpr` function templates: `SBUInt8InRange`, `SBUInt16InRange`, `SBUInt32InRange`, `SBBidiTypeIsStrong` / `IsWeak` / `IsNeutral` / `IsFormat`, `SBCodepointIsSurrogate`, `SBCodepointIsValid`, `SBGeneralCategoryIsLetter` / `Mark` / `Number` / `Punctuation` / `Symbol` / `Separator` / `Other`. Each body is the macro itself, expanded on the parameters: the result has the macro's value and type (`bool`) for every argument type. Each argument is evaluated once, before the call; `SBCodepointIsValid(c)` and the `…InRange` macros evaluate an argument more than once, so for an argument without side effects the two agree.

Each mapping is checked against the raw header for every sample argument type (every 8-bit value, 16- and 32-bit boundaries, code points around the surrogates and past U+10FFFF, ints), and each enumerator constant against the header's enumerator, in [tests/macros_mix_test.cpp](../tests/macros_mix_test.cpp).

### What stays a macro

These stay macros in `<mwrap/sheenbidi/macros.hpp>`, which is `<SheenBidi/SheenBidi.h>`:

| Macros | Why |
|---|---|
| `SB_PUBLIC`, `SB_DEPRECATED` | attributes of declarations (`SB_PUBLIC` is empty here: the library is static and built without `SB_CONFIG_DLL_*`) |
| `SB_EXTERN_C_BEGIN`, `SB_EXTERN_C_END` | open and close an `extern "C"` block |

`SB_TEXT_API_SUPPORTED` is both a constant and, through the side header, a macro for `#if`.

Include the side header before the import. While a header macro is defined, it expands even inside a qualified name: an object-like macro therefore hides `macros::SBLevelMax`; use the plain macro, or set it aside with `#pragma push_macro`. A function-like macro does not expand when its name is followed by `)`, so `(macros::SBBidiTypeIsStrong)(t)` calls the function beside the header. With the header included, its enumerators are its own (`::SBBidiTypeL`); do not also bring `macros` into scope with a using-directive, or the unqualified names are ambiguous ([tests/header_mix_test.cpp](../tests/header_mix_test.cpp)). [tools/check_exports.py](../tools/check_exports.py) accounts for all 54 macros and 236 enumerators.

## Licensing

The wrapper's code is Apache-2.0. SheenBidi is Apache-2.0 ([upstream/sheenbidi/LICENSE](../upstream/sheenbidi/LICENSE)); a program built with it, statically linked, carries the Apache-2.0 notice requirements only. Its tables are generated from the Unicode Character Database 17.0.0 ([Unicode License v3](../upstream/unicode/license.txt)).

`runtime.deploy` copies the wrapper's LICENSE and NOTICE, SheenBidi's README and LICENSE and the Unicode license into `licenses/sheenbidi/` beside every program that depends on this package.

## Checks

- `mcpp test` runs four tests:
  - [tests/api_test.cpp](../tests/api_test.cpp): the API through the module only — bidi types (Unicode 17.0's Sidetic letters included), general categories, mirrors, scripts and their ISO 15924 tags, UTF-8 / UTF-16 decoding and code point sequences, paragraph boundaries (CR LF) and base levels, levels and visual order, UAX #9's N0 bracket-pair example with the mirror locator, isolates, the script locator, retain/release, a custom default allocator, the version;
  - [tests/macros_mix_test.cpp](../tests/macros_mix_test.cpp) (generated): every constant, enumerator and function against the header;
  - [tests/header_mix_test.cpp](../tests/header_mix_test.cpp): the header beside the import — the same entities, `SB_EXTERN_C_BEGIN` and `SB_PUBLIC` on a declaration of one's own, `SB_DEPRECATED`;
  - [tests/macros_test.cpp](../tests/macros_test.cpp): the macros module alone, at compile time.
- `python3 tools/check_exports.py` checks that the sources are what [tools/gen_exports.py](../tools/gen_exports.py) writes from the pinned headers, and that every declaration is exported, every enumerator mapped, and every macro mapped, kept or an include guard.
- `python3 tools/run_conformance.py` runs Unicode's conformance tests on [examples/sheenbidi](../examples/sheenbidi), written against the module: all 770,241 cases of BidiTest.txt (each line under each paragraph direction it lists: levels after L1 and visual order) and all 91,707 of BidiCharacterTest.txt (paragraph level, levels, visual order, and the mirrored characters against BidiMirroring.txt). The three files are the release's, each checked to be byte for byte the one Unicode publishes for 17.0.0. Nothing is excluded.
- `python3 tools/test_template.py` scaffolds the default template from an installed copy of the package, then builds and runs it.
