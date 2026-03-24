#!/bin/bash
#
# Run static analysis tools on dasmxx source code
#
# Usage: ./run_static_analysis.sh [options]
#
# Options:
#   -v, --verbose     Verbose output
#   -o, --output DIR  Output directory for reports (default: ./analysis_reports)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$PROJECT_ROOT/src"
OUTPUT_DIR="$SCRIPT_DIR/analysis_reports"
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -h|--help)
            head -n 10 "$0" | tail -n +2 | sed 's/^#//'
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Create output directory
mkdir -p "$OUTPUT_DIR"

log() {
    if [ "$VERBOSE" = true ]; then
        echo "$@"
    fi
}

echo "==================================================================="
echo "Running Static Analysis on dasmxx"
echo "==================================================================="
echo "Project root: $PROJECT_ROOT"
echo "Source directory: $SRC_DIR"
echo "Output directory: $OUTPUT_DIR"
echo

# Check if tools are available
TOOLS_AVAILABLE=()
TOOLS_MISSING=()

check_tool() {
    local tool=$1
    if command -v "$tool" &> /dev/null; then
        TOOLS_AVAILABLE+=("$tool")
        log "✓ $tool is available"
        return 0
    else
        TOOLS_MISSING+=("$tool")
        log "✗ $tool is not installed"
        return 1
    fi
}

echo "Checking for analysis tools..."
check_tool cppcheck
check_tool clang-tidy
check_tool valgrind
check_tool gcc
check_tool clang

