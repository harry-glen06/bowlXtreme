#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

host_arch="$(uname -m)"
case "$host_arch" in
  arm64|x86_64)
    ;;
  *)
    echo "Unsupported CPU architecture: $host_arch"
    exit 1
    ;;
esac

can_run_here() {
  local bin="$1"
  local info
  info="$(file -b "$bin" 2>/dev/null || true)"
  if [[ -z "$info" ]]; then
    return 1
  fi
  if [[ "$info" == *"universal binary"* ]]; then
    return 0
  fi
  if [[ "$host_arch" == "arm64" && "$info" == *"arm64"* ]]; then
    return 0
  fi
  if [[ "$host_arch" == "x86_64" && "$info" == *"x86_64"* ]]; then
    return 0
  fi
  return 1
}

candidate_bins=(
  "bowling"
  "bowling-arm64"
  "bowling-x86_64"
  "bowling-mac-arm64"
  "bowling-mac-x86_64"
  "build/bowling"
  "build/bowling-arm64"
  "build/bowling-x86_64"
)

for bin in "${candidate_bins[@]}"; do
  if [[ -x "$bin" ]] && can_run_here "$bin"; then
    exec "./$bin"
  fi
done

echo "No compatible bowling binary found for this Mac ($host_arch)."
echo "Detected binaries:"
found_any=0
for bin in "${candidate_bins[@]}"; do
  if [[ -f "$bin" ]]; then
    found_any=1
    echo "  $bin -> $(file -b "$bin")"
  fi
done
if [[ "$found_any" -eq 0 ]]; then
  echo "  (none)"
fi

echo
echo "Build a binary for $host_arch (or universal), then run this launcher again."
exit 1

