#!/usr/bin/env python3
"""Check a generated instruction listing against the independent YAML intent."""

import argparse
import re
from pathlib import Path

from gen_instruction_tests import InstructionSpecParser, TestGenerator


LISTING_LINE = re.compile(
    r"^\s+([0-9A-F]{4,6}):\s+((?:[0-9A-F]{2} ?)+?)\s{2,}(.+)$"
)


def normalize(text):
    return re.sub(r"\s+", "", text).upper()


def expected_z80(test):
    mnemonic = test.expected_mnemonic.upper()
    operands = test.expected_operands
    operands = re.sub(
        r"\$\+(\d+)",
        lambda match: f"${test.address + int(match.group(1)):04X}",
        operands,
    )
    return normalize(f"{mnemonic} {operands}")


def decoded_z80(text):
    decoded = normalize(text)
    for mnemonic in ("SUB", "AND", "OR", "XOR", "CP"):
        prefix = mnemonic + "A,"
        if decoded.startswith(prefix):
            return mnemonic + decoded[len(prefix):]
    return decoded


def verify_z80(spec_path, binary_path, listing_path):
    processor, variants = InstructionSpecParser(spec_path).parse()
    suite = TestGenerator(processor, variants).generate_tests()
    expected_binary = b"".join(bytes(test.bytes) for test in suite.tests)
    errors = []
    if binary_path.read_bytes() != expected_binary:
        errors.append("binary input differs from YAML opcodes")

    listing = []
    for line in listing_path.read_text().splitlines():
        match = LISTING_LINE.match(line)
        if match:
            listing.append((int(match.group(1), 16), bytes.fromhex(match.group(2)), match.group(3)))
    if len(listing) != len(suite.tests):
        errors.append(f"decoded {len(listing)} instructions; expected {len(suite.tests)}")

    for index, (test, actual) in enumerate(zip(suite.tests, listing), 1):
        address, displayed_bytes, decoded_text = actual
        if address != test.address:
            errors.append(f"case {index}: address {address:04X}, expected {test.address:04X}")
        if displayed_bytes != bytes(test.bytes):
            errors.append(f"case {index}: displayed bytes {displayed_bytes.hex(' ')} differ from YAML {bytes(test.bytes).hex(' ')}")
        if decoded_z80(decoded_text) != expected_z80(test):
            errors.append(f"case {index} at {test.address:04X}: {decoded_text.strip()} != {test.expected_mnemonic} {test.expected_operands}")
    return suite, errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("processor", choices=["z80"])
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    suite_dir = root / "test" / f"dasm{args.processor}" / "generated"
    spec = root / "tools" / "instruction_specs" / f"{args.processor}.yaml"
    suite, errors = verify_z80(spec, suite_dir / "test_all.bin", suite_dir / "output" / "test_all.out")
    if errors:
        for error in errors:
            print(error)
        return 1
    print(f"Verified {len(suite.tests)} {args.processor.upper()} cases against YAML intent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
