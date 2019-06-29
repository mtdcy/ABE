cmake_minimum_required (VERSION 2.8)

# http://www.itk.org/Wiki/CMake:How_To_Write_Platform_Checks
include (CheckIncludeFiles)
include (CheckFunctionExists)
include (CheckLibraryExists)

# pthread check
check_include_files (pthread.h  HAVE_PTHREAD_H)
check_function_exists (gettid   HAVE_GETTID)

check_library_exists (pthread pthread_cond_timedwait_relative_np pthread.h HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP)
check_library_exists (pthread pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP)
check_library_exists (pthread pthread_condattr_setclock pthread.h HAVE_PTHREAD_CONDATTR_SETCLOCK)
check_library_exists (pthread pthread_main_np pthread.h HAVE_PTHREAD_MAIN_NP)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