echo
if [ ${#TOOLS_MISSING[@]} -gt 0 ]; then
    echo "Warning: Some tools are not installed: ${TOOLS_MISSING[*]}"
    echo "Install them for complete analysis:"
    echo "  sudo apt-get install cppcheck clang-tidy valgrind"
    echo
fi

cd "$SRC_DIR"

#-------------------------------------------------------------------
# 1. cppcheck - Static analysis for C/C++
#-------------------------------------------------------------------

if check_tool cppcheck &> /dev/null; then
    echo "-------------------------------------------------------------------"
    echo "Running cppcheck..."
    echo "-------------------------------------------------------------------"

    cppcheck \
        --enable=all \
        --inconclusive \
        --std=c99 \
        --suppress=missingIncludeSystem \
        --template='{file}:{line}: {severity}: {message} [{id}]' \
        --output-file="$OUTPUT_DIR/cppcheck.txt" \
        *.c 2>&1 | tee "$OUTPUT_DIR/cppcheck_stderr.txt"

    echo "Results saved to: $OUTPUT_DIR/cppcheck.txt"

    # Summary
    if [ -f "$OUTPUT_DIR/cppcheck.txt" ]; then
        ERROR_COUNT=$(grep -c "error:" "$OUTPUT_DIR/cppcheck.txt" || true)
        WARNING_COUNT=$(grep -c "warning:" "$OUTPUT_DIR/cppcheck.txt" || true)
        STYLE_COUNT=$(grep -c "style:" "$OUTPUT_DIR/cppcheck.txt" || true)

        echo "  Errors: $ERROR_COUNT"
        echo "  Warnings: $WARNING_COUNT"
        echo "  Style issues: $STYLE_COUNT"
    fi
    echo
fi

#-------------------------------------------------------------------
# 2. clang-tidy - Clang-based linter
#-------------------------------------------------------------------

if check_tool clang-tidy &> /dev/null; then
    echo "-------------------------------------------------------------------"
    echo "Running clang-tidy..."
    echo "-------------------------------------------------------------------"

    # Create compile_commands.json for clang-tidy
    # For now, run on individual files

    for file in dasmxx.c xref.c optab.c txt2bin.c decode*.c; do
        if [ -f "$file" ]; then
            log "Analyzing $file..."
            clang-tidy "$file" -- -I. 2>&1 | grep -v "^[0-9]* warnings generated" || true
        fi
    done > "$OUTPUT_DIR/clang-tidy.txt"

    echo "Results saved to: $OUTPUT_DIR/clang-tidy.txt"
    echo
fi

#-------------------------------------------------------------------
# 3. Compiler warnings - gcc with strict warnings
#-------------------------------------------------------------------

echo "-------------------------------------------------------------------"
echo "Building with strict compiler warnings (GCC)..."
echo "-------------------------------------------------------------------"

if check_tool gcc &> /dev/null; then
    # Clean first
    make clean > /dev/null 2>&1 || true

    # Build with maximum warnings
    CFLAGS="-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wformat-security \
            -Wnull-dereference -Wstrict-overflow=2 -Warray-bounds=2 \
            -Wwrite-strings -Wconversion -Wdouble-promotion \
            -Werror=implicit-function-declaration -Werror=return-type \
            -g -O2" \
    make 2>&1 | tee "$OUTPUT_DIR/gcc_warnings.txt"

    WARNING_COUNT=$(grep -c "warning:" "$OUTPUT_DIR/gcc_warnings.txt" || true)
    ERROR_COUNT=$(grep -c "error:" "$OUTPUT_DIR/gcc_warnings.txt" || true)

    echo "  Errors: $ERROR_COUNT"
    echo "  Warnings: $WARNING_COUNT"
    echo "Results saved to: $OUTPUT_DIR/gcc_warnings.txt"
    echo
fi

#-------------------------------------------------------------------
# 4. Address Sanitizer build
#-------------------------------------------------------------------

echo "-------------------------------------------------------------------"
echo "Building with Address Sanitizer..."
echo "-------------------------------------------------------------------"

if check_tool gcc &> /dev/null || check_tool clang &> /dev/null; then
    make clean > /dev/null 2>&1 || true

    CFLAGS="-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -g" \
    make 2>&1 | tee "$OUTPUT_DIR/asan_build.txt"

    if [ $? -eq 0 ]; then
        echo "✓ Address Sanitizer build successful"
        echo "  Instrumented binaries can be tested for memory errors"
    else
        echo "✗ Address Sanitizer build failed"
    fi
    echo
fi

#-------------------------------------------------------------------
# 5. Valgrind - Memory checker (on test runs)
#-------------------------------------------------------------------

if check_tool valgrind &> /dev/null; then
    echo "-------------------------------------------------------------------"
    echo "Running Valgrind on txt2bin..."
    echo "-------------------------------------------------------------------"

    # Rebuild without sanitizers for valgrind
    make clean > /dev/null 2>&1 || true
    make txt2bin > /dev/null 2>&1

    # Run valgrind on txt2bin
    cd "$PROJECT_ROOT"
    if [ -f "test/dasmz80/test.txt" ]; then
        valgrind \
            --leak-check=full \
            --show-leak-kinds=all \
            --track-origins=yes \
            --verbose \
            --log-file="$OUTPUT_DIR/valgrind_txt2bin.txt" \
            ./src/txt2bin test/dasmz80/test.txt /tmp/test.bin 2>&1

        echo "Results saved to: $OUTPUT_DIR/valgrind_txt2bin.txt"

        # Check for errors
        if grep -q "ERROR SUMMARY: 0 errors" "$OUTPUT_DIR/valgrind_txt2bin.txt"; then
            echo "✓ No memory errors found"
        else
            ERROR_COUNT=$(grep "ERROR SUMMARY:" "$OUTPUT_DIR/valgrind_txt2bin.txt" | head -1)
            echo "✗ Memory errors found: $ERROR_COUNT"
        fi
    fi
    echo
fi

#-------------------------------------------------------------------
# Summary
#-------------------------------------------------------------------

echo "==================================================================="
echo "Static Analysis Complete"
echo "==================================================================="
echo "Reports saved in: $OUTPUT_DIR"
echo
echo "Files generated:"
ls -1 "$OUTPUT_DIR"
echo
echo "Next steps:"
echo "  1. Review reports in $OUTPUT_DIR/"
echo "  2. Prioritize critical issues (errors, memory leaks, security)"
echo "  3. Fix issues and document in BUGS.md"
echo "  4. Re-run analysis to verify fixes"
echo

# Create BUGS.md if it doesn't exist
BUGS_FILE="$PROJECT_ROOT/BUGS.md"
if [ ! -f "$BUGS_FILE" ]; then
    cat > "$BUGS_FILE" << 'EOF'
# Known Bugs and Issues

This file tracks bugs found during code review and static analysis.

## Status Legend
- 🔴 CRITICAL - Security issue or crash
- 🟡 HIGH - Major bug affecting functionality
- 🟢 MEDIUM - Minor bug or quality issue
- 🔵 LOW - Style or documentation issue

## Issues

### Fixed Issues
- ✅ src/txt2bin.c:93 - Missing return statement before EXIT_FAILURE (CRITICAL)

### Open Issues

(Add issues here as they are discovered)

EOF
    echo "Created $BUGS_FILE"
fi
