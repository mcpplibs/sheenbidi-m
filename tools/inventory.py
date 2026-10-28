#!/usr/bin/env python3
"""The public surface of SheenBidi 3.0.0, from the pinned release.

Shared by tools/gen_exports.py (which writes the module sources and the
generated macro test), tools/check_exports.py (which audits them) and
tools/run_conformance.py (which needs the release's Unicode test files). The
headers come from the upstream tag archive, downloaded once into
target/upstream and checked against upstream/documents.json: the same bytes
compat.sheenbidi builds.

The public headers are the ones upstream installs (CMakeLists.txt,
SHEENBIDI_HEADERS; meson.build installs the same list). SBConfig.h, beside
them in Headers/, is not installed and no public header includes it: it is
the build's own switch file, and its two scratch-buffer sizes are internal.

The headers are read as this build compiles them: compat.sheenbidi does not
define SB_CONFIG_EXPERIMENTAL_TEXT_API (upstream's default), so
SB_TEXT_API_SUPPORTED is 0 and every `#if SB_TEXT_API_SUPPORTED` block — the
experimental SBText, attribute and run-iterator API — declares nothing.
"""
from dataclasses import dataclass, field
import hashlib
import json
from pathlib import Path
import re
import sys
import tarfile
import urllib.request

INSTALLED = [  # CMakeLists.txt: SHEENBIDI_HEADERS, in its order
    'SheenBidi.h', 'SBAlgorithm.h', 'SBAllocator.h', 'SBAttributeInfo.h', 'SBAttributeList.h',
    'SBAttributeRegistry.h', 'SBBase.h', 'SBBidiType.h', 'SBCodepoint.h', 'SBCodepointSequence.h',
    'SBGeneralCategory.h', 'SBLine.h', 'SBMirrorLocator.h', 'SBParagraph.h', 'SBRun.h', 'SBScript.h',
    'SBScriptLocator.h', 'SBText.h', 'SBTextConfig.h', 'SBTextIterators.h', 'SBTextType.h', 'SBVersion.h',
]
TEXT_API = '#if SB_TEXT_API_SUPPORTED'


def fetch(root: Path) -> Path:
    pins = json.loads((root / 'upstream/documents.json').read_text(encoding='utf-8'))
    cache = root / 'target/upstream'
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / ('SheenBidi-' + pins['upstream_version'] + '.tar.gz')
    if not archive.exists() or hashlib.sha256(archive.read_bytes()).hexdigest() != pins['upstream_sha256']:
        print('+ fetch', pins['upstream'], flush=True)
        with urllib.request.urlopen(pins['upstream'], timeout=120) as response:
            archive.write_bytes(response.read())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != pins['upstream_sha256']:
        sys.exit(f'upstream archive checksum {digest} != {pins["upstream_sha256"]}')
    with tarfile.open(archive) as tar:
        top = tar.getmembers()[0].name.split('/')[0]
        if not (cache / top / 'Headers/SheenBidi/SheenBidi.h').exists():
            tar.extractall(cache, filter='data')
    return cache / top


@dataclass
class Macro:
    name: str
    header: str
    params: list[str] | None   # None: object-like
    body: str


@dataclass
class Surface:
    types: list[str] = field(default_factory=list)        # typedef names
    tags: list[str] = field(default_factory=list)         # struct tags (_SBRun, _SBLine …)
    enumerators: dict[str, str] = field(default_factory=dict)  # enumerator: the typedef the header gives its values
    functions: list[str] = field(default_factory=list)
    deprecated: list[str] = field(default_factory=list)
    macros: dict[str, Macro] = field(default_factory=dict)


