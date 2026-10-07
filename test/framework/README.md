# Python test framework

`test_runner.py` runs Python suites that return a `TestSuite`. The current suite
is `test/tool_features/test_suite.py` (19 tool feature cases). Processor tests,
including the generated Z80 and STM8 cases, run through their Makefiles and the
aggregate `make -C test test` target.

## Run

From the repository root:

```bash
make -C test/tool_features all
python3 test/framework/test_runner.py -v
python3 test/framework/test_runner.py test/tool_features/test_suite.py
python3 test/framework/test_runner.py -o /tmp/dasmxx-results.json
```

With no path, the runner discovers `test/*/test_suite.py`. Each file must export
`create_suite()` returning a nonempty `TestSuite`. Missing or invalid suites,
missing expectation files, and test cases without an expectation fail the run.
`-o` writes a JSON summary.

## Define a suite

```python
from pathlib import Path
from test_cases import TestSuiteBuilder


def create_suite():
    base_dir = Path(__file__).parent
    builder = TestSuiteBuilder("Example", base_dir)
    builder.add_test(
        name="Basic disassembly",
        processor="z80",
        command_file="test.dz80",
        golden_file="test.expected",
    )
    return builder.build()
```

`TestSuiteBuilder` resolves paths from `base_dir` and writes actual output under
`base_dir/output/`. A case needs either a golden file for comparison or
`expected_patterns` to search for in the output. The runner offers exact,
whitespace-insensitive, and address-insensitive golden comparisons through
`VerificationMode`. The disassembler executable must already exist in `src/`.

To accept intentional changes to golden files, run with `--update-golden`, then
review their diffs before committing. That option replaces configured golden
files with the current output, so use it only after checking the change.
