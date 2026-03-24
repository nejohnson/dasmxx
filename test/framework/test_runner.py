#!/usr/bin/env python3
"""
Test runner for dasmxx disassembler suite.

This script orchestrates running tests for the dasmxx disassemblers,
including both tool feature tests and per-CPU instruction tests.
"""

import sys
import os
import subprocess
import argparse
import json
from pathlib import Path
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass

# Add framework directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from test_cases import TestCase, TestSuite
from verify_output import OutputVerifier, VerificationResult


@dataclass
class TestResult:
    """Result of running a single test."""
    test_name: str
    passed: bool
    message: str
    output_file: Optional[Path] = None
    expected_file: Optional[Path] = None
    diff: Optional[str] = None


class TestRunner:
    """Main test runner for dasmxx test suites."""

    def __init__(self, verbose: bool = False, update_golden: bool = False):
        self.verbose = verbose
        self.update_golden = update_golden
        self.verifier = OutputVerifier()
        self.results: List[TestResult] = []

        # Find project root (parent of test directory)
        self.test_dir = Path(__file__).parent.parent
        self.project_root = self.test_dir.parent
        self.src_dir = self.project_root / "src"

    def log(self, message: str, force: bool = False):
        """Print message if verbose mode is enabled."""
        if self.verbose or force:
            print(message)

    def find_disassembler(self, processor: str) -> Optional[Path]:
        """Find the disassembler executable for a given processor."""
        dasm_name = f"dasm{processor}"
        dasm_path = self.src_dir / dasm_name

        if not dasm_path.exists():
            self.log(f"Warning: Disassembler {dasm_name} not found at {dasm_path}")
            return None

        return dasm_path

    def run_disassembler(self,
                        processor: str,
                        command_file: Path,
                        output_file: Path,
                        flags: List[str] = None) -> Tuple[bool, str]:
        """
        Run a disassembler with the given command file and flags.

        Returns (success, error_message)
        """
        dasm_path = self.find_disassembler(processor)
        if not dasm_path:
            return False, f"Disassembler not found for {processor}"

        # Build command
        cmd = [str(dasm_path)]
        if flags:
            cmd.extend(flags)
        cmd.append(str(command_file))

        self.log(f"Running: {' '.join(cmd)}")

        try:
            # Run disassembler and capture output
            result = subprocess.run(
                cmd,
                cwd=command_file.parent,
                capture_output=True,
                text=True,
                timeout=30
            )

            # Write stdout to output file
            output_file.write_text(result.stdout)

            if result.returncode != 0:
                return False, f"Disassembler failed with code {result.returncode}: {result.stderr}"

            return True, ""

        except subprocess.TimeoutExpired:
            return False, "Disassembler timed out after 30 seconds"
        except Exception as e:
            return False, f"Failed to run disassembler: {e}"

    def run_test(self, test: TestCase) -> TestResult:
        """Run a single test case."""
        self.log(f"\nRunning test: {test.name}")

        # Ensure output directory exists
        test.output_file.parent.mkdir(parents=True, exist_ok=True)

        # Run the disassembler
        success, error = self.run_disassembler(
            test.processor,
            test.command_file,
            test.output_file,
            test.flags
        )

        if not success:
            return TestResult(
                test_name=test.name,
                passed=False,
                message=error
            )

        # Verify output
        if test.golden_file and test.golden_file.exists():
            # Compare against golden file
            verification = self.verifier.verify_output(
                test.output_file,
                test.golden_file,
                test.verification_mode.value if hasattr(test.verification_mode, 'value') else test.verification_mode
            )

            if not verification.passed:
                return TestResult(
                    test_name=test.name,
                    passed=False,
                    message=f"Output verification failed: {verification.message}",
                    output_file=test.output_file,
                    expected_file=test.golden_file,
                    diff=verification.diff
                )
        elif test.expected_patterns:
            # Check for expected patterns
            output_text = test.output_file.read_text()
            for pattern in test.expected_patterns:
                verification = self.verifier.check_pattern(output_text, pattern)
                if not verification.passed:
                    return TestResult(
                        test_name=test.name,
                        passed=False,
                        message=f"Pattern check failed: {verification.message}",
                        output_file=test.output_file
                    )
        else:
            # No verification specified - just check it ran
            self.log(f"Warning: No verification for test {test.name}")

        # Update golden file if requested
        if self.update_golden and test.golden_file:
            test.golden_file.parent.mkdir(parents=True, exist_ok=True)
            import shutil
            shutil.copy(test.output_file, test.golden_file)
            self.log(f"Updated golden file: {test.golden_file}")

        return TestResult(
            test_name=test.name,
            passed=True,
            message="Test passed",
            output_file=test.output_file,
            expected_file=test.golden_file
        )

    def run_suite(self, suite: TestSuite) -> List[TestResult]:
        """Run all tests in a test suite."""
        self.log(f"\n{'='*60}", force=True)
        self.log(f"Running test suite: {suite.name}", force=True)
        self.log(f"{'='*60}", force=True)

        suite_results = []
        for test in suite.tests:
            result = self.run_test(test)
            suite_results.append(result)
            self.results.append(result)

            # Print immediate feedback
            status = "PASS" if result.passed else "FAIL"
            print(f"  [{status}] {result.test_name}")
            if not result.passed and self.verbose:
                print(f"        {result.message}")

        return suite_results

    def print_summary(self):
        """Print summary of all test results."""
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed

        print(f"\n{'='*60}")
        print(f"Test Summary")
        print(f"{'='*60}")
        print(f"Total:  {total}")
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")

        if failed > 0:
            print(f"\nFailed tests:")
            for result in self.results:
                if not result.passed:
                    print(f"  - {result.test_name}")
                    print(f"    {result.message}")
                    if result.diff and self.verbose:
                        print(f"    Diff:")
                        for line in result.diff.split('\n')[:20]:  # Limit diff output
                            print(f"      {line}")

        return failed == 0

    def save_results(self, output_file: Path):
        """Save test results to JSON file."""
        results_data = {
            'total': len(self.results),
            'passed': sum(1 for r in self.results if r.passed),
            'failed': sum(1 for r in self.results if not r.passed),
            'tests': [
                {
                    'name': r.test_name,
                    'passed': r.passed,
                    'message': r.message,
                    'output_file': str(r.output_file) if r.output_file else None,
                    'expected_file': str(r.expected_file) if r.expected_file else None
                }
                for r in self.results
            ]
        }

        with open(output_file, 'w') as f:
            json.dump(results_data, f, indent=2)

        self.log(f"Results saved to {output_file}")


