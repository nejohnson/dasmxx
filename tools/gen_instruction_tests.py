#!/usr/bin/env python3
"""
Instruction test generator for dasmxx disassemblers.

Generates instruction inputs and a specification reference from YAML files.
The checked-in test_all.expected files are reviewed disassembler snapshots
and are maintained separately from this generator.

Usage:
    ./gen_instruction_tests.py <processor> [options]

Examples:
    ./gen_instruction_tests.py z80
    ./gen_instruction_tests.py avr --output test/dasmavr/
    ./gen_instruction_tests.py 6502 --spec instruction_specs/6502.yaml
"""

import sys
import argparse
import yaml
from pathlib import Path
from typing import List, Dict, Any, Optional, Tuple
from dataclasses import dataclass, field
import re


@dataclass
class Operand:
    """Represents an instruction operand."""
    type: str  # register, immediate, address, etc.
    value: Any
    display: str  # How it should be displayed in disassembly


@dataclass
class InstructionVariant:
    """
    Represents a specific variant of an instruction.

    For example, "LD B,C" is one variant of the LD instruction.
    """
    mnemonic: str
    operands: str  # String representation (e.g., "B,C", "A,#$42")
    opcode: List[int]  # Byte sequence for this instruction
    reference: Optional[str] = None  # Reference to datasheet page
    category: Optional[str] = None  # Instruction category
    description: Optional[str] = None
    cycles: Optional[int] = None  # Execution cycles
    flags: Optional[str] = None  # Flags affected


@dataclass
class InstructionTest:
    """Represents a test case for an instruction."""
    address: int
    bytes: List[int]
    expected_mnemonic: str
    expected_operands: str
    comment: Optional[str] = None
    reference: Optional[str] = None


@dataclass
class TestSuite:
    """Collection of instruction tests."""
    processor: str
    tests: List[InstructionTest] = field(default_factory=list)
    categories: Dict[str, List[InstructionTest]] = field(default_factory=dict)


class InstructionSpecParser:
    """Parses YAML instruction specifications."""

    def __init__(self, spec_file: Path):
        self.spec_file = spec_file
        self.processor = None
        self.instructions = []

    def parse(self) -> Tuple[str, List[InstructionVariant]]:
        """
        Parse the specification file.

        Returns:
            (processor_name, list_of_instruction_variants)
        """
        with open(self.spec_file, 'r') as f:
            spec = yaml.safe_load(f)

        self.processor = spec.get('processor', 'unknown')
        reference = spec.get('reference', {})

        variants = []
        for inst_def in spec.get('instructions', []):
            mnemonic = inst_def.get('mnemonic')
            category = inst_def.get('category')

            for variant in inst_def.get('variants', []):
                v = InstructionVariant(
                    mnemonic=variant.get('mnemonic', mnemonic),
                    operands=variant.get('operands', ''),
                    opcode=variant.get('opcode', []),
                    reference=variant.get('reference'),
                    category=category,
                    description=variant.get('description'),
                    cycles=variant.get('cycles'),
                    flags=variant.get('flags')
                )
                variants.append(v)

        return self.processor, variants


