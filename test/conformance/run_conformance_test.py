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
from typing import override
from pathlib import Path
from enum import Enum

REPORT_RE = re.compile(r"^\[(?P<file>.*):(?P<line>\d+):(?P<col>\d+)\] \[(?P<category>Error|Warning|Internal Error)\] (?P<message>.*)$")

SUMMARY_PREFIX = "==>"

SUBPROCESS_TIMEOUT_SEC = 30

# codes that fail a test compilation (unknown non-zero error codes result in UNKNOWN)
GLSLANG_VALIDATOR_REJECTED_CODES = [2, 3]

logger = logging.getLogger(__name__)

class ExitCode(Enum):
    TESTS_PASSED = 0 # all tests passed without any being skipped
    TESTS_FAILED = 1 # at least one test failed, and was not marked for skipping
    ARGS_MALFORMED = 2 # baked into argparse, non-modifiable
    TESTS_MALFORMED = 3 # the format of the test file, or its annotations is invalid
    TRANSPILER_ERROR = 4 # encountered an unexpected transpiler-side error
    UNEXPECTED_ERROR = 5 # catch-all for any other internal error of the script
    GLSLANG_VALIDATOR_ERROR = 6 # unknown errors coming from glslangValidator
    TESTS_PASSED_WITH_SKIPS = 77 # tests passed, but some were skipped (e.g. because they were marked missing in the test file)

class ReportType(Enum):
    WARNING = 0
    ERROR = 1
    INTERNAL_ERROR = 2

    @override
    def __str__(self):
        match self:
            case ReportType.WARNING:        return "Warning"
            case ReportType.ERROR:          return "Error"
            case ReportType.INTERNAL_ERROR: return "Internal Error"
            case _:                         return "Unknown"
            
class ScriptMode(Enum):
    LENIENT = 0
    STRICT = 1

    @staticmethod
    def parse(mode: str) -> 'ScriptMode':
        match mode:
            case "lenient": return ScriptMode.LENIENT
            case "strict": return ScriptMode.STRICT
            case _: raise ValueError(f"Value '{mode}' is not a valid script mode")
    
    @override
    def __str__(self):
        match self:
            case ScriptMode.LENIENT: return "Lenient"
            case ScriptMode.STRICT:  return "Strict"
            case _:                  return "Unknown"
    
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

class Report:
    def __init__(self, report_type: ReportType, line_number: int, message: str):
        self.report_type: ReportType = report_type
        self.line_number: int = line_number
        self.message = message

    @override
    def __str__(self):
        return f"{self.report_type} report for line {self.line_number} with message '{self.message}'"

class ExpectedReport(Report):
    def __init__(self, report_type: ReportType, line_number: int, message: str | None, expected_missing: bool):
        Report.__init__(self, report_type, line_number, "")
        self.message = message
        self.expected_missing: bool = expected_missing

    @override
    def __str__(self):
        return f"Expected {self.report_type} report for line {self.line_number}{f" with message pattern '{self.message}'" if self.message is not None else ""}{", expected to fail" if self.expected_missing else ""}"
        
class AnnotationContext:
    def __init__(self):
        self.parsed_shader_stage_directive: bool = False
        self.parsed_glsl_directive: bool = False
        self.parsed_stc_directive: bool = False
        
class FileInfo:
    def __init__(self, path: str):
        self.path: str = path
        self.line_count: int = 0
        self.expected_reports: list[ExpectedReport] = []
        self.shader_stage: ShaderStage = ShaderStage.COMPUTE
        self.out_path: str = ""
        self.expect_glsl_failure: bool = False
        self.glsl_failure_pattern: str = "" # only applies when expect_glsl_failure == True
        self.expect_transpiler_failure: bool = False
        self.transpiler_failure_pattern: str = "" # only applies when expect_transpiler_failure == True
        self.annotation_context: AnnotationContext = AnnotationContext()
    
    def should_be_valid(self):
        return Path(self.path).name.startswith("valid")

