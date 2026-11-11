#!/usr/bin/env python3
"""Evaluate a single conformance test file.

Parses a conformance test file, runs it via the transpiler CLI while collecting
its error reports and then evaluates them against the annotations placed in the
comments of the test file.
See the comment above the parse_annotation function for annotation syntax.
"""

import sys
import re
import os
import argparse
import subprocess
import tempfile
import shutil
import logging
import itertools
from typing import override
from pathlib import Path
from enum import Enum

REPORT_RE: re.Pattern[str] = re.compile(r"^\[(?P<file>.*):(?P<line>\d+)\] \[(?P<category>Error|Warning|Internal Error)\] (?P<message>.*)$")

SUMMARY_PREFIX: str = "==>"

SUBPROCESS_TIMEOUT_SEC: int = 15

# codes that fail a test compilation (unknown non-zero error codes result in UNKNOWN)
GLSLANG_VALIDATOR_REJECTED_CODES: list[int] = [2, 3]

logger: logging.Logger = logging.getLogger(__name__)

class ExitCode(Enum):
    TESTS_PASSED = 0 # all tests passed without any being skipped
    TESTS_FAILED = 1 # at least one test failed, and was not marked for skipping
    ARGS_MALFORMED = 2 # baked into argparse, non-modifiable (also used for runner-level errors for the provided arguments)
    TESTS_MALFORMED = 3 # the format of the test file, or its annotations is invalid
    TRANSPILER_ERROR = 4 # encountered an unexpected transpiler-side error
    UNEXPECTED_ERROR = 5 # catch-all for any other internal error of the script
    GLSLANG_VALIDATOR_ERROR = 6 # unknown errors coming from glslangValidator
    TESTS_STALE = 7 # tests have stale annotations, and --fail-on-stale was passed
    TESTS_PASSED_WITH_SKIPS = 77 # tests passed, but some were skipped (e.g. because they were marked missing in the test file)

    def __str__(self) -> str:
        match self:
            case ExitCode.TESTS_PASSED:            return "tests passed"
            case ExitCode.TESTS_FAILED:            return "tests failed"
            case ExitCode.ARGS_MALFORMED:          return "malformed arguments"
            case ExitCode.TESTS_MALFORMED:         return "malformed tests"
            case ExitCode.TRANSPILER_ERROR:        return "transpiler error"
            case ExitCode.UNEXPECTED_ERROR:        return "unexpected error"
            case ExitCode.GLSLANG_VALIDATOR_ERROR: return "glslangValidator error"
            case ExitCode.TESTS_STALE:             return "stale tests"
            case ExitCode.TESTS_PASSED_WITH_SKIPS: return "tests passed with skips"
            case _:                                return "unknown exit code"

class ReportType(Enum):
    WARNING = 0
    ERROR = 1
    INTERNAL_ERROR = 10 # reserved for collected reports, annotations should not allow them

    @staticmethod
    def parse_directive(directive: str) -> 'ReportType | None':
        match directive:
            case "ERROR":          return ReportType.ERROR
            case "WARNING":        return ReportType.WARNING
            case _:                return None

    @override
    def __str__(self):
        match self:
            case ReportType.WARNING:        return "Warning"
            case ReportType.ERROR:          return "Error"
            case ReportType.INTERNAL_ERROR: return "Internal Error"
            case _:                         return "Unknown"
            
class CheckingMode(Enum):
    LENIENT = 0
    STRICT = 1

    @staticmethod
    def parse(mode: str) -> 'CheckingMode':
        match mode:
            case "lenient": return CheckingMode.LENIENT
            case "strict":  return CheckingMode.STRICT
            case _:         raise ValueError(f"Value '{mode}' is not a valid checking mode")
    
    @override
    def __str__(self):
        match self:
            case CheckingMode.LENIENT: return "Lenient"
            case CheckingMode.STRICT:  return "Strict"
            case _:                    return "Unknown"
            
class TestResult(Enum):
    PASSED = 0
    PASSED_WITH_SKIPS = 1
    FAILED = 2
    
    @staticmethod
    def merge(a: 'TestResult', b: 'TestResult') -> 'TestResult':
        if a.value < b.value:
            a, b = b, a
        return a if a in [TestResult.FAILED, TestResult.PASSED_WITH_SKIPS] else TestResult.PASSED

    @override
    def __str__(self):
        match self:
            case TestResult.PASSED:            return "Passed"
            case TestResult.PASSED_WITH_SKIPS: return "Passed with skips"
            case TestResult.FAILED:            return "Failed"
            case _:                            return "Unknown"

class ShaderStage(Enum):
    COMPUTE = 0
    FRAGMENT = 1

    @override
    def __str__(self):
        match self:
            case ShaderStage.COMPUTE:  return "comp"
            case ShaderStage.FRAGMENT: return "frag"
            case _:                    return "unknown"

class ExpectedGLSLResult(Enum):
    # all these only apply to valid_ tests:
    EXPECTED_TO_PASS = 0 # glslangValidator is expected to pass (also used for non-valid test files, where this property is not read at all)
    EXPECTED_TO_FAIL = 1 # glslangValidator is expected to fail because of a transpiler bug
    INTENDED_TO_FAIL = 2 # glslangValidator is intended to fail (rare, some transpiler flags allow it to produce potentially invalid GLSL code deliberately)

    def expected_to_fail(self):
        return self != ExpectedGLSLResult.EXPECTED_TO_PASS

    @override
    def __str__(self):
        match self:
            case ExpectedGLSLResult.EXPECTED_TO_PASS: return "expected to pass"
            case ExpectedGLSLResult.EXPECTED_TO_FAIL: return "expected to fail"
            case ExpectedGLSLResult.INTENDED_TO_FAIL: return "intended to fail"
            case _:                                   return "unknown"

class Report:
    def __init__(self, report_type: ReportType, line_number: int, message: str):
        self.report_type: ReportType = report_type
        self.line_number: int = line_number
        self.message = message

    @override
    def __str__(self):
        return f"{self.report_type} report for line {self.line_number} with message '{self.message}'"

class ExpectedReport(Report):
    def __init__(self, report_type: ReportType, line_number: int, message: str | None, expected_missing: bool, internal: bool):
        assert report_type != ReportType.INTERNAL_ERROR, "internal error type not allowed in an expected report"
        assert not internal or report_type == ReportType.ERROR, "internal modifier not allowed on non-error reports"
        Report.__init__(self, report_type, line_number, "")
        self.message = message
        self.missing: bool = expected_missing
        self.internal: bool = internal

    @override
    def __str__(self):
        return f"Expected {"internal " if self.internal else ""}{self.report_type} report for line {self.line_number}{f" with message pattern '{self.message}'" if self.message is not None else ""}{", expected to fail" if self.missing else ""}"

class ExpectedOutput:
    def __init__(self, pattern: str | re.Pattern[str], expected_missing: bool):
        self.pattern: str | re.Pattern[str] = pattern
        self.missing: bool = expected_missing

    def pattern_str(self) -> str:
        return f"matching '{self.pattern.pattern}'" if isinstance(self.pattern, re.Pattern) else f"containing '...{self.pattern}...'"

    @override
    def __str__(self):
        return f"Expected output {self.pattern_str()}{", expected to fail" if self.missing else ""}"
        
class AnnotationContext:
    def __init__(self):
        self.parsed_shader_stage_directive: bool = False
        self.parsed_glsl_directive: bool = False
        self.parsed_stc_directive: bool = False

