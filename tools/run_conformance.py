#!/usr/bin/env python3
"""Run Unicode's bidi conformance tests on examples/sheenbidi, programs
written against import mcpplibs.mwrap.sheenbidi.

The release tag archive is downloaded once into target/upstream and checked
against upstream/documents.json. Its Tools/Unicode/BidiTest.txt,
BidiCharacterTest.txt and BidiMirroring.txt are checked against the
checksums recorded there — those of the files Unicode publishes for 17.0.0
(https://www.unicode.org/Public/17.0.0/ucd/) — and used unchanged:

- BidiTest.txt: every data line under each paragraph direction its bitset
  lists, the levels after L1 and the visual order;
- BidiCharacterTest.txt: every line — the paragraph level, the levels after
  L1, the visual order, and the mirrored characters against BidiMirroring.txt.

Nothing is excluded. Every test must pass, and the counts must be the
release's (a guard against reading nothing).
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import fetch  # noqa: E402

# Unicode 17.0.0's counts.
BIDI_TESTS = 770241
BIDI_CHARACTER_TESTS = 91707


def build(example: Path, mcpp: str, toolchain: str | None) -> Path:
    command = [mcpp, 'build', '--profile', 'release'] + (['--toolchain', toolchain] if toolchain else [])
    print('+', ' '.join(command), flush=True)
    subprocess.run(command, cwd=example, check=True)
    suffix = '.exe' if os.name == 'nt' else ''
    found = sorted((p for p in (example / 'target').rglob('BidiTest' + suffix) if p.parent.name == 'bin'),
                   key=lambda p: p.stat().st_mtime)
    if not found:
        sys.exit('examples/sheenbidi was not built')
    return found[-1].parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcpp', default='mcpp')
    parser.add_argument('--toolchain')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    upstream = fetch(root)
    pins = json.loads((root / 'upstream/documents.json').read_text(encoding='utf-8'))
    for data in pins['conformance']:
        digest = hashlib.sha256((upstream / data['path']).read_bytes()).hexdigest()
        if digest != data['sha256']:
            sys.exit(f'{data["path"]}: {digest} is not {data["source"]} ({data["sha256"]})')
        print(f'ok   {data["path"]} is {data["source"]}', flush=True)
    bin_dir = build(root / 'examples/sheenbidi', args.mcpp, args.toolchain)
    exe = lambda name: str(bin_dir / (name + ('.exe' if os.name == 'nt' else '')))  # noqa: E731
    unicode = upstream / 'Tools/Unicode'

    failed, counts = [], {}
    for runner, files in (('BidiTest', ['BidiTest.txt']),
                          ('BidiCharacterTest', ['BidiCharacterTest.txt', 'BidiMirroring.txt'])):
        run = subprocess.run([exe(runner)] + [str(unicode / f) for f in files], capture_output=True, text=True,
                             timeout=1800)
        summary = re.search(runner + r': (\d+) tests, (\d+) errors', run.stdout)
        counts[runner] = int(summary.group(1)) if summary else 0
        ok = run.returncode == 0 and summary is not None and summary.group(2) == '0'
        print(f'{"ok  " if ok else "FAIL"} {runner} {files[0]}: {summary.group(0) if summary else "no summary"}',
              flush=True)
        if not ok:
            failed.append(files[0])
            print(run.stdout[-4000:], run.stderr[-4000:], flush=True)

    print(f'Unicode {pins["unicode_version"]}: BidiTest.txt {counts["BidiTest"]} tests; '
          f'BidiCharacterTest.txt {counts["BidiCharacterTest"]} tests; {len(failed)} failed')
    if failed or counts['BidiTest'] != BIDI_TESTS or counts['BidiCharacterTest'] != BIDI_CHARACTER_TESTS:
        sys.exit(1)


if __name__ == '__main__':
    main()
