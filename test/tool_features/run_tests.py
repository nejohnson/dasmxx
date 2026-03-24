#!/usr/bin/env python3
"""
Test suite for dasmxx tool features.

Tests command codes and output formatting features that work across all processors.
"""

import sys
from pathlib import Path

# Add framework to path
sys.path.insert(0, str(Path(__file__).parent.parent / "framework"))

from test_cases import TestSuiteBuilder, VerificationMode
from test_runner import TestRunner


def create_tool_feature_tests():
    """Create comprehensive tool feature test suite."""
    base_dir = Path(__file__).parent
    builder = TestSuiteBuilder("Tool Features", base_dir)

    # Code command tests
    builder.add_test(
        name="Basic code disassembly",
        processor="z80",
        command_file="code_commands/test_basic_code.dz80",
        golden_file="golden/test_basic_code.golden",
        description="Test basic 'c' command for code disassembly"
    )

    builder.add_test(
        name="Procedure labels",
        processor="z80",
        command_file="code_commands/test_procedures.dz80",
        golden_file="golden/test_procedures.golden",
        description="Test 'p' command for procedure labeling"
    )

    builder.add_test(
        name="Manual labels",
        processor="z80",
        command_file="code_commands/test_labels.dz80",
        golden_file="golden/test_labels.golden",
        description="Test 'l' command for manual label assignment"
    )

    builder.add_test(
        name="Auto-generated labels",
        processor="z80",
        command_file="code_commands/test_auto_labels.dz80",
        golden_file="golden/test_auto_labels.golden",
        description="Test automatic label generation (PROC_nnnn format)"
    )

    # Data dump tests
    builder.add_test(
        name="Byte dump",
        processor="z80",
        command_file="data_dumps/test_byte_dump.dz80",
        golden_file="golden/test_byte_dump.golden",
        description="Test 'b' command for byte dumps"
    )

    builder.add_test(
        name="Word dump",
        processor="z80",
        command_file="data_dumps/test_word_dump.dz80",
        golden_file="golden/test_word_dump.golden",
        description="Test 'w' command for word dumps"
    )

    builder.add_test(
        name="String dump",
        processor="z80",
        command_file="data_dumps/test_string_dump.dz80",
        golden_file="golden/test_string_dump.golden",
        description="Test 's' command for string dumps"
    )

    builder.add_test(
        name="UTF-16 string dump",
        processor="z80",
        command_file="data_dumps/test_utf16_dump.dz80",
        golden_file="golden/test_utf16_dump.golden",
        description="Test 'u' command for UTF-16 string dumps"
    )

    builder.add_test(
        name="Mixed code and data",
        processor="z80",
        command_file="data_dumps/test_mixed.dz80",
        golden_file="golden/test_mixed.golden",
        description="Test mixed code disassembly and data dumps"
    )

    # Output mode tests
    builder.add_test(
        name="Cross-reference output",
        processor="z80",
        command_file="code_commands/test_labels.dz80",
        golden_file="golden/test_xref.golden",
        flags=["-x"],
        description="Test -x flag for cross-reference output"
    )

    builder.add_test(
        name="Assembler output",
        processor="z80",
        command_file="code_commands/test_labels.dz80",
        golden_file="golden/test_assembler.golden",
        flags=["-a"],
        description="Test -a flag for assembler-ready output"
    )

    builder.add_test(
        name="Stripped output",
        processor="z80",
        command_file="code_commands/test_labels.dz80",
        golden_file="golden/test_stripped.golden",
        flags=["-s"],
        description="Test -s flag for stripped output"
    )

    return builder.build()


def main():
    """Run the tool feature test suite."""
    import argparse

    parser = argparse.ArgumentParser(description="Run tool feature tests")
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Enable verbose output')
    parser.add_argument('-u', '--update-golden', action='store_true',
                       help='Update golden files')
    args = parser.parse_args()

    # Create test suite
    suite = create_tool_feature_tests()

    # Run tests
    runner = TestRunner(verbose=args.verbose, update_golden=args.update_golden)
    runner.run_suite(suite)

    # Print summary
    success = runner.print_summary()

    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