class TestContext:
    def __init__(self, path: str):
        self.path: str = path
        self.line_count: int = 0
        self.expected_reports: list[ExpectedReport] = []
        self.expected_outputs: list[ExpectedOutput] = []
        self.shader_stage: ShaderStage = ShaderStage.COMPUTE
        self.out_path: str = ""
        self.expected_glsl_result: ExpectedGLSLResult = ExpectedGLSLResult.EXPECTED_TO_PASS
        self.glsl_failure_pattern: str = "" # only applies when expected_glsl_result != ExpectedGLSLResult.EXPECTED_TO_PASS
        self.expect_transpiler_failure: bool = False
        self.transpiler_failure_pattern: str = "" # only applies when expect_transpiler_failure == True
        self.expect_transpiler_internal_error: bool = False # only applies when expect_transpiler_failure == True
        self.transpiler_failure_skipped: bool = False # set only if expect_transpiler_failure == True and transpiler_failure_pattern matched the error output
        self.extra_cli_flags: list[str] = []
        self.annotation_context: AnnotationContext = AnnotationContext()
        self.pre_eval_block: tuple[int, int] = (0, 0) # (start line, end line), if either component is zero ==> no valid pre eval block in file
        self.stc_success: bool = False
        self.skip_all: bool = False
        self.has_stale: bool = False
    
    def should_be_valid(self):
        return Path(self.path).name.startswith("valid")

    def in_pre_eval_block(self) -> bool:
        return self.pre_eval_block[0] != 0 and self.pre_eval_block[1] == 0
        
    def has_pre_eval_block(self) -> bool:
        return self.pre_eval_block[0] != 0 and self.pre_eval_block[1] != 0

class Config:
    def __init__(self, path: str, cli: str, mode: CheckingMode, skip_message_checks: bool, tmp_dir_path: Path,
                 fail_on_stale: bool, julia_bin_dir: str | None, glslang_validator: str | None):
        self.path: str = path
        self.cli: str = cli
        self.mode: CheckingMode = mode
        self.skip_message_checks: bool = skip_message_checks
        self.tmp_dir_path: Path = tmp_dir_path
        self.fail_on_stale: bool = fail_on_stale
        self.julia_bin_dir: str | None = julia_bin_dir
        self.glslang_validator: str | None = glslang_validator
        
    def strict(self) -> bool:
        return self.mode == CheckingMode.STRICT

# prefix warnings by [WARNING] so that they're grep-able in output
class LogFormatter(logging.Formatter):
    @override
    def format(self, record: logging.LogRecord) -> str:
        message = super().format(record)
        return f"[WARNING] {message}" if record.levelno == logging.WARNING else message

# log [STALE] issues, which usually indicate a temporary annotation no longer applies
def handle_stale(message: str, config: Config, test_context: TestContext) -> None:
    test_context.has_stale = True
    logger.log(logging.ERROR if config.fail_on_stale else logging.WARNING, f"[STALE] {message}")
    
def parse_subproc_output(output: bytes | str | None) -> str:
    if isinstance(output, bytes):
        output = output.decode("utf-8", "replace")

    return output.strip() if isinstance(output, str) else ""

# annotation syntax: <directive>[(<argument1>[, <argument2> [...]])][:<text_argument>]
# <directive> is defined as the substring ranging from the start of the annotation to the first '(' or ':' character
# <text_argument> is defined as the substring ranging from the first colon to the end of the annotation (so extra colons in <text_argument> are allowed)
# <directive>, <argumentN> and <text_argument> are all stripped from leading and trailing whitespace
# supported directives (required arguments are marked with *):
#   ERROR                  register an expected error report for a given line
#       <arguments>:
#           missing        the report is expected to be missing from the output, test will be skipped (can be used to mark known gaps during testing)
#                          it is meant to patch temporary situations, where a certain error not working would turn CI red, even if it's known and fixing was deemed non-priority.
#                          can also be used to track progress on new features, by creating the cases in advance and marking them missing. may not be used together with 'internal'.
#           internal       the error is expected to be raised via an internal error currently, marking a test that hits an expected internal error skipped instead of failing.
#                          similar to missing, it should mark a temporary state, where something is rejected, but not as a user error. may not be used together with 'missing'.
#                          this marks any matching internal errors as skips for the internal error checking pass. other internal errors are still rejected so it is not equivalent
#                          with STC(missing, internal)
#           +<n>           the report should apply to the line <n> lines below whatever line the annotation is on
#           -<n>           the report should apply to the line <n> lines above whatever line the annotation is on
#       <text_argument>:   if provided, acts as a pattern that has to be a substring of the actual report's message for the test to pass (or be skipped if marked 'missing' or 'internal')
#   INTERNAL ERROR         rewritten to ERROR(internal, <arguments>...): <text_argument>
#   WARNING                same as ERROR, for warning reports. may not use the argument 'internal'.
#   OUTPUT                 verify that the transpiled output for the test file (not for the line!) matches some pattern. may only be used in valid files (but allowed repeatedly)
#       <arguments>:
#           missing        specify that the match is currently expected to fail, can be used for temporary skips
#           regex          treat <text_argument> as a regular expression to match (using Python's re) instead of a substring
#           multiline      include re.MULTILINE in the pattern's flags (^ and $ also match line boundaries). only valid if 'regex' is also passed.
#       <text_argument>:   the pattern to match against the generated output (as a substring by default, or a regular expression if 'regex' is passed)
#   RE OUTPUT              alias for OUTPUT(regex), forwards <arguments> and <text_argument>
#   SHADER STAGE           specify the shader stage used with glslangValidator (only affects valid files). must be used at most once per test file.
#       <*arguments>:      must be exactly one of the available options
#           comp           compute shader (default if SHADER STAGE is not specified)
#           frag           fragment shader
#   GLSL                   along with the 'missing' argument, specifies that the shader is currently expected to not pass validation via glslangValidator. must be used at most once per test file.
#       <arguments>:
#           *missing       must be passed with GLSL for readability
#           intended       turns glslangValidator failing expectedly from a skip to a pass. this should only be used for testing flags where the transpiler allows itself to generate potentially
#                          invalid GLSL, like function forwarding.
#       <text_argument>:   if provided, can be used to specify a message pattern that has to appear as a substring of the glslangValidator output for the validation failure to be treated as skipped/passed.
#   STC                    same as GLSL(missing) but for the transpiler. marks that an otherwise valid test file is currently expected to fail transpilation. internal error checks and report validation
#                          is still performed on the transpiler output (since ERROR is not allowed in a valid file, this effectively means WARNING checks).
#       <arguments>:
#           *missing       must be passed with STC for readability
#           internal       specifies that stc is expected to fail with an internal error and that internal error checks should be skipped. unlike ERROR, it can and must be used together with 'missing'.
#   STC FLAG               passes its arguments to the STC CLI as-is, e.g. STC FLAG(--no-coerce-f32, --Wjl-query). more than one can be defined per test file, their flags are accumulated and passed
#                          together to the CLI. argument order is preserved going from top to bottom in file, left to right in arguments.
#       <arguments>:
#           <flag>         any STC CLI compatible argument. these are not checked in the runner, so the result returned in case of invalid flags is undefined.
#                          also ensure that these don't collide with the runner's default flags (e.g. don't try to hijack the output of a valid file).
#                          for managing pre-eval code prefer the PRE EVAL BEGIN and PRE EVAL END approach which keeps the entire code together in one file instead of the --pre-eval flag.
#                          note that a current limitation is colons in arguments (e.g. in a path) not working, since they're treated as the text_argument separator.
#       <text_argument>    can be used for description, explaining intent, etc. the value is simply ignored by the runner
#   PRE EVAL BEGIN         begins the pre-eval block in the code. the pre-eval block will be lifted into a separate file and passed to the test file's transpilation via --pre-eval.
#                          may only be used once per test file, and must have a closing PRE EVAL END later. see the CLI's --pre-eval flag more information.
#                          note that this marks the first line that is considered to be part of the pre-eval block, even if it's at the end of it.
#   PRE EVAL END           marks the end of the pre-eval block. see PRE_EVAL_BEGIN for details. marks the last line, not the exact column position, similarly to PRE EVAL BEGIN.
#   SKIP                   marks the entire test file to be skipped, in every validation stage. should be used for temporary disabling of test cases, not as a general tool.
#                          can be specified at most once per file.
#       <text_argument>    optional, can be used to specify the reason for skipping. ignored by the parser.
#   COMMENT                directive for leaving single-line comments in the test files (multi-line comments are currently ignored, so they don't require this)
#       <*text_argument>:  the body of the comment
#   NOTE: <*text_argument> rewritten to COMMENT: <text_argument>
#   TODO: <*text_argument> rewritten to COMMENT: <text_argument>
DIRECTIVE_LIST = ["ERROR", "INTERNAL ERROR", "WARNING", "OUTPUT", "RE OUTPUT", "SKIP",
                  "SHADER STAGE", "GLSL", "STC", "STC FLAG", "PRE EVAL BEGIN", "PRE EVAL END",
                  "COMMENT", "NOTE", "TODO"]