def strip(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def cut_text_api(text: str) -> str:
    """The header without its `#if SB_TEXT_API_SUPPORTED` block (0 in this build)."""
    start = text.find(TEXT_API)
    if start < 0:
        return text
    depth, i = 0, start
    for m in re.finditer(r'^[ \t]*#[ \t]*(if|ifdef|ifndef|endif)\b', text[start:], flags=re.M):
        depth += 1 if m.group(1) != 'endif' else -1
        if depth == 0:
            i = start + m.end()
            break
    else:
        sys.exit('unterminated ' + TEXT_API)
    return text[:start] + text[i:]


def surface(upstream: Path) -> Surface:
    headers = upstream / 'Headers/SheenBidi'
    s = Surface()
    for name in INSTALLED:
        raw = (headers / name).read_text(encoding='utf-8')
        text = cut_text_api(strip(raw))
        for m in re.finditer(r'^[ \t]*#[ \t]*define[ \t]+(\w+)(\(([^)]*)\))?[ \t]*((?:[^\n]*\\\n)*[^\n]*)', text, flags=re.M):
            params = [p.strip() for p in m.group(3).split(',')] if m.group(2) else None
            body = ' '.join(m.group(4).replace('\\\n', ' ').split())
            s.macros.setdefault(m.group(1), Macro(m.group(1), name, params, body))
        if TEXT_API in raw and re.search(r'\bSB_PUBLIC\b|\btypedef\b', cut_text_api(strip(raw))):
            sys.exit(f'{name}: declarations outside {TEXT_API}')
        # Declarations only: the preprocessor's lines are not.
        code = re.sub(r'^[ \t]*#(?:[^\n]*\\\n)*[^\n]*', '', text, flags=re.M)
        code = re.sub(r'\bSB_EXTERN_C_(?:BEGIN|END)\b', ' ', code)
        # Unnamed enums, each followed by the typedef the header gives its values
        # (`enum { SBBidiTypeL = 0x01, … }; typedef SBUInt8 SBBidiType;`).
        for m in re.finditer(r'\benum\s*\{(.*?)\}\s*;\s*typedef\s+\w+\s+(\w+)\s*;', code, flags=re.S):
            for e in re.findall(r'\b([A-Za-z_]\w*)\s*(?:=[^,}]*)?(?:,|$)', m.group(1).strip()):
                s.enumerators[e] = m.group(2)
        if len(re.findall(r'\benum\b', code)) != len(re.findall(r'\benum\s*\{.*?\}\s*;\s*typedef', code, flags=re.S)):
            sys.exit(f'{name}: an enum the inventory does not know (named, or without its typedef)')
        code = re.sub(r'\benum\s*\{.*?\}\s*;', ';', code, flags=re.S)
        # struct definitions: the tag, then the typedef name after the braces.
        for m in re.finditer(r'\btypedef\s+struct\s+(\w+)\s*\{.*?\}\s*(\w+)\s*;', code, flags=re.S):
            s.tags.append(m.group(1))
            s.types.append(m.group(2))
        code = re.sub(r'\btypedef\s+struct\s+\w+\s*\{.*?\}\s*\w+\s*;', ';', code, flags=re.S)
        # Opaque references: `typedef const struct _SBLine *SBLineRef;`.
        for m in re.finditer(r'\btypedef\s+(?:const\s+)?struct\s+(\w+)\s*\*\s*(\w+)\s*;', code):
            s.tags.append(m.group(1))
            s.types.append(m.group(2))
        code = re.sub(r'\btypedef\s+(?:const\s+)?struct\s+\w+\s*\*\s*\w+\s*;', ';', code)
        # Function pointer types: `typedef void *(*SBAllocatorAllocateBlockFunc)(…);`.
        s.types += re.findall(r'\btypedef\b[^;(]*\(\s*\*\s*(\w+)\s*\)\s*\([^;]*\)\s*;', code)
        code = re.sub(r'\btypedef\b[^;(]*\(\s*\*\s*\w+\s*\)\s*\([^;]*\)\s*;', ';', code)
        # Plain typedefs: `typedef SBUInt8 SBLevel;`.
        s.types += re.findall(r'\btypedef\s+[\w\s\*]+?\b(\w+)\s*;', code)
        code = re.sub(r'\btypedef\b[^;]*;', ';', code)
        # Functions: every one is SB_PUBLIC, some SB_DEPRECATED before it.
        for m in re.finditer(r'(\bSB_DEPRECATED\s+)?\bSB_PUBLIC\b([^;{]*);', code):
            f = re.search(r'\b(\w+)\s*\(', m.group(2))
            if not f:
                sys.exit(f'{name}: SB_PUBLIC on something that is not a function: {m.group(2).strip()}')
            s.functions.append(f.group(1))
            if m.group(1):
                s.deprecated.append(f.group(1))
        rest = re.sub(r'(\bSB_DEPRECATED\s+)?\bSB_PUBLIC\b[^;{]*;', ';', code)
        if re.search(r'\w', rest.replace(';', ' ')):
            sys.exit(f'{name}: declarations the inventory does not know: {" ".join(rest.split())[:200]}')
    s.types = sorted(set(s.types))
    s.tags = sorted(set(s.tags))
    s.functions = sorted(set(s.functions))
    s.deprecated = sorted(set(s.deprecated))
    return s


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    sf = surface(fetch(root))
    print(f'{len(sf.types)} types, {len(sf.tags)} struct tags, {len(sf.enumerators)} enumerators, '
          f'{len(sf.functions)} functions ({len(sf.deprecated)} deprecated), {len(sf.macros)} macros')
    print('types:', ' '.join(sf.types))
    print('tags:', ' '.join(sf.tags))
    print('enumerator types:', ' '.join(sorted(set(sf.enumerators.values()))))
    print('functions:', ' '.join(sf.functions))
    print('deprecated:', ' '.join(sf.deprecated))
    print('object-like:', ' '.join(m.name for m in sf.macros.values() if m.params is None))
    print('function-like:', ' '.join(m.name for m in sf.macros.values() if m.params is not None))
