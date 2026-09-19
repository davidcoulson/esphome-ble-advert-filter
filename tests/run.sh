#!/usr/bin/env bash
# Host tests: the real ble_advert_filter.cpp compiled against tests/stubs/, under
# AddressSanitizer and UBSan. Needs only a C++17 compiler. Run from anywhere.
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d)/test_filter
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer \
  -Itests/stubs \
  components/ble_advert_filter/ble_advert_filter.cpp \
  tests/stubs/aes.cpp \
  tests/test_filter.cpp \
  -o "$out"
"$out"
