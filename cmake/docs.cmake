# Doxygen (+ Graphviz) docs setup

# needs: STC_SRC_DIR, STC_BUILD_DIR defined
# adds targets: docs, docs_pdf
#   - docs target generates docs as HTML
#   - docs_pdf generates LaTeX, and compiles to PDF if pdflatex is available

find_package(Doxygen COMPONENTS dot)

if (Doxygen_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY "${STC_BUILD_DIR}/docs")

    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_RECURSIVE YES)

    set(DOXYGEN_HAVE_DOT YES)
    set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    set(DOXYGEN_INTERACTIVE_SVG YES)
    set(DOXYGEN_DOT_TRANSPARENT YES)

    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_EXTRACT_PRIVATE NO)
    set(DOXYGEN_EXTRACT_ANON_NSPACES NO)

    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_MACRO_EXPANSION YES)

    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_INLINE_SOURCES NO)
    set(DOXYGEN_SHOW_FILES YES)
    set(DOXYGEN_SHOW_USED_FILES YES)

    set(DOXYGEN_EXCLUDE_PATTERNS
        "*.def"
        "*.inc"
        "*/test/*"
        "*/sandbox/*"
        "*/examples/*"
        "*/cli/*"
    )

    set(DOXYGEN_UML_LOOK YES)
    set(DOXYGEN_TEMPLATE_RELATIONS YES)

    set(DOXYGEN_CALL_GRAPH NO)
    set(DOXYGEN_CALLER_GRAPH NO)

    set(DOXYGEN_CLASS_GRAPH YES)
    set(DOXYGEN_COLLABORATION_GRAPH YES)
    set(DOXYGEN_DIRECTORY_GRAPH YES)
    set(DOXYGEN_INCLUDE_GRAPH YES)
    set(DOXYGEN_INCLUDED_BY_GRAPH YES)

    doxygen_add_docs(
        docs
        ${STC_SRC_DIR}/include
        ${STC_SRC_DIR}/src
        COMMENT "Generating HTML docs with doxygen..."
    )

    message(STATUS "doxygen and graphviz found, docs target enabled")

    find_program(PDFLATEX_COMPILER pdflatex)

    if (PDFLATEX_COMPILER)
        set(DOXYGEN_GENERATE_HTML NO)
        set(DOXYGEN_GENERATE_LATEX YES)
        set(DOXYGEN_USE_PDFLATEX YES)
        set(DOXYGEN_PDF_HYPERLINKS YES)

        set(DOXYGEN_SOURCE_BROWSER NO)
        set(DOXYGEN_INLINE_SOURCES NO)
        set(DOXYGEN_SHOW_FILES NO)
        set(DOXYGEN_SHOW_USED_FILES NO)
        set(DOXYGEN_DIRECTORY_GRAPH NO)

        set(DOXYGEN_ALPHABETICAL_INDEX NO)
        set(DOXYGEN_SHOW_NAMESPACES NO)

        set(DOXYGEN_EXTRACT_ALL NO)
        set(DOXYGEN_HIDE_UNDOC_CLASSES YES)
        set(DOXYGEN_HIDE_UNDOC_MEMBERS YES)

        set(DOXYGEN_EXCLUDE_SYMBOLS "std::hash*" "*JuliaSymbolCache" "*JuliaTypeCache")

        if (WIN32)
            set(MAKE_CMD "make.bat")
        else()
            set(MAKE_CMD "make")
        endif()

        doxygen_add_docs(
            docs_pdf
            ${STC_SRC_DIR}/include
            COMMENT "Generating LaTeX docs with doxygen..."
        )

        add_custom_command(TARGET docs_pdf POST_BUILD
            COMMAND ${MAKE_CMD}
            WORKING_DIRECTORY "${STC_BUILD_DIR}/docs/latex"
            COMMENT "Compiling LaTeX docs to PDF..."
            VERBATIM
        )

        message(STATUS "pdflatex found, LaTeX -> PDF docs generation enabled for docs_pdf target")
    else()
        message(STATUS "pdflatex could not be located, PDF generation from LaTeX docs won't be possible.")
    endif()
else()
    message(STATUS "doxygen or graphviz could not be located, docs generation won't be possible.")
endif()
