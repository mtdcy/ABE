cmake_minimum_required (VERSION 2.8)

project (ABE)

set (LIBNAME ${PROJECT_NAME}    CACHE STRING    "Library Name")

# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
add_definitions(-Wall)
#add_definitions(-O2)
if (MSVC)
else()
add_definitions(-Wno-unused-function)
add_definitions(-Wno-unused-variable)
add_definitions(-Wno-unused-value)
add_definitions(-Werror=non-virtual-dtor)
add_definitions(-Werror=delete-non-virtual-dtor)
add_definitions(-fvisibility=hidden)
#add_definitions(-Wextra)
#add_definitions (-Wno-multichar)
#add_definitions (-Wno-switch)
set (CMAKE_C_STANDARD   99)     # c98
set (CMAKE_C_EXTENSIONS OFF)    # no gnu extensions
set (CMAKE_CXX_STANDARD 98)     # c++98
set (CMAKE_CXX_EXTENSIONS OFF)  # no gnu extensions
# gnucc definitions
if (CMAKE_COMPILER_IS_GNUCC)
add_definitions(-Wno-attributes)
add_definitions(-fPIC)
endif()
endif()

add_definitions(-D_POSIX_C_SOURCE=200809L)
add_definitions(-D_ISOC99_SOURCE)
add_definitions(-D_FORTIFY_SOURCE=2)    # https://access.redhat.com/blogs/766093/posts/1976213
if (CMAKE_BUILD_TYPE MATCHES Debug AND APPLE)   # malloc.cpp is ready only in APPLE
    add_definitions(-DDEBUG_MALLOC)
endif()

if (APPLE)
    set (CMAKE_MACOSX_RPATH TRUE)
elseif(WIN32)
    add_definitions(-DBUILD_ABE_DLL)
endif()

if (XCODE)
    #set (CMAKE_MACOSX_BUNDLE TRUE)
    set (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
endif()

# source files
set(ABE_SOURCES
    ABE/basic/debug/backtrace.c
    ABE/basic/debug/malloc.cpp
    ABE/basic/CallStack.cpp
    ABE/basic/Log.cpp
    ABE/basic/Time.cpp
    ABE/basic/Hardware.cpp
    ABE/basic/SharedObject.cpp
    ABE/basic/Allocator.cpp
    ABE/basic/private/ConvertUTF.c
    ABE/basic/String.cpp
    ABE/basic/Mutex.cpp
    ABE/basic/Thread.cpp

    ABE/object/Message.cpp
    ABE/object/Buffer.cpp
    ABE/object/protocol/File.cpp
    ABE/object/Content.cpp
    ABE/object/Looper.cpp
    ABE/object/Event.cpp

    ABE/tools/Bits.cpp

    ABE/stl/List.cpp
    ABE/stl/Vector.cpp
    ABE/stl/HashTable.cpp
    ABE/stl/Queue.cpp

    ABE/c.cpp
    )

# includes
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/ABE)

# using OBJECT library to avoid build twice for shared & static libraries
add_library (${LIBNAME} OBJECT ${ABE_SOURCES})

if (XCODE)
    add_library (${LIBNAME}_shared SHARED ${ABE_SOURCES})
    add_library (${LIBNAME}_static STATIC ${ABE_SOURCES})
else()
    add_library (${LIBNAME}_shared SHARED $<TARGET_OBJECTS:${LIBNAME}>)
    add_library (${LIBNAME}_static STATIC $<TARGET_OBJECTS:${LIBNAME}>)
endif()

set_target_properties (${LIBNAME}_shared PROPERTIES 
    OUTPUT_NAME ${LIBNAME})
set_target_properties (${LIBNAME}_static PROPERTIES 
    OUTPUT_NAME ${LIBNAME})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # dlopen & pthread
    target_link_libraries(${LIBNAME}_shared -ldl -lpthread)
endif()

#------------------------------------------------------------------------------#
# install 
set (ABE_HEADERS basic object tools stl math)
if (XCODE)
    # public headers, xcode need these
    target_sources (${LIBNAME}_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ABE/ABE.h)
    foreach (dir ${ABE_HEADERS})
        file (GLOB _HEADERS "ABE/${dir}/*.h")
        target_sources (${LIBNAME}_shared PUBLIC ${_HEADERS})
        set_property(SOURCE ${_HEADERS} PROPERTY MACOSX_PACKAGE_LOCATION Headers/${dir})
    endforeach()

    set_target_properties(${LIBNAME}_shared PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        PUBLIC_HEADER "ABE/ABE.h"   # need this to create Headers link in framework
        MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Mac Developer"
        )

    install (TARGETS ${LIBNAME}_shared
        FRAMEWORK       DESTINATION     .
        PUBLIC_HEADER   DESTINATION     Headers
        )
else()
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
# test 
remove_definitions(-Werror=non-virtual-dtor)

add_subdirectory(gtest-1.7.0 EXCLUDE_FROM_ALL)
include_directories(BEFORE ${gtest_SOURCE_DIR}/include)

add_executable (test test.cpp)
target_link_libraries(test ${LIBNAME}_static)
target_link_libraries(test gtest gtest_main)

add_executable(test_c test_c.c)
target_link_libraries(test_c ${LIBNAME}_static)

add_executable(perf perf.cpp)
target_link_libraries(perf ${LIBNAME}_static)
set_target_properties(perf PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    EXCLUDE_FROM_DEFAULT_BUILD TRUE)

add_custom_target(${LIBNAME}_test ALL
    DEPENDS test test_c 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND echo test
    COMMAND ${BASH} test
    COMMAND echo test_c
    COMMAND ${BASH} test_c
    )

add_custom_target(${LIBNAME}_perf_test ALL
    DEPENDS perf
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND echo perf
    COMMAND ${BASH} perf
    )
