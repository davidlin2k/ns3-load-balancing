build_lib_example(
    NAME new-module-example
    SOURCE_FILES new-module-example.cc
    LIBRARIES_TO_LINK ${libload-balancing}
                      ${libapplications}
                      ${libinternet}
                      ${libpoint-to-point}
                      ${libflow-monitor}
                      ${libnetanim}
)