DIRECTIVE_LIST_LEN_DESC = sorted(DIRECTIVE_LIST, key=len, reverse=True)
ALLOWED_IN_PRE_EVAL_BLOCK = ["PRE EVAL END", "COMMENT", "TODO", "NOTE"]
def parse_annotation(test_context: TestContext, line_number: int, raw_annotation: str) -> bool:
    def log_error(msg: str, hint: str | None = None) -> bool:
        hint_tail = f"\n  Hint: {hint}" if hint is not None else ""
        logger.error(f"Malformed annotation on line {line_number}: '{raw_annotation}'\n  Why: {msg}{hint_tail}")
        return False

    def get_not_a_valid_arg_message(arg: str, directive: str):
        return f"'{arg}' is not a valid argument for the '{directive}' directive"

    SYNTAX_HINT = "Annotation syntax: <directive>[(argument1[, argument2[...]])][: <text_argument>]"
        
    if raw_annotation == "":
        return log_error("Empty annotation", "Single line comments are reserved for annotations in test files.")
    
    colon_pos = raw_annotation.find(":")
    
    open_paren_pos = raw_annotation.find("(", 0, colon_pos if colon_pos != -1 else len(raw_annotation))
    close_paren_pos = raw_annotation.find(")", open_paren_pos) if open_paren_pos != -1 else -1
    if open_paren_pos != -1 and close_paren_pos == -1:
        return log_error("Opening parenthesis for annotation argument list doesn't have a matching closing parenthesis", SYNTAX_HINT)
    
    assert open_paren_pos == close_paren_pos == -1 or open_paren_pos < close_paren_pos
    assert open_paren_pos + 1 < len(raw_annotation)
    
    if close_paren_pos != -1:
        if colon_pos != -1:
            assert close_paren_pos < colon_pos
            maybe_junk = raw_annotation[close_paren_pos + 1:colon_pos].strip()
            if maybe_junk != "":
                return log_error(f"Unexpected text found between argument list and text argument: '{maybe_junk}'", f"Only whitespace is allowed between the argument list closing ')' and text argument opening ':' characters\n{SYNTAX_HINT}")
        elif close_paren_pos + 1 < len(raw_annotation):
            tail = raw_annotation[close_paren_pos + 1:].strip()
            if tail != "":
                return log_error(f"Unexpected text found after argument list: '{tail}'", f"Only whitespace is allowed after the argument list closing ')' character, unless a text argument is also provided.\n{SYNTAX_HINT}")
         
    directive = raw_annotation[:open_paren_pos] if open_paren_pos != -1 else raw_annotation[:colon_pos] if colon_pos != -1 else raw_annotation    
    directive = directive.strip()
    
    args: list[str] = raw_annotation[open_paren_pos + 1:close_paren_pos].split(",") if open_paren_pos != -1 else []
    args = [arg.strip() for arg in args]
    
    text_arg = raw_annotation[colon_pos + 1:] if colon_pos != -1 and colon_pos + 1 < len(raw_annotation) else ""
    text_arg = text_arg.strip()

    if directive == "INTERNAL ERROR":
        directive = "ERROR"
        args.insert(0, "internal")

    if directive == "RE OUTPUT":
        directive = "OUTPUT"
        args.insert(0, "regex")

    if directive in ["TODO", "NOTE"]:
        directive = "COMMENT"

    if test_context.in_pre_eval_block() and directive not in ALLOWED_IN_PRE_EVAL_BLOCK:
        return log_error("Illegal directive inside a pre-eval block",
                         f"Only {", ".join(ALLOWED_IN_PRE_EVAL_BLOCK)} directives are allowed to appear in a pre-eval block.")

    # TODO: deduplicate code
    match directive:
        case "ERROR" | "WARNING":
            report_type = ReportType.parse_directive(directive)
            assert report_type is not None

            if report_type == ReportType.ERROR and test_context.should_be_valid():
                return log_error(f"'{directive}' directive not allowed in files containing the prefix 'valid', since their transpilation should be successful.")
            
            offset = 0
            found_offset = False
            
            missing = False
            internal = False

            for arg in args:
                if (arg.startswith("+") or arg.startswith("-")):
                    if found_offset:
                        logger.warning(f"Multiple offset adjustments for '{directive}' directive on line {line_number}")
                        
                    try:
                        offset += int(arg)
                    except ValueError:
                        return log_error(f"'{arg}' does not name a valid integer offset after {arg[0]}")

                    found_offset = True
                elif arg == "missing":
                    if missing:
                        logger.warning(f"Multiple 'missing' arguments passed to '{directive}' directive on line {line_number}")
                    missing = True
                elif arg == "internal":
                    if report_type != ReportType.ERROR:
                        return log_error(get_not_a_valid_arg_message(arg, directive), "'internal' may only be used with the 'ERROR' and 'STC' directives.")
                    if internal:
                        logger.warning(f"Multiple 'internal' arguments passed to '{directive}' directive on line {line_number}")
                    internal = True
                else:
                    return log_error(get_not_a_valid_arg_message(arg, directive))

            if missing and internal:
                return log_error(f"Report marked 'missing' is also marked 'internal'.",
                                 "'missing' and 'internal' are mutually exclusive arguments, since an internal error should never be desired in the future.")
                
            report_line_number = line_number + offset
            if report_line_number < 1 or report_line_number > test_context.line_count:
                add_offset_str = f"+ {offset}" if offset >= 0 else f"- {abs(offset)}"
                return log_error(f"Offset points outside the valid line number range (which is [1-{test_context.line_count}]) with {line_number} {add_offset_str} = {report_line_number}")
            
            report_message = text_arg if text_arg != "" else None

            assert not internal or report_type == ReportType.ERROR
            test_context.expected_reports.append(ExpectedReport(report_type, report_line_number, report_message, missing, internal))

        case "OUTPUT":
            if not test_context.should_be_valid():
                return log_error("'OUTPUT' in non-valid test file",
                                 "'OUTPUT' directive can only be used in files with name containing the prefix 'valid', since other test cases are expected to not produce any transpilation output.")
            
            if text_arg == "":
                return log_error("'OUTPUT' directive used without text argument",
                                 "The text argument must specify the pattern to check for in the generated output, otherwise the directive makes no sense")

            missing = False
            as_regex = False
            multiline = False
            for arg in args:
                if arg == "missing":
                    if missing:
                        logger.warning("'missing' argument specified more than once for 'OUTPUT' directive")
                    missing = True
                elif arg == "regex":
                    if as_regex:
                        logger.warning("'regex' argument specified more than once for 'OUTPUT' directive")
                    as_regex = True
                elif arg == "multiline":
                    if multiline:
                        logger.warning("'multiline' argument specified more than once for 'OUTPUT' directive")
                    multiline = True
                else:
                    return log_error(get_not_a_valid_arg_message(arg, directive))

            if multiline and not as_regex:
                return log_error("'multiline' argument passed to 'OUTPUT' directive without 'regex' also being passed",
                                 "'multiline' only makes sense when treating the text argument as a regular expression.")

            pattern: str | re.Pattern[str] = text_arg
            if as_regex:
                try:
                    pattern = re.compile(text_arg, flags=re.MULTILINE if multiline else 0)
                except re.PatternError as e:
                    return log_error(f"Invalid regular expression passed to 'OUTPUT' directive:\n{e}", "The pattern has to be compatible with Python's re module")

            test_context.expected_outputs.append(ExpectedOutput(pattern, missing))

        case "SHADER STAGE":
            if test_context.annotation_context.parsed_shader_stage_directive:
                return log_error(f"Multiple annotations with 'SHADER STAGE' directive.", "'SHADER STAGE' must be specified at most once per test file.")
            test_context.annotation_context.parsed_shader_stage_directive = True
            
            if not test_context.should_be_valid():
                logger.warning("'SHADER STAGE' directive will not have any effect in non-valid file.\n" +
                               "  Hint: Files that are expected to transpile without error (and can thus be GLSL validated) should be marked by prefixing their name with 'valid'.")
            
            if text_arg != "":
                return log_error(f"Text argument passed to 'SHADER STAGE' directive.")
            
            if len(args) != 1:
                return log_error(f"Invalid argument count in 'SHADER STAGE' directive.", "'SHADER STAGE' expects exactly one argument, which must always be provided.")

            arg = args[0]
            if arg == "comp":
                test_context.shader_stage = ShaderStage.COMPUTE
            elif arg == "frag":
                test_context.shader_stage = ShaderStage.FRAGMENT
            else:
                return log_error(get_not_a_valid_arg_message(arg, directive), "Supported arguments are 'comp' and 'frag'.")
                
        case "GLSL":
            if test_context.annotation_context.parsed_glsl_directive:
                return log_error(f"Multiple annotations with 'GLSL' directive.", "'GLSL' must be specified at most once per test file.")
            test_context.annotation_context.parsed_glsl_directive = True
            
            if not test_context.should_be_valid():
                logger.warning("'GLSL' directive will not have any effect in non-valid file.\n" +
                               "  Hint: Files that are expected to transpile without error (and can thus be GLSL validated) should be marked by prefixing their name with 'valid'.")

            missing = False
            intended = False
            for arg in args:
                if arg == "missing":
                    if missing:
                        logger.warning("Multiple 'missing' arguments passed to 'GLSL' directive.")
                    missing = True
                elif arg == "intended":
                    if intended:
                        logger.warning("Multiple 'intended' arguments passed to 'GLSL' directive.")
                    intended = True
                else:
                    return log_error(get_not_a_valid_arg_message(arg, directive), "Supported arguments are 'missing' and 'intended'.")

            if not missing:
                return log_error("'GLSL' directive without a 'missing' argument.", "'missing' must be passed to the 'GLSL' directive for readability.")

            test_context.expected_glsl_result = ExpectedGLSLResult.INTENDED_TO_FAIL if intended else ExpectedGLSLResult.EXPECTED_TO_FAIL
            test_context.glsl_failure_pattern = text_arg
            
        case "STC":
            if test_context.annotation_context.parsed_stc_directive:
                return log_error(f"Multiple annotations with 'STC' directive.", "'STC' must be specified at most once per test file.")
            test_context.annotation_context.parsed_stc_directive = True
            
            if not test_context.should_be_valid():
                logger.warning("'STC' directive will not have any effect in non-valid file.\n" +
                               "  Hint: Files that are normally expected to transpile without error should be marked by prefixing their name with 'valid'.")

            internal = False
            missing = False
            for arg in args:
                if arg == "internal":
                    if internal:
                        logger.warning("Multiple 'internal' arguments passed to 'STC' directive")
                    internal = True
                elif arg == "missing":
                    if missing:
                        logger.warning("Multiple 'missing' arguments passed to 'STC' directive")
                    missing = True
                else:
                    return log_error(get_not_a_valid_arg_message(arg, directive))

            if not missing:
                return log_error(f"'missing' not provided to 'STC' directive.", "The 'STC' directive must always be passed the 'missing' argument for readability.")
                
            test_context.expect_transpiler_failure = True
            test_context.expect_transpiler_internal_error = internal
            test_context.transpiler_failure_pattern = text_arg

        case "STC FLAG":
            if len(args) == 0:
                logger.warning("STC FLAG without any argument has no effect on transpilation.")
            # we don't really do any meaningful checks here, validity is the responsibility of the test file
            # duplicate checks are avoided, since the CLI may support flags in the future which can be repeated (e.g. multiple output paths with repeated -o args)
            for arg in args:
                if arg == "":
                    logger.warning("Empty arguments passed to STC FLAG are ignored",
                                   "This is most likely an empty argument list like STC FLAG(), or a trailing comma like STC FLAG(--my-flag, ). Prefer to avoid these.")
                    continue
                test_context.extra_cli_flags.append(arg)

        case "PRE EVAL BEGIN":
            if test_context.in_pre_eval_block():
                return log_error("'PRE EVAL BEGIN' after pre-eval block has already been opened.",
                                 "You may have intended to close the pre-eval block that was opened earlier.")
            if test_context.has_pre_eval_block():
                return log_error("'PRE EVAL BEGIN' after pre-eval block has already been defined.",
                                 "A single test file may only have one pre-eval block defined.")
            test_context.pre_eval_block = (line_number, 0)

        case "PRE EVAL END":
            if not test_context.in_pre_eval_block():
                return log_error("'PRE EVAL END' without a matching 'PRE EVAL BEGIN'.")
            if test_context.has_pre_eval_block():
                return log_error("'PRE EVAL END' after pre-eval block has been defined.", "This may be an accidentally duplicated, or leftover line.")
            test_context.pre_eval_block = (test_context.pre_eval_block[0], line_number)

        case "SKIP":
            if len(args) != 0:
                return log_error("Arguments passed to 'SKIP' directive.", "'SKIP' does not take any arguments.")

            if test_context.skip_all:
                return log_error("'SKIP' directive used more than once in a test file",
                                 "'SKIP' marks a global skip for the test file, it should be used at most once per file.")
            test_context.skip_all = True

        case "COMMENT":
            if len(args) != 0:
                return log_error(f"Arguments passed to '{directive}' directive", "'COMMENT' annotations take their body as the text argument")
            if text_arg == "":
                return log_error(f"'{directive}' directive without a text argument on line {line_number}", "'COMMENT' annotations require a text argument: the comment body")

        case _:
            hint = None
            # iterate in length descending order, so that the closest prefix match is found first (e.g. STC vs STC FLAG)
            for valid_directive in DIRECTIVE_LIST_LEN_DESC:
                if directive.startswith(valid_directive):
                    hint = f"It looks like you're missing a separator after (or you mistyped) the directive '{valid_directive}'."
                    break
            
            if hint is None and directive.find(" ") != -1 and colon_pos == -1 and open_paren_pos == -1 and close_paren_pos == -1:
                hint = f"This does not look like an annotation at all. You may be missing the text argument separator colon. Single-line comments are reserved for annotations in test files, use a multi-line comment or the COMMENT directive.\n{SYNTAX_HINT}\n"

            supported_directive_hint = f"Supported directives are {', '.join(DIRECTIVE_LIST[:-1])} and {DIRECTIVE_LIST[-1]}."
            if hint is None:
                hint = supported_directive_hint
            else:
                hint += " " + supported_directive_hint
            
            return log_error(f"'{directive}' does not name a valid directive.", hint)
        
    return True

