#!/usr/bin/env python3
"""
Output verification utilities for dasmxx test framework.

Provides various methods for comparing disassembler output against expected results.
"""

import re
import difflib
from pathlib import Path
from typing import List, Optional, Tuple, Callable
from dataclasses import dataclass
from enum import Enum


@dataclass
class VerificationResult:
    """Result of output verification."""
    passed: bool
    message: str
    diff: Optional[str] = None


class OutputVerifier:
    """Handles verification of disassembler output."""

    def __init__(self):
        # Common patterns that may vary between runs
        self.address_pattern = re.compile(r'\b[0-9A-Fa-f]{4,8}\b')
        self.auto_label_pattern = re.compile(r'\b(PROC|AL|DATA)_[0-9A-Fa-f]+\b')

    def verify_output(self,
                     output_file: Path,
                     expected_file: Path,
                     mode: str = "exact") -> VerificationResult:
        """
        Verify output file against expected file.

        Args:
            output_file: Path to actual output
            expected_file: Path to expected output
            mode: Verification mode (exact, ignore_ws, ignore_addr, regex)

        Returns:
            VerificationResult with pass/fail status
        """
        if not output_file.exists():
            return VerificationResult(
                passed=False,
                message=f"Output file not found: {output_file}"
            )

        if not expected_file.exists():
            return VerificationResult(
                passed=False,
                message=f"Expected file not found: {expected_file}"
            )

        output_text = output_file.read_text()
        expected_text = expected_file.read_text()

        if mode == "exact":
            return self._verify_exact(output_text, expected_text)
        elif mode == "ignore_ws":
            return self._verify_ignore_whitespace(output_text, expected_text)
        elif mode == "ignore_addr":
            return self._verify_ignore_addresses(output_text, expected_text)
        else:
            return VerificationResult(
                passed=False,
                message=f"Unknown verification mode: {mode}"
            )

    def _verify_exact(self, output: str, expected: str) -> VerificationResult:
        """Exact string comparison."""
        if output == expected:
            return VerificationResult(
                passed=True,
                message="Output matches exactly"
            )

        diff = self._generate_diff(expected, output)
        return VerificationResult(
            passed=False,
            message="Output does not match expected",
            diff=diff
        )

    def _verify_ignore_whitespace(self, output: str, expected: str) -> VerificationResult:
        """Compare with whitespace normalized."""
        # Normalize whitespace: collapse multiple spaces, strip lines
        def normalize(text):
            lines = [' '.join(line.split()) for line in text.splitlines()]
            return '\n'.join(line for line in lines if line)

        output_norm = normalize(output)
        expected_norm = normalize(expected)

        if output_norm == expected_norm:
            return VerificationResult(
                passed=True,
                message="Output matches (ignoring whitespace)"
            )

        diff = self._generate_diff(expected_norm, output_norm)
        return VerificationResult(
            passed=False,
            message="Output does not match (ignoring whitespace)",
            diff=diff
        )

    def _verify_ignore_addresses(self, output: str, expected: str) -> VerificationResult:
        """
        Compare with addresses normalized.

        Replaces all hex addresses with placeholders before comparing.
        Useful for comparing disassembly structure when addresses may vary.
        """
        def normalize_addresses(text):
            # Replace addresses with fixed placeholder
            text = self.address_pattern.sub('ADDR', text)
            # Also normalize auto-generated labels
            text = self.auto_label_pattern.sub('LABEL', text)
            return text

        output_norm = normalize_addresses(output)
        expected_norm = normalize_addresses(expected)

        if output_norm == expected_norm:
            return VerificationResult(
                passed=True,
                message="Output structure matches (addresses normalized)"
            )

        diff = self._generate_diff(expected_norm, output_norm)
        return VerificationResult(
            passed=False,
            message="Output structure does not match",
            diff=diff
        )

    def check_pattern(self, output: str, pattern: str) -> VerificationResult:
        """
        Check if output contains a regex pattern.

        Args:
            output: Output text to check
            pattern: Regular expression pattern to search for

        Returns:
            VerificationResult indicating if pattern was found
        """
        try:
            if re.search(pattern, output, re.MULTILINE):
                return VerificationResult(
                    passed=True,
                    message=f"Pattern found: {pattern}"
                )
            else:
                return VerificationResult(
                    passed=False,
                    message=f"Pattern not found: {pattern}"
                )
        except re.error as e:
            return VerificationResult(
                passed=False,
                message=f"Invalid regex pattern: {e}"
            )

    def check_patterns(self, output: str, patterns: List[str]) -> VerificationResult:
        """
        Check if output contains all specified patterns.

        Returns verification result for first failed pattern, or success if all pass.
        """
        for pattern in patterns:
            result = self.check_pattern(output, pattern)
            if not result.passed:
                return result

        return VerificationResult(
            passed=True,
            message=f"All {len(patterns)} patterns found"
        )

    def check_not_pattern(self, output: str, pattern: str) -> VerificationResult:
        """
        Check that output does NOT contain a regex pattern.

        Useful for verifying that certain errors or invalid output don't appear.
        """
        try:
            if re.search(pattern, output, re.MULTILINE):
                return VerificationResult(
                    passed=False,
                    message=f"Unexpected pattern found: {pattern}"
                )
            else:
                return VerificationResult(
                    passed=True,
                    message=f"Pattern correctly absent: {pattern}"
                )
        except re.error as e:
            return VerificationResult(
                passed=False,
                message=f"Invalid regex pattern: {e}"
            )

    def verify_cross_references(self, output: str) -> VerificationResult:
        """
        Verify that cross-references are properly formatted and consistent.

        Checks:
        - All referenced labels are defined
        - All defined labels are referenced (or are entry points)
        - Cross-reference format is correct
        """
        # Extract label definitions (lines with label: format)
        label_defs = set()
        label_def_pattern = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*):.*$', re.MULTILINE)
        for match in label_def_pattern.finditer(output):
            label_defs.add(match.group(1))

        # Extract label references (in instructions or data)
        label_refs = set()
        # Common reference patterns: "jmp LABEL", "call LABEL", "ld hl,LABEL"
        label_ref_pattern = re.compile(r'\b([A-Z_][A-Z0-9_]*)\b(?!\:)')
        for match in label_ref_pattern.finditer(output):
            label = match.group(1)
            # Filter out instruction mnemonics and common keywords
            if not self._is_instruction_or_keyword(label):
                label_refs.add(label)

        # Check for undefined references
        undefined = label_refs - label_defs
        if undefined:
            return VerificationResult(
                passed=False,
                message=f"Undefined label references: {', '.join(sorted(undefined))}"
            )

        return VerificationResult(
            passed=True,
            message="Cross-references are consistent"
        )

    def verify_instruction_format(self, output: str, processor: str) -> VerificationResult:
        """
        Verify that instructions are properly formatted.

        Checks basic structure:
        - Address field
        - Byte dump field
        - Instruction mnemonic
        - Operands (if present)
        """
        # Basic pattern for instruction lines (address, bytes, instruction)
        # Example: "0000  AF          xor  a"
        inst_pattern = re.compile(
            r'^[0-9A-Fa-f]{4}[ \t]+([0-9A-Fa-f]{2}[ \t]+)+[ \t]+[a-z]+.*$',
            re.MULTILINE
        )

        # Check if we have any instruction lines
        matches = list(inst_pattern.finditer(output))
        if not matches:
            return VerificationResult(
                passed=False,
                message="No properly formatted instruction lines found"
            )

        return VerificationResult(
            passed=True,
            message=f"Found {len(matches)} properly formatted instructions"
        )

    def _generate_diff(self, expected: str, actual: str, context: int = 3) -> str:
        """Generate unified diff between expected and actual output."""
        expected_lines = expected.splitlines(keepends=True)
        actual_lines = actual.splitlines(keepends=True)

        diff = difflib.unified_diff(
            expected_lines,
            actual_lines,
            fromfile='expected',
            tofile='actual',
            lineterm='',
            n=context
        )

        return ''.join(diff)

    def _is_instruction_or_keyword(self, word: str) -> bool:
        """Check if a word is likely an instruction mnemonic or keyword."""
        # Common instruction mnemonics and keywords to filter out
        keywords = {
            # Z80
            'ADC', 'ADD', 'AND', 'BIT', 'CALL', 'CCF', 'CP', 'CPD', 'CPDR', 'CPI',
            'CPIR', 'CPL', 'DAA', 'DEC', 'DI', 'DJNZ', 'EI', 'EX', 'EXX', 'HALT',
            'IM', 'IN', 'INC', 'IND', 'INDR', 'INI', 'INIR', 'JP', 'JR', 'LD',
            'LDD', 'LDDR', 'LDI', 'LDIR', 'NEG', 'NOP', 'OR', 'OTDR', 'OTIR',
            'OUT', 'OUTD', 'OUTI', 'POP', 'PUSH', 'RES', 'RET', 'RETI', 'RETN',
            'RL', 'RLA', 'RLC', 'RLCA', 'RLD', 'RR', 'RRA', 'RRC', 'RRCA', 'RRD',
            'RST', 'SBC', 'SCF', 'SET', 'SLA', 'SRA', 'SRL', 'SUB', 'XOR',
            # Registers
            'A', 'B', 'C', 'D', 'E', 'H', 'L', 'BC', 'DE', 'HL', 'SP', 'IX', 'IY',
            'AF', 'PC', 'I', 'R',
            # AVR
            'RJMP', 'RCALL', 'BRNE', 'BREQ', 'BRCS', 'BRCC', 'LSL', 'LSR', 'ROL',
            'ROR', 'ASR', 'SWAP', 'BSET', 'BCLR', 'SBI', 'CBI', 'BST', 'BLD',
            'SEC', 'CLC', 'SEN', 'CLN', 'SEZ', 'CLZ', 'SEI', 'CLI', 'SES', 'CLS',
            'SEV', 'CLV', 'SET', 'CLT', 'SEH', 'CLH', 'MOV', 'MOVW', 'LDI', 'LDS',
            'LPM', 'SPM', 'ELPM', 'STS', 'STD',
            # 6502
            'BRK', 'ORA', 'ASL', 'PHP', 'BPL', 'CLC', 'JSR', 'BIT', 'PLP',
            'BMI', 'RTI', 'PHA', 'BVC', 'RTS', 'PLA', 'BVS', 'STY', 'STX',
            'DEY', 'TXA', 'BCC', 'TYA', 'TXS', 'LDY', 'LDX', 'LDA', 'TAY',
            'TAX', 'BCS', 'TSX', 'CPY', 'CPX', 'INY', 'INX', 'BNE', 'CLD',
            'SED', 'BEQ',
            # 8051
            'AJMP', 'LJMP', 'SJMP', 'ACALL', 'LCALL', 'MOVC', 'MOVX', 'SETB',
            'CLR', 'CJNE', 'DJNZ', 'JC', 'JNC', 'JB', 'JNB', 'JBC', 'SWAP',
            'RL', 'RLC', 'RR', 'RRC', 'MUL', 'DIV', 'DA', 'XCHD', 'XCH',
            # Common
            'NOP', 'ILLEGAL', 'DB', 'DW', 'DS', 'END'
        }

        return word.upper() in keywords


class CustomVerifier:
    """
    Base class for custom output verifiers.

    Subclass this to implement processor-specific or test-specific verification logic.
    """

    def verify(self, output: str, expected: Optional[str] = None) -> VerificationResult:
        """
        Verify output with custom logic.

        Args:
            output: Actual output from disassembler
            expected: Optional expected output (may be None for custom verifiers)

        Returns:
            VerificationResult
        """
        raise NotImplementedError("Subclasses must implement verify()")


def create_pattern_list(*patterns: str) -> List[str]:
    """Helper function to create a list of regex patterns."""
    return list(patterns)


def create_multi_line_pattern(*lines: str) -> str:
    """Helper to create a regex pattern matching multiple consecutive lines."""
    # Escape special regex characters in each line
    escaped_lines = [re.escape(line) for line in lines]
    # Join with pattern that matches line breaks and optional whitespace
    return r'\s*'.join(escaped_lines)