class TestGenerator:
    """Generates test cases from instruction specifications."""

    def __init__(self, processor: str, variants: List[InstructionVariant]):
        self.processor = processor
        self.variants = variants
        self.suite = TestSuite(processor=processor)
        self.current_address = 0

    def generate_tests(self) -> TestSuite:
        """Generate test cases for all instruction variants."""
        # Organize by category first
        by_category = {}
        for variant in self.variants:
            category = variant.category or 'Other'
            if category not in by_category:
                by_category[category] = []
            by_category[category].append(variant)

        # Generate tests for each category
        for category, variants in sorted(by_category.items()):
            category_tests = []
            for variant in variants:
                test = self._create_test(variant)
                self.suite.tests.append(test)
                category_tests.append(test)

            self.suite.categories[category] = category_tests

        return self.suite

    def _create_test(self, variant: InstructionVariant) -> InstructionTest:
        """Create a test case for a single instruction variant."""
        address = self.current_address

        # Some specifications put operands in both fields. Use the mnemonic's
        # embedded operands once, and reject conflicting definitions.
        parts = variant.mnemonic.strip().split(None, 1)
        mnemonic = parts[0]
        embedded_operands = parts[1] if len(parts) == 2 else ''
        operands = variant.operands.strip()
        if embedded_operands:
            if operands and re.sub(r'\s+', '', operands).upper() != re.sub(r'\s+', '', embedded_operands).upper():
                raise ValueError(f"Conflicting operands for {variant.mnemonic}: {operands}")
            operands = embedded_operands

        comment = None
        if variant.reference:
            comment = f"Reference: {variant.reference}"
        if variant.category:
            comment = f"{variant.category}" + (f" - {comment}" if comment else "")

        test = InstructionTest(
            address=address,
            bytes=variant.opcode,
            expected_mnemonic=mnemonic,
            expected_operands=operands,
            comment=comment,
            reference=variant.reference
        )

        self.current_address += len(variant.opcode)
        return test

    def generate_binary(self, output_file: Path):
        """Generate binary file with all test instructions."""
        with open(output_file, 'wb') as f:
            for test in self.suite.tests:
                f.write(bytes(test.bytes))

    def generate_txt_source(self, output_file: Path):
        """Generate txt2bin source file."""
        with open(output_file, 'w') as f:
            f.write(f"# Generated instruction tests for {self.processor.upper()}\n")
            f.write(f"# Total instructions: {len(self.suite.tests)}\n\n")

            current_category = None
            for test in self.suite.tests:
                # Category header
                if test.comment and ' - ' in test.comment:
                    category = test.comment.split(' - ')[0]
                    if category != current_category:
                        f.write(f"\n# {category}\n")
                        current_category = category

                # Comment with instruction
                display = f"{test.expected_mnemonic}"
                if test.expected_operands:
                    display += f" {test.expected_operands}"
                f.write(f"# {display:30s}")
                if test.reference:
                    f.write(f" ({test.reference})")
                f.write("\n")

                # Bytes
                for byte in test.bytes:
                    f.write(f"{byte:02X}\n")

    def generate_spec_reference(self, output_file: Path):
        """
        Record the instructions described by the specification.

        This is not a disassembler output file and is not used as a golden.
        """
        with open(output_file, 'w') as f:
            f.write(f"# Specification reference for {self.processor.upper()} instruction tests\n")
            if self.processor == 'z80':
                f.write("# YAML intent; make test verifies the decoded listing against these cases.\n")
            else:
                f.write("# This records YAML intent; make test compares test_all.expected instead.\n")
            f.write(f"# Total instructions: {len(self.suite.tests)}\n")

            current_category = None
            for test in self.suite.tests:
                if test.comment and ' - ' in test.comment:
                    category = test.comment.split(' - ')[0]
                    if category != current_category:
                        f.write(f"\n# {category}\n")
                        current_category = category

                bytes_str = ' '.join(f"{b:02X}" for b in test.bytes)
                line = f"{test.address:04X}  {bytes_str:<20s}  {test.expected_mnemonic}"
                if test.expected_operands:
                    line += f" {test.expected_operands}"
                if test.reference:
                    line += f"  # {test.reference}"
                f.write(line.rstrip() + "\n")

    def generate_command_file(self, output_file: Path, bin_file: str):
        """Generate dasmxx command file."""
        ext = f"d{self.processor}"
        with open(output_file, 'w') as f:
            f.write(f"# Generated command file for {self.processor.upper()} instruction tests\n")
            f.write(f"f{bin_file}\n")
            f.write(f"c0000\n")
            f.write(f"e{self.current_address:04X}\n")

    def generate_readme(self, output_file: Path, reference: Dict[str, Any]):
        """Generate README documenting the test suite."""
        with open(output_file, 'w') as f:
            f.write(f"# {self.processor.upper()} Instruction Tests\n\n")
            f.write(f"Comprehensive instruction set tests generated from specifications.\n\n")

            f.write(f"## Reference Documentation\n\n")
            if reference:
                f.write(f"- **Document**: {reference.get('title', 'N/A')}\n")
                f.write(f"- **Version**: {reference.get('version', 'N/A')}\n")
                f.write(f"- **URL**: {reference.get('url', 'N/A')}\n\n")

            f.write(f"## Test Statistics\n\n")
            f.write(f"- Total instructions: {len(self.suite.tests)}\n")
            f.write(f"- Categories: {len(self.suite.categories)}\n\n")

            f.write(f"### Instructions by Category\n\n")
            for category, tests in sorted(self.suite.categories.items()):
                f.write(f"- {category}: {len(tests)} instructions\n")

            f.write(f"\n## Files\n\n")
            f.write(f"- `test_all.txt` - txt2bin source with all instructions\n")
            f.write(f"- `test_all.bin` - Binary test file\n")
            f.write(f"- `test_all.d{self.processor}` - Command file\n")
            if self.processor == 'z80':
                f.write("- `test_all.reference` - YAML intent checked by `make test`\n")
            else:
                f.write("- `test_all.reference` - Generated YAML intent (not a test oracle)\n")
            f.write("- `test_all.expected` - Reviewed disassembler output used by `make test`\n\n")

            f.write(f"## Running Tests\n\n")
            f.write(f"```bash\n")
            f.write("# Build inputs, run the disassembler, and compare its output\n")
            f.write("make test\n")
            f.write(f"```\n\n")

            f.write("To accept an intentional output change, run `make golden` and review the diff.\n")
            if self.processor == 'z80':
                f.write("`make test` also checks the YAML opcodes and instruction meanings.\n")
                f.write("It treats an implicit accumulator and resolved relative targets as\n")
                f.write("equivalent notation.\n\n")
            else:
                f.write("The YAML reference may differ from current decoder behavior; reconcile those\n")
                f.write("differences before treating it as an independent correctness check.\n\n")

            f.write(f"## Regenerating Tests\n\n")
            f.write(f"```bash\n")
            f.write(f"cd ../../../tools\n")
            f.write(f"./gen_instruction_tests.py {self.processor}\n")
            f.write(f"```\n")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Generate instruction tests from YAML specifications'
    )
    parser.add_argument(
        'processor',
        help='Processor name (e.g., z80, avr, 6502)'
    )
    parser.add_argument(
        '-s', '--spec',
        type=Path,
        help='Path to specification YAML file (default: instruction_specs/<processor>.yaml)'
    )
    parser.add_argument(
        '-o', '--output',
        type=Path,
        help='Output directory (default: ../test/dasm<processor>/generated/)'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose output'
    )

    args = parser.parse_args()

    # Determine paths
    script_dir = Path(__file__).parent
    spec_name = 'stm8' if args.processor == 'm8' else args.processor
    spec_file = args.spec or script_dir / 'instruction_specs' / f'{spec_name}.yaml'
    output_dir = args.output or script_dir.parent / 'test' / f'dasm{args.processor}' / 'generated'

    if not spec_file.exists():
        print(f"Error: Specification file not found: {spec_file}", file=sys.stderr)
        print(f"\nCreate a specification file at {spec_file}", file=sys.stderr)
        return 1

    # Parse specification
    if args.verbose:
        print(f"Parsing specification: {spec_file}")

    parser = InstructionSpecParser(spec_file)
    processor, variants = parser.parse()

    if args.verbose:
        print(f"Loaded {len(variants)} instruction variants for {processor.upper()}")

    # Generate tests
    generator = TestGenerator(processor, variants)
    suite = generator.generate_tests()

    if args.verbose:
        print(f"Generated {len(suite.tests)} test cases")
        print(f"Categories: {', '.join(sorted(suite.categories.keys()))}")

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)

    # Generate output files
    if args.verbose:
        print(f"\nGenerating files in {output_dir}/")

    # Binary file
    bin_file = output_dir / 'test_all.bin'
    generator.generate_binary(bin_file)
    if args.verbose:
        print(f"  - {bin_file.name}")

    # txt2bin source
    txt_file = output_dir / 'test_all.txt'
    generator.generate_txt_source(txt_file)
    if args.verbose:
        print(f"  - {txt_file.name}")

    # Specification reference; do not overwrite the reviewed regression oracle.
    reference_file = output_dir / 'test_all.reference'
    generator.generate_spec_reference(reference_file)
    if args.verbose:
        print(f"  - {reference_file.name}")

    # Command file
    cmd_file = output_dir / f'test_all.d{processor}'
    generator.generate_command_file(cmd_file, 'test_all.bin')
    if args.verbose:
        print(f"  - {cmd_file.name}")

    # README
    readme_file = output_dir / 'README.md'
    # Load reference from spec
    with open(spec_file, 'r') as f:
        spec_data = yaml.safe_load(f)
    reference = spec_data.get('reference', {})
    generator.generate_readme(readme_file, reference)
    if args.verbose:
        print(f"  - {readme_file.name}")

    print(f"\nSuccessfully generated {len(suite.tests)} instruction tests for {processor.upper()}")
    print(f"Output directory: {output_dir}")

    return 0


if __name__ == '__main__':
    sys.exit(main())