# annotation parsing could be improved if ever needed
# currently only single-line Julia comments are supported
def parse_file(config: Config) -> TestContext | None:
    test_context = TestContext(config.path)
    
    try:
        f = open(config.path, "r", encoding="utf-8") # UTF-8 for math symbols (supported by Julia)
    except OSError:
        logger.error(f"Failed to open test file at '{config.path}'")
        return None

    # ugly, but test files are small enough for this to be fast, even using "r" instead of "rb"
    test_context.line_count = sum([1 for _ in f])
    new_pos = f.seek(0)
    if new_pos != 0:
        logger.error("Failed to reset file stream position after reading line count")
        return None
        
    parse_success = True
    with f:
        for (line_number, line) in enumerate(f, start=1):
            comment_begin_pos = line.find("#")
            next_is_eq = (len(line) > comment_begin_pos + 1 and line[comment_begin_pos + 1] == "=")
            prev_is_eq = (comment_begin_pos >= 1 and line[comment_begin_pos - 1] == "=")
            if comment_begin_pos == -1 or next_is_eq or prev_is_eq:
                continue

            raw_annotation = line[comment_begin_pos + 1:].strip()
            if not parse_annotation(test_context, line_number, raw_annotation):
                parse_success = False

    if test_context.in_pre_eval_block():
        logger.error("'PRE EVAL BEGIN' without a matching 'PRE EVAL END' in test file")
        parse_success = False

    if test_context.has_pre_eval_block():
        for report in test_context.expected_reports:
            if test_context.pre_eval_block[0] <= report.line_number <= test_context.pre_eval_block[1]:
                logger.error(f"Report annotation points to line {report.line_number}, which is inside the pre eval block")
                parse_success = False
        
    if not parse_success:
        return None

    if test_context.skip_all:
        return test_context
    
    if not test_context.should_be_valid() and not any([rep.report_type == ReportType.ERROR for rep in test_context.expected_reports]):
        logger.error("A non-valid test file (i.e. filename not starting with the prefix 'valid') should have at least one 'ERROR' directive in its annotations.")
        return None
            
    return test_context

