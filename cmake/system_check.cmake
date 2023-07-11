##
## Author: Hank Anderson <hank@statease.com>
## Description: Ported from the OpenBLAS/c_check perl script.
##              This is triggered by prebuild.cmake and runs before any of the code is built.
##              Creates config.h and Makefile.conf.

# Convert CMake vars into the format that OpenBLAS expects
string(TOUPPER ${CMAKE_SYSTEM_NAME} HOST_OS)
if (${HOST_OS} STREQUAL "WINDOWS")
  set(HOST_OS WINNT)
endif ()

if (${HOST_OS} STREQUAL "LINUX")
# check if we're building natively on Android (TERMUX)
    EXECUTE_PROCESS( COMMAND uname -o COMMAND tr -d '\n' OUTPUT_VARIABLE OPERATING_SYSTEM)
      if(${OPERATING_SYSTEM} MATCHES "Android")
        set(HOST_OS ANDROID)
      endif()
endif()



if(MINGW)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine
              OUTPUT_VARIABLE OPENBLAS_MINGW_TARGET_MACHINE
              OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(OPENBLAS_MINGW_TARGET_MACHINE MATCHES "amd64|x86_64|AMD64")
      set(MINGW64 1)
    endif()
endif()

# Pretty thorough determination of arch. Add more if needed
if(CMAKE_CL_64 OR MINGW64)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)")
    set(ARM64 1)
  else()
    set(X86_64 1)
  endif()
elseif(MINGW OR (MSVC AND NOT CMAKE_CROSSCOMPILING))
  set(X86 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc.*|power.*|Power.*")
  set(POWER 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "mips64.*")
  set(MIPS64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "loongarch64.*")
  set(LOONGARCH64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64.*")
  set(RISCV64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
  if (NOT BINARY)
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
      set(X86_64 1)
    else()
      set(X86 1)
    endif()
  else()
    if (${BINARY} EQUAL "64")
       set(X86_64 1)
    else ()
       set(X86 1)
    endif()
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*|amd64.*|AMD64.*")
  set(X86 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)")
  if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(ARM64 1)
  else()
    set(ARM 1)
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
  set(ARM 1)
elseif (${CMAKE_CROSSCOMPILING})
   if (${TARGET} STREQUAL "CORE2")
    if (NOT BINARY)
       set(X86 1)
    elseif (${BINARY} EQUAL "64")
       set(X86_64 1)
    else ()
       set(X86 1)
    endif()
   elseif (${TARGET} STREQUAL "P5600" OR ${TARGET} MATCHES "MIPS.*")
       set(MIPS32 1)
   elseif (${TARGET} STREQUAL "ARMV7")
       set(ARM 1)
   else()
       set(ARM64 1)
   endif ()
else ()
   message(WARNING "Target ARCH could not be determined, got \"${CMAKE_SYSTEM_PROCESSOR}\"")
endif()

if (X86_64)
  set(ARCH "x86_64")
elseif(X86)
  set(ARCH "x86")
elseif(POWER)
  set(ARCH "power")
elseif(MIPS32)
  set(ARCH "mips")
elseif(MIPS64)
  set(ARCH "mips64")
elseif(ARM)
  set(ARCH "arm")
elseif(ARM64)
  set(ARCH "arm64")
else()
  set(ARCH ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "Target Architecture")
endif ()

if (NOT BINARY)
  if (X86_64 OR ARM64 OR POWER OR MIPS64 OR LOONGARCH64 OR RISCV64)
    set(BINARY 64)
  else ()
    set(BINARY 32)
  endif ()
endif()

if(BINARY EQUAL 64)
  set(BINARY64 1)
else()
  set(BINARY32 1)
endif()

if (X86_64 OR X86)
if (NOT NO_AVX512)
  file(WRITE ${PROJECT_BINARY_DIR}/avx512.c "#include <immintrin.h>\n\nint main(void){ __asm__ volatile(\"vbroadcastss -4 * 4(%rsi), %zmm2\"); }")
execute_process(COMMAND ${CMAKE_C_COMPILER} -march=skylake-avx512 -c -v -o ${PROJECT_BINARY_DIR}/avx512.o ${PROJECT_BINARY_DIR}/avx512.c OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE NO_AVX512)
if (NO_AVX512 EQUAL 1)
set (CCOMMON_OPT "${CCOMMON_OPT} -DNO_AVX512")
endif()
  file(REMOVE "avx512.c" "avx512.o")
endif()
endif()

include(CheckIncludeFile)
CHECK_INCLUDE_FILE("stdatomic.h" HAVE_C11)
if (HAVE_C11 EQUAL 1)
set (CCOMMON_OPT "${CCOMMON_OPT} -DHAVE_C11")
endif()
