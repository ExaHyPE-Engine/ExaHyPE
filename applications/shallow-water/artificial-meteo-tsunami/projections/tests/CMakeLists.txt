cmake_minimum_required(VERSION 3.10)
project(AlbersTest)

set(CMAKE_CXX_STANDARD     20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS  OFF)

# 1) Pfade definieren
# --------------------
# a) Test-Quellordner: .../albersEqualArea/test
set(TEST_SRC_DIR    ${CMAKE_CURRENT_SOURCE_DIR})

# b) albersEqualArea-Ordner: enthält zuTesten.cpp
set(AEA_DIR         ${TEST_SRC_DIR}/..)

# c) Stub-Header
set(STUB_HEADER     ${TEST_SRC_DIR}/stubs/Constants.h)

# d) Kleiner Build-Ordner für die kopierten Test-Quellen
set(TEST_BUILD_DIR  ${CMAKE_BINARY_DIR}/test_sources)

# 2) Build-Quell-Verzeichnis aufräumen und anlegen
# ------------------------------------------------
file(REMOVE_RECURSE ${TEST_BUILD_DIR})
file(MAKE_DIRECTORY  ${TEST_BUILD_DIR}/albersEqualArea/test)

# 3) Nur die Dateien kopieren, die wir brauchen
# ------------------------------------------------
# a) zuTesten.cpp
file(COPY
  ${AEA_DIR}/EllipsoidProjection.h
  DESTINATION ${TEST_BUILD_DIR}/albersEqualArea
)

file(COPY
  ${AEA_DIR}/SphereProjection.h
  DESTINATION ${TEST_BUILD_DIR}/albersEqualArea
)

# b) WrapperC.cpp
file(COPY
  ${TEST_SRC_DIR}/WrapperC.cpp
  DESTINATION ${TEST_BUILD_DIR}/albersEqualArea/test
)

# c) Stub Constants.h dorthin, wo WrapperC "../Constants.h" finden wird:
#    -> test/WrapperC.cpp inklusive "../Constants.h" → albersEqualArea/Constants.h
file(COPY
  ${STUB_HEADER}
  DESTINATION ${TEST_BUILD_DIR}/
)

# 4) Shared Library aus den kopierten Dateien bauen
# ------------------------------------------------
add_library(mylib SHARED
  ${TEST_BUILD_DIR}/albersEqualArea/test/WrapperC.cpp
)

# 5) Für relative Includes in WrapperC.cpp brauchen wir keinen -I:
#    "#include \"../Constants.h\"" löst bereits zu
#      test_sources/albersEqualArea/test/../Constants.h
#      → test_sources/albersEqualArea/Constants.h
#    Trotzdem: falls zuTesten.cpp weitere Headers braucht, kannst Du sie hier hinzufügen.
# target_include_directories(mylib PRIVATE <weitere-pfade>)

# 6) Compiler-Optimierung
target_compile_options(mylib PRIVATE -O2)