class Config:
    def __init__(self, path: str, cli: str, mode: ScriptMode, allow_internal_errors: bool, skip_message_checks: bool, tmp_dir_path: str,
                 julia_bin_dir: str | None, glslang_validator: str | None):
        self.path: str = path
        self.cli: str = cli
        self.mode: ScriptMode = mode
        self.allow_internal_errors: bool = allow_internal_errors
        self.skip_message_checks: bool = skip_message_checks
        self.tmp_dir_path: str = tmp_dir_path
        self.julia_bin_dir: str | None = julia_bin_dir
        self.glslang_validator: str | None = glslang_validator
        self.stc_success: bool = False

    def strict(self) -> bool:
        return self.mode == ScriptMode.STRICT

def parse_subproc_output(output: bytes | str | None) -> str:
    if isinstance(output, bytes):
        return output.decode("utf-8", "replace").strip()
    if isinstance(output, str):
        return output.strip()
    return ""

# annotation syntax: <directive>[(<argument1>[, <argument2> [...]])] [:<text_argument>]
# <directive> is defined as the substring ranging from the start of the annotation to the first '(' or ':' character
# <text_argument> is defined as the substring ranging from the first colon to the end of the annotation (so extra colons in <text_argument> are allowed)
# <directive>, <argumentN> and <text_argument> are all stripped from leading and trailing whitespace
# supported directives (and their variants):
#   ERROR                register an expected error report for a given line
#       <arguments>:
#           missing      the report is expected to be missing from the output, test will be skipped (can be used to mark known gaps during testing)
#           +<n>         the report should apply to the line <n> lines below whatever line the annotation is on
#           -<n>         the report should apply to the line <n> lines above whatever line the annotation is on
#       <text_argument>: if provided, acts as a pattern that has to be a substring of the actual report's message for the test to pass
#   WARNING              same as ERROR, for warning reports
#   COMMENT              directive for leaving single-line comments in the test files (multi-line comments are currently ignored, so they don't require this)
#       <text_argument>: the body of the comment
#   SHADER STAGE         specify the shader stage used with glslangValidator (only affects valid files). must be used at most once per test file.
#       <arguments>:     must be exactly one of the available options
#           comp         compute shader (default if SHADER STAGE is not specified)
#           frag         fragment shader
#   GLSL                 along with the 'missing' argument, specifies that the shader is currently expected to not pass validation via glslangValidator. must be used at most once per test file.
#       <arguments>:
#           missing      must be passed with GLSL
#       <text_argument>: if provided, can be used to specify a message pattern that has to appear as a substring of the glslangValidator output for the validation failure to be treated as skipped.
#   STC(missing)         same as GLSL(missing) but for the transpiler. marks that an otherwise valid test file is currently expected to fail transpilation.
def parse_annotation(file_info: FileInfo, line_number: int, raw_annotation: str) -> bool:
    def log_error(msg: str, hint: str | None = None) -> bool:
        hint_tail = f"\n  Hint: {hint}" if hint is not None else ""
        logger.error(f"Malformed annotation on line {line_number}: '{raw_annotation}'\n  Why: {msg}{hint_tail}")
        return False
        
    if raw_annotation == "":
        return log_error("Empty annotation", "Single line comments are reserved for annotations in test files")
    
    colon_pos = raw_annotation.find(":")
    
    open_paren_pos = raw_annotation.find("(", 0, colon_pos if colon_pos != -1 else len(raw_annotation))
    close_paren_pos = raw_annotation.find(")", open_paren_pos) if open_paren_pos != -1 else -1
    if open_paren_pos != -1 and close_paren_pos == -1:
        return log_error("Opening parenthesis for annotation argument list doesn't have a matching closing parenthesis")
    
    assert open_paren_pos == close_paren_pos == -1 or open_paren_pos < close_paren_pos
    assert open_paren_pos + 1 < len(raw_annotation)
    
    if close_paren_pos != -1:
        if colon_pos != -1:
            assert close_paren_pos < colon_pos
            maybe_junk = raw_annotation[close_paren_pos + 1:colon_pos].strip()
            if maybe_junk != "":
                return log_error(f"Unexpected text found between argument list and text argument: '{maybe_junk}'", "Only whitespace is allowed between the argument list closing ')' and text argument opening ':' characters")
        elif close_paren_pos + 1 < len(raw_annotation):
            tail = raw_annotation[close_paren_pos + 1:].strip()
            if tail != "":
                return log_error(f"Unexpected text found after argument list: '{tail}'", f"Only whitespace is allowed after the argument list closing ')' character, unless a text argument is also provided.")
         
    directive = raw_annotation[:open_paren_pos] if open_paren_pos != -1 else raw_annotation[:colon_pos] if colon_pos != -1 else raw_annotation    
    directive = directive.strip()
    
    args: list[str] = raw_annotation[open_paren_pos + 1:close_paren_pos].split(",") if open_paren_pos != -1 else []
    args = [arg.strip() for arg in args]
    
    text_arg = raw_annotation[colon_pos + 1:] if colon_pos != -1 and colon_pos + 1 < len(raw_annotation) else ""
    text_arg = text_arg.strip()

    # TODO: deduplicate code
    DIRECTIVE_LIST = ["ERROR", "WARNING", "COMMENT", "SHADER STAGE", "GLSL", "STC"]
    match directive:
        case "ERROR" | "WARNING":
            if directive == "ERROR" and file_info.should_be_valid():
                return log_error("ERROR directive not allowed in files containing the prefix 'valid', since their transpilation should be successful.")
            
            report_type = ReportType.ERROR if directive == "ERROR" else ReportType.WARNING
            
            offset = 0
            found_offset = False
            
            missing = False

            for arg in args:
                if arg.startswith("+") or arg.startswith("-"):
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
                else:
                    return log_error(f"'{arg}' is not a valid argument for the '{directive}' directive")
                
            report_line_number = line_number + offset
            if report_line_number < 1 or report_line_number > file_info.line_count:
                add_offset_str = f"+ {offset}" if offset >= 0 else f"- {abs(offset)}"
                return log_error(f"Offset points outside the valid line number range (which is [1-{file_info.line_count}]) with {line_number} {add_offset_str} = {report_line_number}")
            
            report_message = text_arg if text_arg != "" else None
            
            file_info.expected_reports.append(ExpectedReport(report_type, report_line_number, report_message, missing))
        
        case "COMMENT":
            if len(args) != 0:
                return log_error(f"Arguments passed to '{directive}' directive")
            if text_arg == "":
                logger.warning(f"'{directive}' directive without a text argument on line {line_number}")
        
        case "SHADER STAGE":
            if file_info.annotation_context.parsed_shader_stage_directive:
                return log_error(f"Multiple annotations with 'SHADER STAGE' directive.", "'SHADER STAGE' must be specified at most once per test file.")
            file_info.annotation_context.parsed_shader_stage_directive = True
            
            if not file_info.should_be_valid():
                logger.warning("'SHADER STAGE' directive will not have any effect in non-valid file.\n" +
                               "  Hint: Files that are expected to transpile without error (and can thus be GLSL validated) should be marked by prefixing their name with 'valid'.")
            
            if text_arg != "":
                return log_error(f"Text argument passed to 'SHADER STAGE' directive.")
            
            if len(args) == 0:
                return log_error(f"No arguments provided to 'SHADER STAGE' directive, expected 1.")
            
            if len(args) > 1:
                return log_error(f"Multiple arguments provided to 'SHADER STAGE' directive, expected 1.")

            if args[0] == "comp":
                file_info.shader_stage = ShaderStage.COMPUTE
            elif args[0] == "frag":
                file_info.shader_stage = ShaderStage.FRAGMENT
            else:
                return log_error(f"'{args[0]}' is not a valid argument for the 'SHADER STAGE' directive.", "Supported shader stages are: comp and frag")
            
        case "GLSL":
            if file_info.annotation_context.parsed_glsl_directive:
                return log_error(f"Multiple annotations with 'GLSL' directive.", "'GLSL' must be specified at most once per test file.")
            file_info.annotation_context.parsed_glsl_directive = True
            
            if not file_info.should_be_valid():
                logger.warning("'GLSL' directive will not have any effect in non-valid file.\n" +
                               "  Hint: Files that are expected to transpile without error (and can thus be GLSL validated) should be marked by prefixing their name with 'valid'.")
                        
            if len(args) != 1 or args[0] != "missing":
                return log_error(f"Invalid argument(s) provided to 'GLSL' directive.", "The 'GLSL' directive's only supported argument is 'missing', which must always be used.")

            file_info.expect_glsl_failure = True
            file_info.glsl_failure_pattern = text_arg
            
        case "STC":
            if file_info.annotation_context.parsed_stc_directive:
                return log_error(f"Multiple annotations with 'STC' directive.", "'STC' must be specified at most once per test file.")
            file_info.annotation_context.parsed_stc_directive = True
            
            if not file_info.should_be_valid():
                logger.warning("'STC' directive will not have any effect in non-valid file.\n" +
                               "  Hint: Files that are normally expected to transpile without error should be marked by prefixing their name with 'valid'.")
                        
            if len(args) != 1 or args[0] != "missing":
                return log_error(f"Invalid argument(s) provided to 'STC' directive.", "The 'STC' directive's only supported argument is 'missing', which must always be used.")

            file_info.expect_transpiler_failure = True
            file_info.transpiler_failure_pattern = text_arg
           
        case _:
            hint = None
            for valid_directive in DIRECTIVE_LIST:
                if directive.startswith(valid_directive):
                    hint = f"It looks like you're missing a separator after (or mistyped) the directive '{valid_directive}'."
                    break
            
            if hint is None and directive.find(" ") != -1 and colon_pos == -1 and open_paren_pos == -1 and close_paren_pos == -1:
                hint = f"This does not look like an annotation at all. Single-line comments are reserved for annotations in test files, use a multi-line comment or the COMMENT directive."

            supported_directive_hint = f"Supported directives are {', '.join(DIRECTIVE_LIST[:-1])} and {DIRECTIVE_LIST[-1]}."
            if hint is None:
                hint = supported_directive_hint
            else:
                hint += " " + supported_directive_hint
            
            return log_error(f"'{directive}' does not name a valid directive.", hint)
        
    return True

