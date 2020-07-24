set (CMAKE_C_STANDARD   99)     # c99
set (CMAKE_C_EXTENSIONS OFF)    # no gnu extensions
set (CMAKE_CXX_STANDARD 98)     # c++98
set (CMAKE_CXX_EXTENSIONS OFF)  # no gnu extensions

add_definitions(-fno-rtti)
add_definitions(-fno-exceptions)

# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
add_definitions(-Wall)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-O2)
endif()

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
    # gnucc definitions
    if (CMAKE_COMPILER_IS_GNUCC)
        add_definitions(-Wno-attributes)
        add_definitions(-fPIC)
    endif()
endif()

add_definitions(-D_POSIX_C_SOURCE=200809L)
add_definitions(-D_ISOC99_SOURCE)
add_definitions(-D_FORTIFY_SOURCE=2)    # https://access.redhat.com/blogs/766093/posts/1976213

if (APPLE)
    set (CMAKE_MACOSX_RPATH TRUE)
endif()

if (XCODE)
    #set (CMAKE_MACOSX_BUNDLE TRUE)
    set (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set (CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
endif()

function (add_libcxx_library ...)
    if (XCODE)
        add_library (${ARGV0}_shared SHARED ${ARGN})
        add_library (${ARGV0}_static STATIC ${ARGN})
    else()
        # using OBJECT library to avoid build twice for shared & static libraries
        add_library (${ARGV0}_objects OBJECT ${ARGN})
        add_library (${ARGV0}_shared SHARED $<TARGET_OBJECTS:${ARGV0}_objects>)
        add_library (${ARGV0}_static STATIC $<TARGET_OBJECTS:${ARGV0}_objects>)
    endif()

    # no stdc++
    #if (APPLE)
    #    target_link_options (${ARGV0}_shared PRIVATE -nodefaultlibs -lSystem)
    #else()
    #    target_link_options (${ARGV0}_shared PRIVATE -nodefaultlibs -lc -ldl -lpthread)
    #endif()

    # default properties
    set_target_properties (${ARGV0}_shared ${ARGV0}_static PROPERTIES OUTPUT_NAME ${ARGV0})

    if (XCODE)
        set_target_properties (${ARGV0}_shared PROPERTIES
            FRAMEWORK TRUE
            FRAMEWORK_VERSION   A
            MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Mac Developer"
            )
    endif()
endfunction()

function (add_libcxx_executable ...) 
    add_executable (${ARGV})
    #if (APPLE)
    #    target_link_options (${ARGV0} PUBLIC -nodefaultlibs -lSystem)
    #else()
    #    target_link_options (${ARGV0} PUBLIC -nodefaultlibs -lc -ldl -lpthread)
    #endif()
endfunction()


