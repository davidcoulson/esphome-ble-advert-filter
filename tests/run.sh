#!/usr/bin/env bash
# Host tests: the real ble_advert_filter.cpp compiled against tests/stubs/, under
# AddressSanitizer and UBSan. Needs only a C++17 compiler. Run from anywhere.
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d)
build() {  # build <output> <test source> [extra component sources...]
  local out=$1 test=$2
  shift 2
  ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer \
    -Itests/stubs \
    components/ble_advert_filter/ble_advert_filter.cpp \
    tests/stubs/aes.cpp \
    "$@" "$test" -o "$out"
}

# The filter itself.
build "$dir/test_filter" tests/test_filter.cpp
"$dir/test_filter"

# The sensor and number platforms and the actions.
echo
build "$dir/test_platforms" tests/test_platforms.cpp \
  components/ble_advert_filter/sensor/ble_advert_filter_sensor.cpp \
  components/ble_advert_filter/number/rssi_threshold_number.cpp
"$dir/test_platforms"
