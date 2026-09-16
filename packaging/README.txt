Shader Transpiler Core (STC)

REQUIREMENTS
  A Julia installation, with `julia` reachable from your PATH. The transpiler
  embeds the Julia runtime, so it will not run without one.

USAGE
  Linux:    bin/stc.sh  <source_file> [options]
  Windows:  bin\stc.bat <source_file> [options]

  Use the wrappers, not the bin/stc executable directly. The wrapper asks Julia
  where its shared libraries live and puts that directory on the loader path
  before starting the transpiler. Invoking bin/stc yourself will fail with a
  message about libjulia not being found, unless your Julia was installed in a
  way that already places it on the loader path.

  To skip the lookup step, set STC_JULIA_LIBDIR to the directory containing the
  Julia shared library: <julia>/lib on Linux, <julia>/bin on Windows.

CONTENTS
  bin/  the transpiler CLI and its wrappers. On Windows this also holds
        libstc.dll, which has to sit beside the executable to be found.

  lib/  on Linux, libstc itself. On Windows, only stc.lib, the import library
        needed to link against the DLL. No headers are shipped: the C API is
        ABI-only for now, so a C consumer declares the entry points itself and
        links against this file.