def report_matches(expected: ExpectedReport, actual: Report, config: Config) -> bool:
    if expected.line_number != actual.line_number:
        return False

    message_check = config.skip_message_checks or expected.message is None or actual.message.find(expected.message) != -1

    report_type_check = expected.report_type == actual.report_type
    if not report_type_check and expected.internal:
        assert expected.report_type == ReportType.ERROR
        report_type_check = actual.report_type == ReportType.INTERNAL_ERROR

    return message_check and report_type_check

# format: [file:line] [category] message...
# file is dropped, because the script only ever deals with a single file
def try_parse_report(line: str) -> Report | None:
    report_match = re.fullmatch(REPORT_RE, line)

    if report_match is None:
        return None
    
    try:
        line_number = int(report_match["line"])
    except ValueError:
        return None

    match report_match.groupdict()["category"]:
        case "Warning":
            category = ReportType.WARNING
        case "Error":
            category = ReportType.ERROR
        case "Internal Error":
            category = ReportType.INTERNAL_ERROR
        case _:
            return None
    
    message = report_match.groupdict()["message"].strip()

    return Report(category, line_number, message)

# (src_path, pre_eval_path | None)
def prepare_file_for_transpilation(test_context: TestContext, config: Config) -> tuple[str, str | None] | None:
    if not test_context.has_pre_eval_block():
        return (test_context.path, None)

    src_file_path = str(config.tmp_dir_path / "src.jl")
    pre_eval_file_path = str(config.tmp_dir_path / "pre_eval.jl")

    try:
        with open(test_context.path, "r", encoding="utf-8") as test_file:
            with open(src_file_path, "w", encoding="utf-8") as src_file, open(pre_eval_file_path, "w", encoding="utf-8") as pre_eval_file:
                # take up to, but not including the first line of the pre-eval block (line containing PRE EVAL BEGIN)
                assert test_context.pre_eval_block[0] > 0
                for line in itertools.islice(test_file, test_context.pre_eval_block[0] - 1):
                    _ = src_file.write(line)
                # take up to and including the last line of the pre-eval block (line containing PRE EVAL END)
                pre_eval_length = test_context.pre_eval_block[1] - test_context.pre_eval_block[0] + 1
                assert pre_eval_length >= 0
                for line in itertools.islice(test_file, pre_eval_length):
                    _ = pre_eval_file.write(line)
                    _ = src_file.write("\n") # this keeps numbering consistent with the already parsed report annotations, without requiring any offset math later
                # drain rest of the file
                for line in test_file:
                    _ = src_file.write(line)

        return src_file_path, pre_eval_file_path
    except OSError as e:
        logger.error(f"Failed to split test file into separate source and pre eval files:\n{e}")
        return None

