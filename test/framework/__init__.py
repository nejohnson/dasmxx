"""
dasmxx test framework package.

Provides automated testing infrastructure for the dasmxx disassembler suite.
"""

from .test_cases import (
    TestCase,
    TestSuite,
    TestSuiteBuilder,
    VerificationMode,
    create_tool_feature_suite,
    create_instruction_suite,
    discover_processor_tests
)

from .verify_output import (
    OutputVerifier,
    VerificationResult,
    CustomVerifier,
    create_pattern_list,
    create_multi_line_pattern
)

from .test_runner import TestRunner, TestResult

__all__ = [
    # Test cases
    'TestCase',
    'TestSuite',
    'TestSuiteBuilder',
    'VerificationMode',
    'create_tool_feature_suite',
    'create_instruction_suite',
    'discover_processor_tests',
    # Verification
    'OutputVerifier',
    'VerificationResult',
    'CustomVerifier',
    'create_pattern_list',
    'create_multi_line_pattern',
    # Test runner
    'TestRunner',
    'TestResult',
]
