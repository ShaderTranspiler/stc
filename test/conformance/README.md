# Conformance tests

Conformance tests are meant to validate the output of the transpiler, not only speaking of the produced GLSL, but also the reported error messages for invalid cases.

Each file in subdirectories starting from here (`test/conformance`) represents a separate invocation of the transpiler. Those files in turn can specify test cases in annotations, placed in comments. If the filename begins with the prefix `valid`, it marks a test file that is expected to pass transpilation and pass `glslangValidator`. Other files are expected to fail. Specific test cases, like errors on certain lines, output substrings, etc. can be specified.

These tests are checked through `run_conformance_test.py`, which is run for each file. Testing should be possible via both the `test` target in CMake and `ctest` directly. They're also run in CI. The suite is currently expected to pass only in lenient mode, since cascading errors are still very much a thing. See the python script's help message for all available options. A large comment above its `parse_annotation` function documents the current syntax of annotations allowed in test files, and the possible test cases one can introduce with them.

Known bugs should be marked with annotations that result in the failing check being skipped (ideally as locally as possible), so that passing tests remain valid behavior, skipped tests document known bugs, deviations and missing features, while failing tests keep indicating newly introduced bugs.

## AI usage

Conformance tests were generated partially by AI (Claude Opus 5.5). I initially handwrote ~150 test files, which served as the base style guide for Claude. It was also given access to the transpiler's source code, my half-done thesis text (which already included description about the supported language subset, the main use case and other useful context info) and we iterated and recorded a list of undecided cases. I then went through these and made decisions case by case, so that the transpiler's intended behavior is never guessed at by AI. It then generated the remaining files, and made some minor changes to previously handwritten ones. These were all reviewed and modified if needed by [szgerii](https://github.com/szgerii) before they were included in the test suite.

Test files that were created by AI are marked with a comment stating so in the very first line.