def collect_reports(test_context: TestContext, config: Config) -> list[Report] | ExitCode:
    should_be_valid = test_context.should_be_valid()
    only_expects_missing_errors = all([rep.missing for rep in test_context.expected_reports if rep.report_type == ReportType.ERROR])
    
    stc_proc_env = os.environ.copy()
    stc_proc_env["NO_COLOR"] = "1"
    if config.julia_bin_dir is not None:
        stc_proc_env["PATH"] = config.julia_bin_dir + os.pathsep + stc_proc_env.get("PATH", "")

    cli_args = ["--no-time", "--quiet", "--cg-indent", "0"]
    if should_be_valid:
        assert test_context.out_path != ""
        cli_args.extend(["-o", test_context.out_path])
    else:
        cli_args.append("--no-out")

    prepare_result = prepare_file_for_transpilation(test_context, config)
    if prepare_result is None:
        logger.error("Failed to prepare test files for transpilation")
        return ExitCode.UNEXPECTED_ERROR

    src_path, pre_eval_path = prepare_result
    if pre_eval_path is not None:
        cli_args.extend(["--pre-eval", pre_eval_path])

    cli_args.extend(test_context.extra_cli_flags)

    stc_proc_result = None
    try:
        subproc_args = [config.cli, src_path, *cli_args]
        logger.debug(f"Running '{" ".join(subproc_args)}'")
        stc_proc_result = subprocess.run(subproc_args, env=stc_proc_env, timeout=SUBPROCESS_TIMEOUT_SEC,
                                         stderr=subprocess.PIPE, text=True, encoding="utf-8", errors="replace")
    except OSError as e:
        logger.error(f"Failed to run transpiler CLI using the provided path: '{config.cli}':\n{e}")
        return ExitCode.ARGS_MALFORMED
    except subprocess.TimeoutExpired as e:
        stderr_output = parse_subproc_output(e.stderr)
        tail = f"see output below:\n== STC OUTPUT BEGIN ==\n{stderr_output}\n== STC OUTPUT END ==" if stderr_output != "" else "no output was captured"
        logger.error(f"stc timed out after {e.timeout} seconds, {tail}")
        return ExitCode.TRANSPILER_ERROR

    def log_error_with_stc_output(msg: str):
        logger.error(msg)
        logger.debug("See stc stderr output for details:")
        logger.debug(f"== STC OUTPUT BEGIN ==\n{stc_proc_result.stderr}\n== STC OUTPUT END ==")

    if stc_proc_result.returncode not in [0, 1]:
        log_error_with_stc_output(f"Unexpected stc exit code: {stc_proc_result.returncode}")
        if stc_proc_result.returncode == 0xC0000135:
            logger.error(f"Hint: based on the exit code, this is a DLL not found error. The most likely culprit is libjulia not being present in PATH while it's also not set correctly through --julia-bin-dir.")
        return ExitCode.TRANSPILER_ERROR

    collected_error_reports = False
    reports: list[Report] = []
    for line in stc_proc_result.stderr.splitlines():
        if line.startswith("[Internal Error]"): # STC(missing, internal) does not affect this, since this is specifically for catching internal errors without source location info
            log_error_with_stc_output("Internal error without source location information in transpiler output")
            return ExitCode.TESTS_FAILED
        
        maybe_report = try_parse_report(line)
        if maybe_report is not None:
            reports.append(maybe_report)
            if maybe_report.report_type in [ReportType.ERROR, ReportType.INTERNAL_ERROR]:
                collected_error_reports = True
                
    if stc_proc_result.returncode == 0:
        test_context.stc_success = True
    
    # TODO: separate these checks from report collection (and use TestResult instead of ExitCode)
    if should_be_valid:
        base_log = "Check that file named 'valid*' transpiles successfully ---"
        if stc_proc_result.returncode != 0:
            if test_context.expect_transpiler_failure:
                if test_context.transpiler_failure_pattern == "" or stc_proc_result.stderr.find(test_context.transpiler_failure_pattern) != -1:
                    logger.info(f"{base_log} skipped (marked missing)")
                    test_context.transpiler_failure_skipped = True
                else:
                    log_error_with_stc_output(f"{base_log} failed (marked missing, but output did not match the provided pattern)")
                    return ExitCode.TESTS_FAILED
            else:
                log_error_with_stc_output(f"{base_log} failed")
                return ExitCode.TESTS_FAILED
        else:
            if test_context.expect_transpiler_failure:
                handle_stale(f"Transpilation was marked as an expected failure (annotated with 'STC(missing)'), but stc reported no errors.", config, test_context)
            logger.info(f"{base_log} passed")
    
    if not should_be_valid and not only_expects_missing_errors and stc_proc_result.returncode == 0:
        logger.error("Transpilation succeeded for non-valid test file. " +
                     "Test files that are expected to transpile successfully should be named with the prefix 'valid'. ")
        return ExitCode.TESTS_FAILED

    # check that at least one of the missing cases was hit (otherwise the transpiler failed for an unrelated reason)    
    if not should_be_valid and only_expects_missing_errors and stc_proc_result.returncode != 0:
        missing_case_hit = False
        for (expected, actual) in ((e, a) for e in test_context.expected_reports for a in reports):
            if expected.report_type != ReportType.ERROR:
                continue

            assert expected.missing or expected.internal
            if report_matches(expected, actual, config):
                missing_case_hit = True
                break
            
        if not missing_case_hit:
            log_error_with_stc_output(f"Transpilation failed (with exit code {stc_proc_result.returncode}) for test file containing only missing ERROR directives, but none of them were the reason for failure.")
            return ExitCode.TESTS_FAILED

    if collected_error_reports and stc_proc_result.returncode == 0:
        log_error_with_stc_output("Transpilation succeeded after transpilation produced error reports.")
        return ExitCode.TRANSPILER_ERROR
    
    if not collected_error_reports and stc_proc_result.returncode != 0:
        log_error_with_stc_output(f"Transpilation failed (with exit code {stc_proc_result.returncode}), but no error reports were found")
        return ExitCode.TRANSPILER_ERROR
        
    return reports

def validate_reports(test_context: TestContext, actual_reports: list[Report], config: Config) -> TestResult:
    expected_reports = test_context.expected_reports
    
    # validation is done "inefficiently", in several passes, to allow all errors to be printed at once in case of failure
    # it's also assumed that actual and expected report count is small enough for repeated O(n*m) comparisons to not matter here
    
    used_skips = False
    
    internal_errors_check = True
    internal_used_skips = False
    if not test_context.expect_transpiler_internal_error:
        for actual in actual_reports:
            if actual.report_type == ReportType.INTERNAL_ERROR:
                base_log = f"Internal error on line {actual.line_number} ---"
                any_expected_matches = False
                for expected in expected_reports:
                    if report_matches(expected, actual, config):
                        any_expected_matches = True
                        break

                if not any_expected_matches:
                    logger.error(f"{base_log} failed")
                    internal_errors_check = False
                else:
                    logger.info(f"{base_log} skipped (marked expected)")
                    internal_used_skips = True
                    used_skips = True
    else:
        had_internal_errors = any(rep.report_type == ReportType.INTERNAL_ERROR for rep in actual_reports)
        if test_context.transpiler_failure_skipped and not had_internal_errors:
            handle_stale("Transpilation produced no internal errors, but test file was marked as expecting to fail transpilation with internal errors (STC(missing, internal)).", config, test_context)
        internal_used_skips = had_internal_errors

    if internal_errors_check:
        logger.info(f"{SUMMARY_PREFIX} Internal error checks passed{" (with skips)" if internal_used_skips else ""}")
    else:
        logger.error(f"{SUMMARY_PREFIX} Internal error checks failed")
    logger.info("")
    
    lenient_check = True
    lenient_used_skips = False
    for expected in expected_reports:
        if expected.message is not None:
            base_log = f"Case: '...{expected.message}...' {expected.report_type.__str__().lower()} on line {expected.line_number} ---" 
        else:
            base_log = f"Case: any {expected.report_type.__str__().lower()} on line {expected.line_number} ---" 

        same_line_reports: list[Report] = []
        matched: Report | None = None
        for actual in actual_reports:
            if expected.line_number == actual.line_number:
                if report_matches(expected, actual, config):
                    matched = actual
                    # if multiple reports match, we try to find the closest one, i.e. internal for internal, anything else for others
                    if expected.internal == (actual.report_type == ReportType.INTERNAL_ERROR):
                        break
                
                same_line_reports.append(actual)

        if matched is not None:
            logger.info(f"{base_log} passed")
            if expected.missing:
                handle_stale(f"Previous case was marked as an expected failure (marked 'missing'), but a matching report was found.", config, test_context)
            if expected.internal and matched.report_type != ReportType.INTERNAL_ERROR:
                handle_stale(f"Previous case was marked as an expected internal error (marked 'internal'), but the matching report is not an internal error.", config, test_context)
            continue
        
        if expected.missing:
            logger.info(f"{base_log} skipped (marked missing)")
            used_skips = True
            lenient_used_skips = True
            continue
        
        logger.error(f"{base_log} failed")
        lenient_check = False
        if len(same_line_reports) != 0:
            logger.info(f"  Why: found the following report(s) for line {expected.line_number}, but none of them match the expected pattern:")
            for report in same_line_reports:
                logger.info(f"    \"[{report.report_type}] {report.message.splitlines()[0]}\"")
        else:
            logger.info(f"  Why: no reports were generated for line {expected.line_number}")
            
    if lenient_check:
        logger.info(f"{SUMMARY_PREFIX} Lenient checks passed{" (with skips)" if lenient_used_skips else ""}")
    else:
        logger.error(f"{SUMMARY_PREFIX} Lenient checks failed")
    logger.info("")
    
    strict_check = True
    # if a valid file's transpilation failed but was expected (i.e. resulted in a skip), error messages cannot be matched
    # because ERROR directives are not allowed in valid test files. in this case we skip strict checks, to stop a skipped
    # transpiler failure propagating into an actual test failure in strict mode
    if config.strict() and not test_context.transpiler_failure_skipped:
        for actual in actual_reports:
            found = False
            base_log = f"[line {actual.line_number}] [{actual.report_type}] {actual.message} ---"
            
            for expected in expected_reports:
                if report_matches(expected, actual, config):
                    found = True
                    break
            
            if found:
                logger.info(f"{base_log} passed")
            else:
                strict_check = False
                logger.error(f"{base_log} failed")
        
        if strict_check:
            logger.info(f"{SUMMARY_PREFIX} Strict checks passed")
        else:
            logger.error(f"{SUMMARY_PREFIX} Strict checks failed")
    elif test_context.transpiler_failure_skipped:
        logger.info(f"{SUMMARY_PREFIX} Strict checks skipped (expected transpiler failure)")
        used_skips = True
    logger.info("")

    result = TestResult.PASSED if internal_errors_check and lenient_check and strict_check else TestResult.FAILED
    return TestResult.PASSED_WITH_SKIPS if result == TestResult.PASSED and used_skips else result

