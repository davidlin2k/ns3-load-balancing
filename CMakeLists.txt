check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/new-module-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME new-module
    SOURCE_FILES model/new-module.cc
                 helper/new-module-helper.cc
    HEADER_FILES model/new-module.h
                 helper/new-module-helper.h
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES test/new-module-test-suite.cc
                 ${examples_as_tests_sources}
)
