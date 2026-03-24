#!/usr/bin/env python3
"""
Test case definitions for dasmxx test framework.

Defines TestCase and TestSuite classes for organizing and running tests.
"""

from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Dict, Any
from enum import Enum


class VerificationMode(Enum):
    """Mode for output verification."""
    EXACT = "exact"              # Exact match required
    IGNORE_WHITESPACE = "ignore_ws"  # Ignore whitespace differences
    IGNORE_ADDRESSES = "ignore_addr"  # Ignore address values (for relocatable code)
    REGEX_PATTERNS = "regex"     # Match against regex patterns
    CUSTOM = "custom"            # Use custom verification function


@dataclass
class TestCase:
    """
    Represents a single test case for a disassembler.

    Attributes:
        name: Human-readable test name
        processor: Processor name (e.g., "z80", "avr", "6502")
        command_file: Path to .dXX command file
        output_file: Path where disassembler output will be written
        golden_file: Optional path to golden/expected output file
        expected_patterns: Optional list of regex patterns to check in output
        flags: Optional list of command-line flags to pass to disassembler
        verification_mode: How to verify the output
        description: Optional test description
        metadata: Optional metadata for the test
    """
    name: str
    processor: str
    command_file: Path
    output_file: Path
    golden_file: Optional[Path] = None
    expected_patterns: Optional[List[str]] = None
    flags: List[str] = field(default_factory=list)
    verification_mode: VerificationMode = VerificationMode.EXACT
    description: Optional[str] = None
    metadata: Dict[str, Any] = field(default_factory=dict)

    def __post_init__(self):
        """Validate test case configuration."""
        if not self.command_file.exists():
            raise ValueError(f"Command file not found: {self.command_file}")

        if not self.golden_file and not self.expected_patterns:
            # Warning: no verification specified
            pass


@dataclass
class TestSuite:
    """
    Represents a collection of related test cases.

    Attributes:
        name: Human-readable suite name
        tests: List of test cases in this suite
        description: Optional suite description
        metadata: Optional metadata for the suite
    """
    name: str
    tests: List[TestCase]
    description: Optional[str] = None
    metadata: Dict[str, Any] = field(default_factory=dict)

    def add_test(self, test: TestCase):
        """Add a test to this suite."""
        self.tests.append(test)

    def filter_by_processor(self, processor: str) -> 'TestSuite':
        """Create a new suite with only tests for the specified processor."""
        filtered_tests = [t for t in self.tests if t.processor == processor]
        return TestSuite(
            name=f"{self.name} ({processor})",
            tests=filtered_tests,
            description=self.description,
            metadata=self.metadata
        )


class TestSuiteBuilder:
    """Helper class for building test suites."""

    def __init__(self, name: str, base_dir: Path):
        """
        Initialize suite builder.

        Args:
            name: Suite name
            base_dir: Base directory for resolving relative paths
        """
        self.name = name
        self.base_dir = base_dir
        self.tests: List[TestCase] = []

    def add_test(self,
                 name: str,
                 processor: str,
                 command_file: str,
                 golden_file: Optional[str] = None,
                 expected_patterns: Optional[List[str]] = None,
                 flags: Optional[List[str]] = None,
                 verification_mode: VerificationMode = VerificationMode.EXACT,
                 description: Optional[str] = None) -> 'TestSuiteBuilder':
        """
        Add a test to the suite.

        Paths are resolved relative to base_dir.
        Output file is automatically generated in output/ subdirectory.

        Returns self for chaining.
        """
        cmd_path = self.base_dir / command_file
        output_path = self.base_dir / "output" / f"{Path(command_file).stem}.out"
        golden_path = self.base_dir / golden_file if golden_file else None

        test = TestCase(
            name=name,
            processor=processor,
            command_file=cmd_path,
            output_file=output_path,
            golden_file=golden_path,
            expected_patterns=expected_patterns,
            flags=flags or [],
            verification_mode=verification_mode,
            description=description
        )

        self.tests.append(test)
        return self

    def add_processor_test(self,
                          processor: str,
                          test_name: str,
                          flags: Optional[List[str]] = None,
                          verification_mode: VerificationMode = VerificationMode.EXACT) -> 'TestSuiteBuilder':
        """
        Add a standard processor test that follows naming convention.

        Expects:
        - Command file: <processor>/<test_name>.d<processor>
        - Golden file: <processor>/golden/<test_name>.golden

        Returns self for chaining.
        """
        proc_lower = processor.lower()
        cmd_file = f"{proc_lower}/{test_name}.d{proc_lower}"
        golden = f"{proc_lower}/golden/{test_name}.golden"

        return self.add_test(
            name=f"{processor.upper()} - {test_name}",
            processor=proc_lower,
            command_file=cmd_file,
            golden_file=golden,
            flags=flags,
            verification_mode=verification_mode
        )

    def build(self, description: Optional[str] = None) -> TestSuite:
        """Build and return the test suite."""
        return TestSuite(
            name=self.name,
            tests=self.tests,
            description=description
        )


def create_tool_feature_suite(test_dir: Path) -> TestSuite:
    """
    Create a test suite for tool features (command codes).

    Tests command codes like c, p, l, b, w, s, etc. across multiple processors.
    """
    builder = TestSuiteBuilder("Tool Features", test_dir / "tool_features")

    # These tests will be created in Phase 1
    # For now, just define the structure

    return builder.build(
        description="Tests for dasmxx command codes and output formatting features"
    )


def create_instruction_suite(test_dir: Path, processor: str) -> TestSuite:
    """
    Create a test suite for processor instruction set testing.

    Tests all instructions for a specific processor.
    """
    proc_test_dir = test_dir / f"dasm{processor}"
    builder = TestSuiteBuilder(f"{processor.upper()} Instructions", proc_test_dir)

    # These tests will be auto-generated in Phase 2
    # For now, include existing tests if they exist

    # Check for existing test file
    existing_test = proc_test_dir / f"test.d{processor}"
    if existing_test.exists():
        golden = proc_test_dir / "golden" / "test.golden"
        builder.add_test(
            name=f"{processor.upper()} - comprehensive",
            processor=processor,
            command_file=f"test.d{processor}",
            golden_file=f"golden/test.golden" if golden.exists() else None,
            verification_mode=VerificationMode.EXACT
        )

    return builder.build(
        description=f"Comprehensive instruction set tests for {processor.upper()}"
    )


def discover_processor_tests(test_dir: Path) -> List[TestSuite]:
    """
    Discover all processor test directories and create test suites.

    Looks for directories matching test/dasm* pattern.
    """
    suites = []

    for proc_dir in test_dir.glob("dasm*"):
        if not proc_dir.is_dir():
            continue

        processor = proc_dir.name.replace("dasm", "")
        suite = create_instruction_suite(test_dir, processor)

        if suite.tests:  # Only add if tests exist
            suites.append(suite)

    return suites