def validate_output(test_context: TestContext, config: Config) -> TestResult | ExitCode:
    result = TestResult.PASSED

    assert len(test_context.expected_outputs) > 0

    transpiler_output: str = ""
    if test_context.stc_success:
        try:
            with open(test_context.out_path, "r", encoding="utf-8") as transpiler_output_file:
                transpiler_output = transpiler_output_file.read()
        except OSError as e:
            logger.error(f"Failed to read output of transpilation:\n{e}")
            return ExitCode.UNEXPECTED_ERROR

    # we don't early return here if not stc_success, to still display the output checks and indicate them as skipped
    for expected in test_context.expected_outputs:
        base_log = f"Case {expected.pattern_str()} in GLSL output ---"
        if not test_context.stc_success:
            logger.info(f"{base_log} skipped (failed transpilation)")
            result = TestResult.merge(result, TestResult.PASSED_WITH_SKIPS)
            continue

        matches: bool = False
        if isinstance(expected.pattern, re.Pattern):
            matches = re.search(expected.pattern, transpiler_output) is not None
        else:
            assert isinstance(expected.pattern, str)
            matches = transpiler_output.find(expected.pattern) != -1

        if matches:
            logger.info(f"{base_log} passed")
            if expected.missing:
                handle_stale(f"Previous case was marked as an expected failure (marked 'missing'), but a matching output snippet was found.", config, test_context)
            continue

        if expected.missing:
            logger.info(f"{base_log} skipped (marked missing)")
            result = TestResult.merge(result, TestResult.PASSED_WITH_SKIPS)
            continue

        result = TestResult.FAILED
        logger.error(f"{base_log} failed")

    return result

