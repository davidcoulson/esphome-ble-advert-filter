#!/usr/bin/env python3
"""Fail if the filter logic changed without the tests being revisited.

tests/test_ibeacon_match.cpp cannot include ble_advert_filter.cpp - that pulls in
the whole ESPHome and ESP-IDF world - so it transcribes two pieces of logic:
ibeacon_match_ and the limit-resolution block. A transcription silently drifting
from the component is worse than no test, because it passes while the firmware
misbehaves.

This hashes those two regions of the source and compares them against the
digests recorded below. Changing either one fails the check until a human has
re-read the transcription and updated the digest deliberately.

    python3 tools/check_test_sync.py            # verify
    python3 tools/check_test_sync.py --update   # accept the current source
"""

import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "components" / "ble_advert_filter" / "ble_advert_filter.cpp"
SELF = Path(__file__)

# region name -> (start marker, end marker), both matched literally
REGIONS = {
    "ibeacon_match_": (
        "bool BLEAdvertFilter::ibeacon_match_(",
        "bool BLEAdvertFilter::payload_blocked_(",
    ),
    "limit_resolution": (
        "  int8_t limit;\n  const char *limit_name;",
        "  // 5. Exclusive mode",
    ),
}

EXPECTED = {
    "ibeacon_match_": "a6a6425d1ba68ba3",
    "limit_resolution": "521e5a3d2c995877",
}


def digest(name: str, text: str) -> str:
    start, end = REGIONS[name]
    i = text.find(start)
    if i < 0:
        raise SystemExit(f"check_test_sync: start marker for {name!r} not found in {SRC.name}")
    j = text.find(end, i)
    if j < 0:
        raise SystemExit(f"check_test_sync: end marker for {name!r} not found in {SRC.name}")
    body = text[i:j]
    # Comments and blank lines are not behaviour; reflowing them should not
    # trip the guard.
    body = re.sub(r"//[^\n]*", "", body)
    body = re.sub(r"\s+", " ", body).strip()
    return hashlib.sha256(body.encode()).hexdigest()[:16]


def main() -> int:
    text = SRC.read_text()
    actual = {name: digest(name, text) for name in REGIONS}

    if "--update" in sys.argv:
        me = SELF.read_text()
        for name, d in actual.items():
            me = re.sub(rf'("{name}": ")[0-9a-f]+|PLACEHOLDER"', "", me, count=0) if False else me
            me = re.sub(rf'("{name}": ")[^"]*(")', rf"\g<1>{d}\g<2>", me, count=1)
        SELF.write_text(me)
        print("check_test_sync: digests updated ->", ", ".join(f"{k}={v}" for k, v in actual.items()))
        return 0

    bad = [n for n in REGIONS if EXPECTED[n] != actual[n]]
    if not bad:
        print("check_test_sync: filter logic unchanged; tests still describe the source")
        return 0

    print("check_test_sync: FILTER LOGIC CHANGED\n", file=sys.stderr)
    for n in bad:
        print(f"  {n}: recorded {EXPECTED[n]}, now {actual[n]}", file=sys.stderr)
    print(
        "\ntests/test_ibeacon_match.cpp transcribes this logic and may now be\n"
        "testing something the component no longer does. Re-read the test\n"
        "against the source, then run:  python3 tools/check_test_sync.py --update",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