# annotation parsing could be improved if ever needed
# currently only single-line Julia comments are supported
def parse_file(config: Config) -> FileInfo | None:
    file_info = FileInfo(config.path)
    
    try:
        f = open(config.path, "r", encoding="utf-8") # UTF-8 for math symbols (supported by Julia)
    except OSError:
        logger.error(f"Failed to open test file at '{config.path}'")
        return None

    # ugly, but test files are small enough for this to be fast, even using "r" instead of "rb"
    file_info.line_count = sum([1 for _ in f])
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
            if not parse_annotation(file_info, line_number, raw_annotation):
                parse_success = False
    
    if not parse_success:
        return None
    
    if not file_info.should_be_valid() and not any([rep.report_type == ReportType.ERROR for rep in file_info.expected_reports]):
        logger.error("A non-valid test file (i.e. filename not starting with the prefix 'valid') should have at least one ERROR directive in its annotations.")
        return None
            
    return file_info

# format: [file:line:col] [category] message...
# since Julia doesn't provide column information, it is simply dropped here
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
        
def collect_reports(file_info: FileInfo, config: Config) -> list[Report] | ExitCode:
    should_be_valid = file_info.should_be_valid()
    only_expects_missing_errors = all([rep.expected_missing for rep in file_info.expected_reports if rep.report_type == ReportType.ERROR])
    
    stc_proc_env = os.environ.copy()
    stc_proc_env["NO_COLOR"] = "1"
    if config.julia_bin_dir is not None:
        stc_proc_env["PATH"] = config.julia_bin_dir + os.pathsep + stc_proc_env.get("PATH", "")
    
    cli_args = ["--no-benchmark", "--quiet", "--cg-indent", "0"]
    if should_be_valid:
        assert file_info.out_path != ""
        cli_args.append("-o")
        cli_args.append(file_info.out_path)
    else:
        cli_args.append("--no-out")

    stc_proc_result = None
    try:
        stc_proc_result = subprocess.run([config.cli, file_info.path, *cli_args], env=stc_proc_env, timeout=SUBPROCESS_TIMEOUT_SEC,
                                         stderr=subprocess.PIPE, text=True, encoding="utf-8", errors="replace")
    except subprocess.TimeoutExpired as e:
        stderr_output = parse_subproc_output(e.stderr)
        tail = f"see output below:\n== STC OUTPUT BEGIN ==\n{stderr_output}\n== STC OUTPUT END ==" if stderr_output != "" else "no output was captured"
        logger.error(f"stc timed out after {e.timeout} seconds, {tail}")
        return ExitCode.TRANSPILER_ERROR

    def log_error_with_stc_output(msg: str):
        logger.error(f"{msg}\nSee stc stderr output for details:")
        print(f"== STC OUTPUT BEGIN ==\n{stc_proc_result.stderr}\n== STC OUTPUT END ==", file=sys.stderr)
     
    if stc_proc_result.returncode not in [0, 1]:
        log_error_with_stc_output(f"Unexpected stc exit code: {stc_proc_result.returncode}")
        if stc_proc_result.returncode == 0xC0000135:
            logger.error(f"Hint: based on the exit code, this is a DLL not found error. The most likely culprit is libjulia not being present in PATH while it's also not set correctly through --julia-bin-dir.")
        return ExitCode.TRANSPILER_ERROR

    collected_error_reports = False
    reports: list[Report] = []
    for line in stc_proc_result.stderr.splitlines():
        if line.startswith("[Internal Error]"): # --allow-internal-errors does not affect this, since this is specifically for internal errors with no source location info
            log_error_with_stc_output("Internal error without source location information in transpiler output")
            return ExitCode.TESTS_FAILED
        
        maybe_report = try_parse_report(line)
        if maybe_report is not None:
            reports.append(maybe_report)
            if maybe_report.report_type in [ReportType.ERROR, ReportType.INTERNAL_ERROR]:
                collected_error_reports = True
                
    if stc_proc_result.returncode == 0:
        config.stc_success = True
    
    # TODO: separate these check from report collection (and use TestResult instead of ExitCode)
    if should_be_valid:
        base_log = "Check that file named 'valid*' transpiles successfully ---"
        if stc_proc_result.returncode != 0:
            if file_info.expect_transpiler_failure:
                if file_info.transpiler_failure_pattern == "" or stc_proc_result.stderr.find(file_info.transpiler_failure_pattern) != -1:
                    logger.info(f"{base_log} skipped (marked missing)")
                else:
                    logger.error(f"{base_log} failed (marked missing, but output did not match the provided pattern)")
                    return ExitCode.TESTS_FAILED
            else:
                logger.error(f"{base_log} failed")
                return ExitCode.TESTS_FAILED
        else:
            if file_info.expect_transpiler_failure:
                logger.warning(f"Transpilation was marked as an expected failure (annotated with 'STC(missing)'), but stc reported no errors.")
            logger.info(f"{base_log} passed")
    
    if not should_be_valid and not only_expects_missing_errors and stc_proc_result.returncode == 0:
        logger.error("Transpilation succeeded for non-valid test file. " +
                     "Test files that are expected to transpile successfully should be named with the prefix 'valid'. ")
        return ExitCode.TESTS_FAILED

    # check that at least one of the missing cases was hit (otherwise the transpiler failed for an unrelated reason)    
    if not should_be_valid and only_expects_missing_errors and stc_proc_result.returncode != 0:
        missing_case_hit = False
        for (expected, actual) in ((e, a) for e in file_info.expected_reports for a in reports):
            if expected.report_type != ReportType.ERROR:
                continue

            assert expected.expected_missing
            if report_matches(expected, actual, config):
                missing_case_hit = True
                break
            
        if not missing_case_hit:
            logger.error(f"Transpilation failed (with exit code {stc_proc_result.returncode}) for test file containing only missing ERROR directives, but none of them were the reason for failure.")
            return ExitCode.TESTS_FAILED

    if collected_error_reports and stc_proc_result.returncode == 0:
        log_error_with_stc_output("Transpilation succeeded after transpilation produced error reports.")
        return ExitCode.TRANSPILER_ERROR
    
    if not collected_error_reports and stc_proc_result.returncode != 0:
        log_error_with_stc_output(f"Transpilation failed (with exit code {stc_proc_result.returncode}), but no error reports were found")
        return ExitCode.TRANSPILER_ERROR
        
    return reports