# GLSL validation via glslangValidator below:
def validate_glsl(test_context: TestContext, config: Config) -> TestResult | ExitCode:   
    # only valid shaders can be GLSL validated
    if not test_context.should_be_valid():
        return TestResult.PASSED
    
    if config.glslang_validator is None:
        return TestResult.PASSED_WITH_SKIPS
    
    base_log = "Run glslangValidator on generated code ---"
    
    if not test_context.stc_success:
        logger.info(f"{base_log} skipped (transpilation failed)")
        return TestResult.PASSED_WITH_SKIPS
    
    try:
        subproc_args = [config.glslang_validator, "-l", test_context.out_path]
        logger.debug(f"Running '{" ".join(subproc_args)}'")
        proc_result = subprocess.run(subproc_args, text=True, encoding="utf-8", errors="replace",
                                     timeout=SUBPROCESS_TIMEOUT_SEC, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except OSError as e:
        logger.error(f"Failed to run glslangValidator using the provided path: '{config.glslang_validator}':\n{e}")
        return ExitCode.ARGS_MALFORMED
    except subprocess.TimeoutExpired as e:
        stdout_output = parse_subproc_output(e.stdout)
        tail = f"see output below:\n== GLSLANG VALIDATOR OUTPUT BEGIN ==\n{stdout_output}\n== GLSLANG VALIDATOR OUTPUT END ==" if stdout_output != "" else "no output was captured"
        logger.error(f"glslangValidator timed out after {e.timeout} seconds, {tail}")
        return ExitCode.GLSLANG_VALIDATOR_ERROR
    
    if proc_result.returncode == 0:
        logger.info(f"{base_log} passed")
        if test_context.expected_glsl_result.expected_to_fail():
            handle_stale(f"GLSL validation was marked as an expected or intended failure (annotated with 'GLSL(missing)'), but glslangValidator reported no errors.", config, test_context)
        return TestResult.PASSED
    
    def dump_glslang_validator_output():
        logger.info(f"See glslangValidator output for details:\n" +
                    "== GLSLANG VALIDATOR OUTPUT BEGIN ==\n" + 
                    proc_result.stdout +
                    "\n== GLSLANG VALIDATOR OUTPUT END ==")
    
    if proc_result.returncode in GLSLANG_VALIDATOR_REJECTED_CODES:
        if test_context.expected_glsl_result.expected_to_fail():
            matched_pattern = (test_context.glsl_failure_pattern == "" or proc_result.stdout.find(test_context.glsl_failure_pattern) != -1)
            if matched_pattern:
                if test_context.expected_glsl_result == ExpectedGLSLResult.EXPECTED_TO_FAIL:
                    logger.info(f"{base_log} skipped (marked missing)")
                    return TestResult.PASSED_WITH_SKIPS
                if test_context.expected_glsl_result == ExpectedGLSLResult.INTENDED_TO_FAIL:
                    logger.info(f"{base_log} passed (marked missing and intended)")
                    return TestResult.PASSED
                assert False, "unexpected ExpectedGLSLResult value"
            else:
                logger.error(f"{base_log} failed (marked missing, but output did not match the provided pattern)")
        else:
            logger.error(f"{base_log} failed")
        dump_glslang_validator_output()
        
        return TestResult.FAILED
    
    logger.error(f"glslangValidator returned with unexpected exit code: {proc_result.returncode}")
    dump_glslang_validator_output()
    return ExitCode.GLSLANG_VALIDATOR_ERROR

# strips path base up to the test/conformance subdirectory
def get_stripped_path(path: str) -> str:
    conformance_tests_base_dir = Path(__file__).resolve().parent
    test_path = Path(path).resolve()
    
    try:
        return str(test_path.relative_to(conformance_tests_base_dir))
    except ValueError:
        return path

def main() -> ExitCode:    
    arg_parser = argparse.ArgumentParser(description=__doc__)
    _ = arg_parser.add_argument("--path", required=True, help="path of the conformance test file to run")
    _ = arg_parser.add_argument("--cli", required=True, help="path of the stc CLI executable (or wrapper script) to use for running the tests")
    _ = arg_parser.add_argument("--mode", choices=("strict", "lenient"), default="lenient", help="lenient mode requires all report annotations to have a matching report generated to pass, strict mode additionally requires all error reports to have a matching report annotation as well (default: lenient)")
    _ = arg_parser.add_argument("--julia-bin-dir", help="path of the installed Julia runtime's bin directory, prepended to the PATH of the subprocess' environment that runs stc (should only be needed on Windows)")
    _ = arg_parser.add_argument("--glslang-validator", help="path of the installed glslangValidator executable, which should be used for running the GLSL validation tests (default: try to find through PATH)")
    _ = arg_parser.add_argument("--fail-on-stale", action="store_true", help="fail when a test raises a [STALE] warning, indicating that a temporary bypass annotation is no longer needed")
    _ = arg_parser.add_argument("--verbose", action="store_true", help="print DEBUG level messages")
    _ = arg_parser.add_argument("--skip-message-checks", action="store_true", help="if enabled, message pattern checks will pass unconditionally")
    _ = arg_parser.add_argument("--keep-artifacts", action="store_true", help="do not delete any temporary files created by the runner")
    args = arg_parser.parse_args()

    log_handler = logging.StreamHandler()
    log_handler.setFormatter(LogFormatter())
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO, handlers=[log_handler])
    
    tmp_dir = tempfile.TemporaryDirectory(prefix="conformance_output_", delete=(not args.keep_artifacts))
    tmp_dir_path = Path(tmp_dir.name).resolve()

    if args.keep_artifacts:
        logger.info(f"Keeping artifacts directory: {tmp_dir_path}")
    
    config = Config(args.path, args.cli, CheckingMode.parse(args.mode), args.skip_message_checks,
                    tmp_dir_path, args.fail_on_stale, args.julia_bin_dir, args.glslang_validator)
    
    if config.julia_bin_dir is not None:
        if not os.path.exists(config.julia_bin_dir):
            logger.error(f"The path specified as the Julia bin directory ({config.julia_bin_dir}) does not exist")
            return ExitCode.ARGS_MALFORMED
        
        if not os.path.isdir(config.julia_bin_dir):
            logger.error(f"The path specified as the Julia bin directory ({config.julia_bin_dir}) does not point to a directory")
            return ExitCode.ARGS_MALFORMED
        
        if len([f for f in Path(config.julia_bin_dir).glob("libjulia.*") if f.is_file()]) == 0:
            logger.warning(f"The directory specified as the Julia bin directory ({config.julia_bin_dir}) does not contain a file named 'libjulia'. " +
                           "Testing will continue, but --julia-bin-dir will not have an effect on whether or not the loader is able to locate libjulia.")
            
    if config.glslang_validator is None:
        config.glslang_validator = shutil.which("glslangValidator")
    if config.glslang_validator is None:
        logger.warning("glslangValidator was not specified explicitly, and could not be resolved through PATH. GLSL validation tests will be skipped.")
    elif not os.path.isfile(config.glslang_validator):
        logger.warning(f"Path for the glslangValidator executable does not exist, or is not a file. GLSL validation tests will be skipped. ({config.glslang_validator})")
        config.glslang_validator = None
    
    stripped_path = get_stripped_path(config.path)
    logger.info(f"Running tests for '{stripped_path}'...")
    logger.info("")
    
    test_context = parse_file(config)
    if test_context is None:
        logger.error("Failed to parse test file")
        return ExitCode.TESTS_MALFORMED

    if test_context.skip_all:
        logger.info("Skipping test file (marked 'SKIP')")
        return ExitCode.TESTS_PASSED_WITH_SKIPS
    
    if test_context.should_be_valid():
        test_context.out_path = str(config.tmp_dir_path / f"out.{test_context.shader_stage}")
    
    maybe_reports: list[Report] | ExitCode = collect_reports(test_context, config)
    if isinstance(maybe_reports, ExitCode):
        logger.error("Failed to collect transpiler reports")
        return maybe_reports
    
    result = validate_reports(test_context, maybe_reports, config)

    if len(test_context.expected_outputs) > 0:
        output_result: TestResult | ExitCode = validate_output(test_context, config)
        if isinstance(output_result, ExitCode):
            logger.error("Failed to run output validation tests")
            return output_result

        assert isinstance(output_result, TestResult)
        result = TestResult.merge(result, output_result)

        match output_result:
            case TestResult.PASSED:            logger.info(f"{SUMMARY_PREFIX} Output validation checks passed")
            case TestResult.PASSED_WITH_SKIPS: logger.info(f"{SUMMARY_PREFIX} Output validation checks passed (with skips)")
            case TestResult.FAILED:            logger.error(f"{SUMMARY_PREFIX} Output validation checks failed")
            case _:
                logger.error(f"{SUMMARY_PREFIX} Output validation check status unknown")
                return ExitCode.UNEXPECTED_ERROR
    else:
        logger.info(f"{SUMMARY_PREFIX} Output validation checks skipped (no OUTPUT constraints specified)")
    logger.info("")
    
    if test_context.should_be_valid():
        glsl_result: TestResult | ExitCode = validate_glsl(test_context, config)
        if isinstance(glsl_result, ExitCode):
            logger.error("Failed to run GLSL validation checks")
            return glsl_result
        
        assert isinstance(glsl_result, TestResult)
        result = TestResult.merge(result, glsl_result)
        
        match glsl_result:
            case TestResult.PASSED:            logger.info(f"{SUMMARY_PREFIX} GLSL validation checks passed")
            case TestResult.PASSED_WITH_SKIPS: logger.info(f"{SUMMARY_PREFIX} GLSL validation checks passed (with skips)")
            case TestResult.FAILED:            logger.error(f"{SUMMARY_PREFIX} GLSL validation checks failed")
            case _:
                logger.error(f"{SUMMARY_PREFIX} GLSL validation check status unknown")
                return ExitCode.UNEXPECTED_ERROR
    else:
        logger.info(f"{SUMMARY_PREFIX} GLSL validation checks skipped (file is not marked valid)")
    logger.info("")

    base_log = f"{SUMMARY_PREFIX} Test staleness checks"
    if not test_context.has_stale:
        logger.info(f"{base_log} passed")
    else:
        if config.fail_on_stale:
            logger.error(f"{base_log} failed")
            # let failed tests "overpower" staleness results (tests fail either way)
            if result.value <= TestResult.PASSED_WITH_SKIPS.value:
                return ExitCode.TESTS_STALE
        else:
            # we don't force result here to PASSED_WITH_SKIPS, since staleness is orthogonal enough to test results for me to not want to propagate it beyond printing (unless --fail-on-stale is passed)
            logger.info(f"{base_log} skipped (--fail-on-stale not passed)")
    logger.info("")
    
    match result:
        case TestResult.PASSED:
            logger.info(f"All tests of '{stripped_path}' passed")
            return ExitCode.TESTS_PASSED
        case TestResult.PASSED_WITH_SKIPS:
            logger.info(f"All tests of '{stripped_path}' passed (some used skips)")
            return ExitCode.TESTS_PASSED_WITH_SKIPS
        case TestResult.FAILED:
            logger.info(f"Some tests of '{stripped_path}' failed")
            return ExitCode.TESTS_FAILED

    logger.error("Unexpected test result")
    return ExitCode.UNEXPECTED_ERROR

if __name__ == "__main__":
    exit_code: ExitCode = main()
    logger.info(f"\nFinal result: {exit_code} (exit code {exit_code.value})")
    sys.exit(exit_code.value)
