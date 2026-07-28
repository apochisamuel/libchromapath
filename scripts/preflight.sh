#!/bin/bash -eu
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
make clean
CC=${CC:-clang} CXX=${CXX:-clang++} make test
for fuzzer in fuzz/*_fuzzer.cc; do
    ${CXX:-clang++} -std=c++17 -Wall -Wextra -Wpedantic -Isrc -c "$fuzzer" -o /tmp/"$(basename "${fuzzer%.cc}")".o
done
python3 - <<'PY'
from pathlib import Path
root = Path('.')
for path in [Path('build.sh'), Path('.clusterfuzzlite/build.sh')]:
    text = path.read_text()
    required = [
        'cd $SRC', '$CC $CFLAGS -Isrc -c', '$CXX $CXXFLAGS -Isrc -o',
        '$LIB_FUZZING_ENGINE'
    ]
    missing = [item for item in required if item not in text]
    if missing:
        raise SystemExit(f'{path}: missing required build fragments: {missing}')
project = Path('.clusterfuzzlite/project.yaml').read_text().strip().splitlines()
if project != ['language: c']:
    raise SystemExit('project.yaml must contain only language: c')
for fuzzer in Path('fuzz').glob('*_fuzzer.cc'):
    text = fuzzer.read_text()
    gate = 'if (size < 64 || size > 16*1024*1024) return 0;'
    if gate not in text:
        raise SystemExit(f'{fuzzer}: input-size gate is missing or changed')
    corpus = Path('fuzz/corpus') / fuzzer.stem
    if not corpus.is_dir() or not any(corpus.iterdir()):
        raise SystemExit(f'{fuzzer}: seed corpus is missing')
print('structural checks passed')
PY

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/libchromapath"
cp -a . "$tmp/libchromapath/source"
rm -rf "$tmp/libchromapath/source/internal"
find "$tmp/libchromapath/source" -type f \( -name '*.o' -o -name '*.a' -o -name 'test-basic' -o -name 'chromapath-inspect' \) -delete
python3 scripts/audit_submission.py "$tmp/libchromapath/source"