def main():
    """Main entry point for test runner."""
    parser = argparse.ArgumentParser(
        description='Run tests for dasmxx disassembler suite'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose output'
    )
    parser.add_argument(
        '-u', '--update-golden',
        action='store_true',
        help='Update golden files with current output'
    )
    parser.add_argument(
        '-o', '--output',
        type=Path,
        help='Save test results to JSON file'
    )
    parser.add_argument(
        'suites',
        nargs='*',
        help='Test suite files to run (default: run all)'
    )

    args = parser.parse_args()

    runner = TestRunner(verbose=args.verbose, update_golden=args.update_golden)

    # If no suites specified, find all test suites
    if not args.suites:
        # Look for test suite definitions
        test_dir = Path(__file__).parent.parent
        suite_files = list(test_dir.glob('*/test_suite.py'))
        suite_files.extend(test_dir.glob('test_*/test_suite.py'))
        args.suites = suite_files

    if not args.suites:
        print("No test suites found")
        return 1

    # Run each suite
    for suite_file in args.suites:
        suite_path = Path(suite_file)
        if not suite_path.exists():
            print(f"Warning: Suite file not found: {suite_path}")
            continue

        # Import and run the suite
        # For now, we'll use a simple approach
        # TODO: Implement dynamic loading of test suites
        print(f"Note: Dynamic suite loading not yet implemented")
        print(f"      Please define suites in test code")

    # Print summary
    success = runner.print_summary()

    # Save results if requested
    if args.output:
        runner.save_results(args.output)

    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
