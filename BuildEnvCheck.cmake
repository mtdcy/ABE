cmake_minimum_required (VERSION 2.8)

# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
  OUTPUT_VARIABLE GIT_HEAD
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

#execute_process(
#  COMMAND cat VERSION 
#  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#  OUTPUT_VARIABLE VERSION
#  OUTPUT_STRIP_TRAILING_WHITESPACE
#)

set(PROJECT     ${PROJECT_NAME})
set(PATH        ${CMAKE_SOURCE_DIR})
set(TARGET_ARCH ${BUILD})

# http://www.itk.org/Wiki/CMake:How_To_Write_Platform_Checks
include (CheckIncludeFiles)
include (CheckFunctionExists)
include (CheckLibraryExists)

check_include_files (malloc.h HAVE_MALLOC_H)
check_include_files (unistd.h HAVE_UNISTD_H)
check_include_files (android/log.h HAVE_ANDROID_LOG_H)
check_include_files (pthread.h HAVE_PTHREAD_H)
check_include_files (sys/prctl.h HAVE_SYS_PRCTL_H)
check_include_files (sys/time.h HAVE_SYS_TIME_H)

check_function_exists (gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists (posix_memalign HAVE_POSIX_MEMALIGN)
check_function_exists (memalign HAVE_MEMALIGN)
check_function_exists (lseek64 HAVE_LSEEK64)
check_function_exists (strdup HAVE_STRDUP)
check_function_exists (strndup HAVE_STRNDUP)
check_function_exists (nanosleep HAVE_NANOSLEEP)

if (BUILD MATCHES linux)
    check_library_exists(rt clock_gettime time.h HAVE_CLOCK_GETTIME)
else()
    check_function_exists (clock_gettime HAVE_CLOCK_GETTIME)
endif()

check_library_exists (pthread pthread_cond_timedwait_relative_np pthread.h HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE)
check_library_exists (pthread pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP)
check_library_exists (pthread pthread_getthreadid_np pthread.h HAVE_PTHREAD_GETTHREADID_NP)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

