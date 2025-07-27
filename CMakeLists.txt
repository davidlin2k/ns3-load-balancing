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
    LIBNAME load-balancing
    SOURCE_FILES model/ipv4-drill-routing-protocol.cc
    HEADER_FILES model/ipv4-drill-routing-protocol.h
    LIBRARIES_TO_LINK ${libcore}
                      ${libinternet}
                      ${libinternet-apps}
                      ${libpoint-to-point}
    TEST_SOURCES test/new-module-test-suite.cc
                 ${examples_as_tests_sources}
)
