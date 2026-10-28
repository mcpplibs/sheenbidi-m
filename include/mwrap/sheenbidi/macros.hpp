// <mwrap/sheenbidi/macros.hpp> — SheenBidi's macros as macros, for the ones a
// C++ entity cannot be: the declaration attributes (SB_PUBLIC, SB_DEPRECATED)
// and the extern "C" brackets (SB_EXTERN_C_BEGIN / SB_EXTERN_C_END).
//
// Include it before `import mcpplibs.mwrap.sheenbidi;`: it is
// <SheenBidi/SheenBidi.h>, as the module sees it (compat.sheenbidi is a static
// library built without SB_CONFIG_DLL_*, so SB_PUBLIC is empty everywhere, and
// without SB_CONFIG_EXPERIMENTAL_TEXT_API, so SB_TEXT_API_SUPPORTED is 0). Every
// other macro is defined too, and while a macro is defined it expands even
// inside `mcpplibs::mwrap::sheenbidi::macros::NAME`; a function-like one does
// not when its name is followed by `)`: `(macros::SBBidiTypeIsStrong)(t)` is
// the function. The header's enumerators (SBBidiTypeL …) are its own here, and
// `macros::SBBidiTypeL` the module's constant of the same value.
#pragma once
#include <SheenBidi/SheenBidi.h>