def report_matches(expected: ExpectedReport, actual: Report, config: Config) -> bool:
    if expected.line_number != actual.line_number:
        return False

    message_check = config.skip_message_checks or expected.message is None or actual.message.find(expected.message) != -1
    report_type_check = expected.report_type == actual.report_type or (config.allow_internal_errors and expected.report_type == ReportType.ERROR and actual.report_type == ReportType.INTERNAL_ERROR)
    return message_check and report_type_check

def validate_reports(file_info: FileInfo, actual_reports: list[Report], config: Config) -> TestResult:
    expected_reports = file_info.expected_reports
    
    # validation is done "inefficiently", in several passes, to allow all errors to be printed at once in case of failure
    # it also assumed that actual and expected report count is small enough for O(n*m) to not matter here
    
    used_skips = False
    
    internal_errors_check = True
    internal_used_skips = False
    for actual in actual_reports:
        if actual.report_type == ReportType.INTERNAL_ERROR:
            base_log = f"Internal error on line {actual.line_number} ---"
            if not config.allow_internal_errors:
                logger.error(f"{base_log} fail")
                internal_errors_check = False
            else:
                logger.debug(f"{base_log} ignored")
                internal_used_skips = True
                used_skips = True
    
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
        found = False
        for actual in actual_reports:
            if expected.line_number == actual.line_number:
                if report_matches(expected, actual, config):
                    found = True
                    break
                
                same_line_reports.append(actual)

        if found:
            logger.info(f"{base_log} passed")
            if expected.expected_missing:
                logger.warning(f"Previous case was marked as an expected failure (annotated with the 'missing' argument), but a matching report was found.")
            continue
        
        if expected.expected_missing:
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
    if config.strict():
        # we do an extra iteration here to report failures properly, a simple success = success and len(actual_reports) == len(expected_reports) would suffice otherwise
        
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
    else:
        logger.info(f"{SUMMARY_PREFIX} Strict checks skipped")
    logger.info("")
    
    if not internal_errors_check:
        assert not config.allow_internal_errors
        add_partially = not lenient_check or not strict_check
        logger.info(f"Test failed{" partially" if add_partially else ""} because internal error report(s) were generated, run with --allow-internal-errors to treat these as regular errors for a temporary workaround")        
    
    result = TestResult.PASSED if internal_errors_check and lenient_check and strict_check else TestResult.FAILED
    if result == TestResult.PASSED and used_skips:
        result = TestResult.PASSED_WITH_SKIPS
    
    return result

