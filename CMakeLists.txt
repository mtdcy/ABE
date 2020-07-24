cmake_minimum_required (VERSION 3.14)

# malloc.cpp is ready only in APPLE
option (DEBUG_MALLOC    "DEBUG MALLOC"  OFF)

project (ABE)

set (LIBNAME ${PROJECT_NAME}    CACHE STRING    "Library Name")

# compile options 
include (libcxx.cmake)
# build env check
include (BuildEnvCheck.cmake)

add_definitions(-DBUILD_ABE_DLL)

# version
file (READ ${CMAKE_CURRENT_SOURCE_DIR}/ABE/core/Version.h version)
string (REGEX MATCH "VERSION_MAJOR[ ]*\\(?([0-9]*)\\)?" _ ${version})
set (VERSION_MAJOR  ${CMAKE_MATCH_1})
string (REGEX MATCH "VERSION_MINOR[ ]*\\(?([0-9]*)\\)?" _ ${version})
set (VERSION_MINOR ${CMAKE_MATCH_1})
string (REGEX MATCH "VERSION_PATCH[ ]*\\(?([0-9]*)\\)?" _ ${version})
set (VERSION_PATCH ${CMAKE_MATCH_1})
message ("${PROJECT_NAME} VERSION: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# source files
set(ABE_SOURCES
    ABE/core/Malloc.cpp
    ABE/core/CallStack.cpp
    ABE/core/Log.cpp
    ABE/core/System.cpp
    ABE/core/SharedObject.cpp
    ABE/core/Allocator.cpp
    ABE/core/private/ConvertUTF.c
    ABE/core/String.cpp
    ABE/core/Mutex.cpp
    ABE/core/Message.cpp
    ABE/core/Buffer.cpp
    ABE/core/protocol/File.cpp
    ABE/core/Content.cpp
    ABE/core/Thread.cpp
    ABE/core/Looper.cpp
    ABE/core/Process.cpp

    ABE/stl/List.cpp
    ABE/stl/Vector.cpp
    ABE/stl/HashTable.cpp
    ABE/stl/Queue.cpp

    ABE/ABE.cpp
    )

# includes
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})     # Config.h
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/ABE)

add_libcxx_library (${LIBNAME} ${ABE_SOURCES})

#------------------------------------------------------------------------------#
# install 
set (ABE_HEADERS core stl math)
if (XCODE)
    # public headers, xcode need these
    target_sources (${LIBNAME}_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ABE/ABE.h)
    foreach (dir ${ABE_HEADERS})
        file (GLOB _HEADERS "ABE/${dir}/*.h")
        target_sources (${LIBNAME}_shared PUBLIC ${_HEADERS})
        set_property(SOURCE ${_HEADERS} PROPERTY MACOSX_PACKAGE_LOCATION Headers/${dir})
    endforeach()

    set_target_properties(${LIBNAME}_shared PROPERTIES
        VERSION             "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
        SOVERSION           "${VERSION_MAJOR}.${VERSION_MINOR}"
        PUBLIC_HEADER       "ABE/ABE.h"   # need this to create Headers link in framework
        )

    install (TARGETS ${LIBNAME}_shared
        FRAMEWORK       DESTINATION     .
        PUBLIC_HEADER   DESTINATION     Headers
        )

    # ugly install code
    set (_DEST ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.framework)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.BSD DESTINATION ${_DEST}/Resources)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/RELEASE DESTINATION ${_DEST}/Resources)
else()
    set_target_properties(${LIBNAME}_shared PROPERTIES
        SOVERSION       "${VERSION_MAJOR}.${VERSION_MINOR}"
        VERSION         "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
    )

    install (FILES  ABE/ABE.h   DESTINATION include/ABE)
    foreach (dir ${ABE_HEADERS}) 
        install (DIRECTORY ABE/${dir} 
            DESTINATION include/ABE 
            FILES_MATCHING PATTERN "ABE/${dir}/*.h")
    endforeach()

    install (TARGETS ${LIBNAME}_shared
        PUBLIC_HEADER   DESTINATION     include/ABE
        RUNTIME         DESTINATION     bin
        ARCHIVE         DESTINATION     lib
        LIBRARY         DESTINATION     lib
        )
endif()

#------------------------------------------------------------------------------#
# tools
remove_definitions(-Werror=non-virtual-dtor)

add_libcxx_executable (object EXCLUDE_FROM_ALL object.cpp)
target_link_libraries(object ${LIBNAME}_shared)

add_executable(perf EXCLUDE_FROM_ALL perf.cpp)
target_link_libraries(perf ${LIBNAME}_static)

add_executable (process EXCLUDE_FROM_ALL process_main.cpp)
target_link_libraries (process ${LIBNAME}_static)

#------------------------------------------------------------------------------#
# test 
add_subdirectory(gtest-1.7.0 EXCLUDE_FROM_ALL)
include_directories(BEFORE ${gtest_SOURCE_DIR}/include)

add_executable (test EXCLUDE_FROM_ALL test.cpp)
target_link_libraries (test ${LIBNAME}_static)
target_link_libraries (test gtest gtest_main)

add_libcxx_executable (test_c EXCLUDE_FROM_ALL test_c.c)
target_link_libraries (test_c ${LIBNAME}_static)

add_custom_target(${LIBNAME}_test ALL
    DEPENDS test test_c 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND echo test
    COMMAND ${BASH} test ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND echo test_c
    COMMAND ${BASH} test_c
    )
