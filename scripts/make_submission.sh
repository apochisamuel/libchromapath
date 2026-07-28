#!/bin/bash -eu
root=$(cd "$(dirname "$0")/.." && pwd)
out=${1:-"$root/../libchromapath-submission.zip"}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/libchromapath"
rsync -a --exclude '.git' --exclude 'internal' --exclude '*.o' --exclude '*.a' \
  --exclude '*.zip' --exclude 'chromapath-inspect' --exclude 'test-basic' \
  "$root/" "$tmp/libchromapath/"
python3 "$tmp/libchromapath/scripts/audit_submission.py" "$tmp/libchromapath"
(cd "$tmp" && zip -qr "$out" libchromapath)
printf 'wrote %s\n' "$out"
