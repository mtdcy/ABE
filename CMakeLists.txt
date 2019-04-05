cmake_minimum_required (VERSION 2.8)

project (ABE)

# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
add_definitions(-Wall)
#add_definitions(-O2)
if (MSVC)
else()
add_definitions(-Wno-unused-function)
add_definitions(-Wno-unused-variable)
add_definitions(-Werror=non-virtual-dtor)
add_definitions(-Werror=delete-non-virtual-dtor)
add_definitions(-fvisibility=hidden)
#add_definitions(-Wextra)
#add_definitions (-Wno-multichar)
#add_definitions (-Wno-switch)
set (CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} -std=c++98")
set (CMAKE_C_FLAGS      "${CMAKE_C_FLAGS} -std=c99")
endif()

add_definitions(-D_POSIX_C_SOURCE=200809L)
add_definitions(-D_ISOC99_SOURCE)
add_definitions(-D_FORTIFY_SOURCE=2)    # https://access.redhat.com/blogs/766093/posts/1976213
add_definitions(-DHAVE_CONFIG_H)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG_MALLOC)
endif()

if (APPLE)
    set (CMAKE_MACOSX_RPATH TRUE)
endif()

if ("${CMAKE_GENERATOR}" STREQUAL "Xcode")
    #set (CMAKE_MACOSX_BUNDLE TRUE)
    set (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set (CMAKE_XCODE_ATTRIBUTE_SIGN_IDENTITY "macOS Developer")
    #set (CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 10.8)
endif()

# detect target OS
include (BuildEnvCheck.cmake)

set(SOURCE_FILES
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

    ABE/object/Message.cpp
    ABE/object/Buffer.cpp
    ABE/object/protocol/File.cpp
    ABE/object/Content.cpp
    ABE/object/Looper.cpp
    ABE/object/Event.cpp

    ABE/tools/Bits.cpp
    ABE/tools/Mutex.cpp
    ABE/tools/Thread.cpp

    ABE/stl/List.cpp
    ABE/stl/Vector.cpp
    ABE/stl/HashTable.cpp
    ABE/stl/Queue.cpp
    )

file (GLOB PUBLIC_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "ABE/*.h")

file (GLOB BASIC_PUBLIC_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "ABE/basic/*.h")
file (GLOB OBJECT_PUBLIC_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "ABE/object/*.h")
file (GLOB TOOLS_PUBLIC_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "ABE/tools/*.h")
file (GLOB STL_PUBLIC_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "ABE/stl/*.h")

add_library (${PROJECT_NAME} SHARED
    ${SOURCE_FILES}
    ${PUBLIC_HEADERS}
    ${BASIC_PUBLIC_HEADERS}
    ${OBJECT_PUBLIC_HEADERS}
    ${TOOLS_PUBLIC_HEADERS}
    ${STL_PUBLIC_HEADERS}
    )

# includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ABE)
include_directories(${CMAKE_CURRENT_BINARY_DIR})    # Config.h

if ("${CMAKE_GENERATOR}" STREQUAL "Xcode")
    # install headers nested
    set_property(SOURCE ${BASIC_PUBLIC_HEADERS}
        PROPERTY MACOSX_PACKAGE_LOCATION Headers/basic)
    set_property(SOURCE ${OBJECT_PUBLIC_HEADERS}
        PROPERTY MACOSX_PACKAGE_LOCATION Headers/object)
    set_property(SOURCE ${TOOLS_PUBLIC_HEADERS}
        PROPERTY MACOSX_PACKAGE_LOCATION Headers/tools)
    set_property(SOURCE ${STL_PUBLIC_HEADERS}
        PROPERTY MACOSX_PACKAGE_LOCATION Headers/stl)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        PUBLIC_HEADER "${PUBLIC_HEADERS}"   # need this to create Headers link in framework
        MACOSX_FRAMEWORK_IDENTIFIER com.mtdcy.media.toolkit
        #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
        )
else()
    install (FILES ${PUBLIC_HEADERS} DESTINATION include/ABE)
    install (FILES ${BASIC_PUBLIC_HEADERS} DESTINATION include/ABE/basic)
    install (FILES ${OBJECT_PUBLIC_HEADERS} DESTINATION include/ABE/object)
    install (FILES ${TOOLS_PUBLIC_HEADERS} DESTINATION include/ABE/tools)
    install (FILES ${STL_PUBLIC_HEADERS} DESTINATION include/ABE/stl)
endif()

install (TARGETS ${PROJECT_NAME}
    FRAMEWORK DESTINATION Library/Frameworks
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    )

# test
remove_definitions(-Werror=non-virtual-dtor)

set (BUILD_SHARED_LIBS OFF)
add_subdirectory(gtest-1.7.0)
include_directories(${gtest_SOURCE_DIR}/include)

add_executable (test test.cpp)
target_link_libraries(test ${PROJECT_NAME})
target_link_libraries(test gtest gtest_main)

add_executable(test_c test_c.c)
target_link_libraries(test_c ${PROJECT_NAME})

add_executable(perf perf.cpp)
target_link_libraries(perf ${PROJECT_NAME})
set_target_properties(perf PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    EXCLUDE_FROM_DEFAULT_BUILD TRUE)