def validate_glsl(file_info: FileInfo, config: Config) -> TestResult | ExitCode:
    # only valid shaders can be GLSL validated
    if not file_info.should_be_valid():
        return TestResult.PASSED
    
    if config.glslang_validator is None:
        return TestResult.PASSED_WITH_SKIPS
    
    base_log = "Run glslangValidator on generated code ---"
    
    if not config.stc_success:
        logger.info(f"{base_log} skipped (transpilation failed)")
        return TestResult.PASSED_WITH_SKIPS
    
    try:
        proc_result = subprocess.run([config.glslang_validator, "-l", file_info.out_path], text=True, encoding="utf-8", errors="replace",
                                     timeout=SUBPROCESS_TIMEOUT_SEC, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired as e:
        stdout_output = parse_subproc_output(e.stdout)
        tail = f"see output below:\n== GLSLANG VALIDATOR OUTPUT BEGIN ==\n{stdout_output}\n== GLSLANG VALIDATOR OUTPUT END ==" if stdout_output != "" else "no output was captured"
        logger.error(f"glslangValidator timed out after {e.timeout} seconds, {tail}")
        return ExitCode.GLSLANG_VALIDATOR_ERROR
    
    if proc_result.returncode == 0:
        logger.info(f"{base_log} passed")
        if file_info.expect_glsl_failure:
            logger.warning(f"GLSL validation was marked as an expected failure (annotated with 'GLSL(missing)'), but glslangValidator reported no errors.")
        return TestResult.PASSED
    
    def dump_glslang_validator_output():
        logger.info(f"See glslangValidator output for details:\n" +
                    "== GLSLANG VALIDATOR OUTPUT BEGIN ==\n" + 
                    proc_result.stdout +
                    "\n== GLSLANG VALIDATOR OUTPUT END ==")
    
    if proc_result.returncode in GLSLANG_VALIDATOR_REJECTED_CODES:
        if file_info.expect_glsl_failure:
            matched_pattern = (file_info.glsl_failure_pattern == "" or proc_result.stdout.find(file_info.glsl_failure_pattern) != -1)
            if matched_pattern:
                logger.info(f"{base_log} skipped (marked missing)")
                return TestResult.PASSED_WITH_SKIPS
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
    arg_parser.add_argument("--path", required=True, help="path of the conformance test file to run")
    arg_parser.add_argument("--cli", required=True, help="path of the stc CLI executable (or wrapper script) to use for running the tests")
    arg_parser.add_argument("--mode", choices=("strict", "lenient"), default="lenient", help="lenient mode requires all report annotations to have a matching report generated to pass, strict mode additionally requires all error reports to have a matching report annotation as well (default: lenient)")
    arg_parser.add_argument("--julia-bin-dir", help="path of the installed Julia runtime's bin directory, prepended to the PATH of the subprocess' environment that runs stc (should only be needed on Windows)")
    arg_parser.add_argument("--glslang-validator", help="path of the installed glslangValidator executable, which should be used for running the GLSL validation tests (default: try to find through PATH)")
    arg_parser.add_argument("--allow-internal-errors", action="store_true", help="if enabled, internal error reports will be treated as regular errors when trying to match them to report annotations")
    arg_parser.add_argument("--skip-message-checks", action="store_true", help="if enabled, message pattern checks will pass unconditionally")
    arg_parser.add_argument("--verbose", action="store_true", help="print DEBUG messages too")
    args = arg_parser.parse_args()
    
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO, format="%(message)s")
    
    tmp_dir = tempfile.TemporaryDirectory(prefix="conformance_output_")
    tmp_dir_path = str(Path(tmp_dir.name).resolve())
    
    config = Config(args.path, args.cli, ScriptMode.parse(args.mode), args.allow_internal_errors, args.skip_message_checks,
                    tmp_dir_path, args.julia_bin_dir, args.glslang_validator)
    
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
    
    file_info = parse_file(config)
    if file_info is None:
        logger.error("Failed to parse test file")
        return ExitCode.TESTS_MALFORMED
    
    if file_info.should_be_valid():
        out_path = Path(config.tmp_dir_path) / f"out.{file_info.shader_stage}"
        file_info.out_path = str(out_path)
    
    maybe_reports: list[Report] | ExitCode = collect_reports(file_info, config)
    if isinstance(maybe_reports, ExitCode):
        logger.error("Failed to collect transpiler reports")
        return maybe_reports
    
    result = validate_reports(file_info, maybe_reports, config)
    
    if file_info.should_be_valid():
        glsl_result = validate_glsl(file_info, config)
        if isinstance(glsl_result, ExitCode):
            logger.error("Failed to run glslangValidator reference tests")
            return glsl_result
        
        assert isinstance(glsl_result, TestResult)
        result = TestResult.merge(result, glsl_result)
        
        match glsl_result:
            case TestResult.PASSED:            logger.info(f"{SUMMARY_PREFIX} GLSL checks passed")
            case TestResult.PASSED_WITH_SKIPS: logger.info(f"{SUMMARY_PREFIX} GLSL checks passed (with skips)")
            case TestResult.FAILED:            logger.error(f"{SUMMARY_PREFIX} GLSL checks failed")
            case _:
                logger.error(f"{SUMMARY_PREFIX} GLSL check status unknown")
                return ExitCode.UNEXPECTED_ERROR
    else:
        logger.info(f"{SUMMARY_PREFIX} GLSL checks skipped")
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
    sys.exit(main().value)
