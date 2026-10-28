#!/usr/bin/env python3
"""Audit the module surface against the pinned release's public SheenBidi headers.

- src/sheenbidi.cppm, src/macros.cppm and tests/macros_mix_test.cpp are what
  tools/gen_exports.py writes.
- Every type, struct tag and SB_PUBLIC function of the installed headers is
  exported by `export using`, and nothing else is.
- Every enumerator of the headers' unnamed enums is a constant of its
  typedef in src/macros.cppm.
- Every macro of the headers is mapped in src/macros.cppm (a constant or a
  function), kept as a macro in <mwrap/sheenbidi/macros.hpp> with its
  reason, or an include guard; and each mapping, enumerators included, is
  checked against the raw header by tests/macros_mix_test.cpp.
"""
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import fetch, surface  # noqa: E402
from gen_exports import SIDE_HEADER, excluded  # noqa: E402


def mapped_names(text: str) -> set[str]:
    text = text[text.index('export module'):]
    names = set(re.findall(r'^inline constexpr auto&? (\w+) = c::value_\1;', text, flags=re.M))
    names |= set(re.findall(r'^constexpr auto \((\w+)\)\(', text, flags=re.M))
    return names


def main():
    root = Path(__file__).resolve().parents[1]
    problems = []
    generated = subprocess.run([sys.executable, str(root / 'tools/gen_exports.py'), '--check'], capture_output=True, text=True)
    if generated.returncode != 0:
        problems.append(generated.stderr.strip() or generated.stdout.strip())
    sf = surface(fetch(root))
    module = (root / 'src/sheenbidi.cppm').read_text(encoding='utf-8')
    exported = set(re.findall(r'^export using ::(\w+);', module, flags=re.M))
    declared = set(sf.types) | set(sf.tags) | set(sf.functions)
    if declared - exported:
        problems.append('src/sheenbidi.cppm lacks: ' + ', '.join(sorted(declared - exported)))
    if exported - declared:
        problems.append('src/sheenbidi.cppm exports what the headers do not declare: ' + ', '.join(sorted(exported - declared)))

    macros_text = (root / 'src/macros.cppm').read_text(encoding='utf-8')
    enumerators = {m.group(2): m.group(1)  # enumerator: its typedef
                   for m in re.finditer(r'^inline constexpr ::(\w+) (\w+) = ::\2;', macros_text, flags=re.M)}
    if enumerators != sf.enumerators:
        missing = sf.enumerators.keys() - enumerators.keys()
        wrong = {n for n in sf.enumerators.keys() & enumerators.keys() if sf.enumerators[n] != enumerators[n]}
        problems.append(f'enumerators: missing {sorted(missing)}, of the wrong type {sorted(wrong)}, '
                        f'extra {sorted(enumerators.keys() - sf.enumerators.keys())}')

    mapped = mapped_names(macros_text)
    macros = set(sf.macros)
    guards = {m for m in macros if excluded(m)}
    unaccounted = macros - mapped - SIDE_HEADER.keys() - guards
    if unaccounted:
        problems.append('macros neither mapped, side-header nor include guards: ' + ', '.join(sorted(unaccounted)))
    both = SIDE_HEADER.keys() & mapped
    if both:
        problems.append('listed as side-header yet mapped: ' + ', '.join(sorted(both)))
    stale = SIDE_HEADER.keys() - macros
    if stale:
        problems.append('listed but not in the headers: ' + ', '.join(sorted(stale)))
    test = (root / 'tests/macros_mix_test.cpp').read_text(encoding='utf-8')
    untested = {n for n in mapped | enumerators.keys() if f'm::{n}' not in test}
    if untested:
        problems.append('mapped but not checked against the header: ' + ', '.join(sorted(untested)))
    side = (root / 'include/mwrap/sheenbidi/macros.hpp').read_text(encoding='utf-8')
    if '#include <SheenBidi/SheenBidi.h>' not in side:
        problems.append('<mwrap/sheenbidi/macros.hpp> does not include <SheenBidi/SheenBidi.h>')
    if problems:
        sys.exit('\n'.join(problems))
    functions = sum(1 for m in sf.macros.values() if m.name in mapped and m.params is not None)
    print(f'{len(sf.types)} types, {len(sf.tags)} struct tags and {len(sf.functions)} functions exported; '
          f'{len(enumerators)} enumerators of unnamed enums as constants of their typedefs; {len(macros)} macros: '
          f'{len(mapped) - functions} constants and {functions} functions mapped (each checked against the header), '
          f'{len(SIDE_HEADER)} kept as macros, {len(guards)} include guards')


if __name__ == '__main__':
    main()
