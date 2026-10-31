cmake_minimum_required(VERSION 3.20)
project(NWindow_Decomp LANGUAGES C CXX)

# NWindow_Decomp - source reconstruction of the Lineage II C4 (Scions of Destiny) NWindow.dll (stock
# build TimeDateStamp 0x43C73652 = 13/01/2006, md5 afab3ae3fa4f85f7f4c323715e5155e3, MSVC 7.0
# 13.00.9466; forked from the C3 project, itself forked from the C2 and C1 ones). See README.md and
# docs/PLANO_C4.md.
#
# Hook mode (target NWindowDecomp, the one the cycle builds and installs):
#   NWindowDecomp.dll is loaded by the stock NWindow.dll (PE import added by
#   tools/install_test.py) and replaces stock functions one by one.
# Standalone (target NWindowStandalone, phase 5, outside ALL until it links):
#   the same source builds NWindow.dll on its own (docs/notas/C4_Standalone.md).
#
# NWD_GAME_SOURCES: whether src/nwindow (the decompiled game source) is compiled in. OFF (the
# default, kept from the first stage of the port) builds the hook runtime alone (src/re): no function
# is replaced, and the chain (build, import injection, load outside the game, verify) runs on the
# stock DLL. The real builds turn it ON: cmake -S . -B build -A Win32 -DNWD_GAME_SOURCES=ON
# (tools/cycle.py --game-source does it).

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "Win32 only: configure with -A Win32")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # no VC++ redist on clients

option(NWD_GAME_SOURCES "compile src/nwindow (the decompiled game source) into the targets; OFF = src/re only" OFF)
# one hook on the C2 image (tools/smoke/hook_selftest.cpp), to prove the installer while the game source is empty
option(NWD_HOOK_SELFTEST "compile tools/smoke/hook_selftest.cpp into NWindowDecomp (infrastructure self-test)" OFF)

# --- import libraries for the engine DLLs (every export, generated from orig/*.dll) ---------
set(UE_IMPORT_DIR "${CMAKE_BINARY_DIR}/ue_import")
file(MAKE_DIRECTORY "${UE_IMPORT_DIR}")
set(UE_IMPORT_LIBS "")
foreach(dll Core Engine)
  set(_def "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ue_import/${dll}.def")
  set(_lib "${UE_IMPORT_DIR}/${dll}.lib")
  add_custom_command(OUTPUT "${_lib}"
    COMMAND "${CMAKE_AR}" /nologo /machine:x86 "/def:${_def}" "/out:${_lib}"
    DEPENDS "${_def}"
    COMMENT "import library ${dll}.lib")
  list(APPEND UE_IMPORT_LIBS "${_lib}")
endforeach()
add_custom_target(ue_import_libs DEPENDS ${UE_IMPORT_LIBS})

# --- common flags: match the stock ABI ----------------------------------------------------------
#  /Zc:wchar_t-  TCHAR = unsigned short, mangled 'G' like VC7.0 (Core/Engine exports use PBG/PAG)
#  /GR-          no RTTI (stock has none)
#  /EHa          catch(...) also catches SEH, the guard/unguard semantics UE2 had under VC7.0
#  /arch:IA32    x87 floating point like the stock code (same rounding/precision)
#  /Oy-          keep frame pointers: readable stacks in crash dumps
set(NWD_CXX_FLAGS /W3 /utf-8 /Zc:wchar_t- /GR- /EHa /arch:IA32 /Oy- /Zi /MP)
set(NWD_DEFS WIN32 _WINDOWS UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
             _CRT_SECURE_NO_WARNINGS _CRT_NON_CONFORMING_SWPRINTFS)

if(NWD_GAME_SOURCES)
  file(GLOB_RECURSE NWD_GAME_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/nwindow/*.cpp")
  list(LENGTH NWD_GAME_SRC_FILES _n)
  message(STATUS "NWD_GAME_SOURCES=ON: ${_n} game source files from src/nwindow")
  set(NWD_GAME_DEFS NW_GAME_SOURCE=1)
else()
  set(NWD_GAME_SRC_FILES "")
  message(STATUS "NWD_GAME_SOURCES=OFF: src/re only (no game source, no hook)")
  set(NWD_GAME_DEFS NW_GAME_SOURCE=0)
endif()
set(NWD_RE_SOURCES
  src/re/NwHook.cpp
  src/re/NwMain.cpp
  src/re/NwLog.cpp
  src/re/NwMemory.cpp
  src/re/NwDebug.cpp
  src/re/NwCrtCompat.cpp
  src/re/Prologues.gen.cpp
)

if(NWD_HOOK_SELFTEST)
  list(APPEND NWD_RE_SOURCES tools/smoke/hook_selftest.cpp)
endif()

add_library(NWindowDecomp SHARED ${NWD_RE_SOURCES} ${NWD_GAME_SRC_FILES} src/re/NWindowDecomp.rc)
add_dependencies(NWindowDecomp ue_import_libs)
target_include_directories(NWindowDecomp PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(NWindowDecomp PRIVATE ${NWD_DEFS} ${NWD_GAME_DEFS} NW_HOOK_MODE=1)
target_compile_options(NWindowDecomp PRIVATE ${NWD_CXX_FLAGS})
# legacy_stdio_float_rounding.obj: printf rounds an exact half away from zero like the VC7.0 CRT
# ("%.2f" of 12.125 = "12.13"; the UCRT default gives "12.12"), docs/CONVENCOES.md
set(NWD_CRT_COMPAT_OBJS legacy_stdio_float_rounding.obj)
target_link_libraries(NWindowDecomp PRIVATE ${UE_IMPORT_LIBS} user32 imm32 version)
# /MAP: tools/difftest.py --standalone names the function behind each hook through it
# -DNWD_NOICF=ON: no identical-code folding, so tools/port/check_breakers.py can name each stub it finds
option(NWD_NOICF "hook DLL without /OPT:ICF (diagnosis)" OFF)
set(NWD_ICF_OPT /OPT:ICF)
if(NWD_NOICF)
  set(NWD_ICF_OPT /OPT:NOICF)
endif()
target_link_options(NWindowDecomp PRIVATE /DEBUG /OPT:REF ${NWD_ICF_OPT} /MAP ${NWD_CRT_COMPAT_OBJS})
set_target_properties(NWindowDecomp PROPERTIES
  OUTPUT_NAME "NWindowDecomp"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# --- standalone build (phase 5, docs/notas/Standalone.md): the same game source as NWindow.dll on
# its own - NW_HOOK_MODE=0, no hook runtime, our IMPLEMENT_CLASS / IMPLEMENT_PACKAGE
# (src/standalone). Outside ALL: tools/cycle.py builds it in a step of its own, after the hook
# checks, then verifies, difftests and smoke-loads it:
#   cmake --build build --config Release --target NWindowStandalone
# tools/standalone_report.py builds it and lists what the link misses, if anything (C4: stage 5,
# docs/notas/C4_Standalone.md; src/standalone holds the 341 classes and 343 exports of the C4 image). With
# NWD_GAME_SOURCES=OFF there is no game source to link, so these targets configure but do not link.
# Configure with -G "Visual Studio 17 2022" -A Win32 (a plain `cmake -S . -B build` picks x64 and stops above).
find_package(Python3 COMPONENTS Interpreter)
file(GLOB NWD_STANDALONE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/standalone/*.cpp")
# custom features (src/custom, NW_CUSTOM): the standalone build only; the hook target stays faithful
option(NWD_CUSTOM "build src/custom into the standalone DLL (NW_CUSTOM=1)" ON)
set(NWD_CUSTOM_SOURCES)
if(NWD_CUSTOM AND NWD_GAME_SOURCES)
  file(GLOB NWD_CUSTOM_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/custom/*.cpp")
endif()
add_library(nwd_standalone_objs OBJECT EXCLUDE_FROM_ALL
  ${NWD_GAME_SRC_FILES}
  ${NWD_STANDALONE_SOURCES}
  ${NWD_CUSTOM_SOURCES}
  src/re/NwMemory.cpp      # global new/delete through GMalloc, as every UE2 module
  src/re/NwCrtCompat.cpp)  # printf of infinity/NaN like the VC7.0 CRT
target_include_directories(nwd_standalone_objs PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(nwd_standalone_objs PRIVATE ${NWD_DEFS} ${NWD_GAME_DEFS} NW_HOOK_MODE=0)
if(NWD_CUSTOM_SOURCES)
  target_compile_definitions(nwd_standalone_objs PRIVATE NW_CUSTOM=1)
endif()
# /Ob1: only functions marked inline (or defined in the class) are inlined. The stock build kept
# small functions out of line that /Ob2 would fold into their callers (NString::AllocBuffer,
# NStdString::Assign, NStlWString::Copy, NStlWordVector::PushBack, NHtmlItem::ParseEditType...),
# which points to limited inlining in the original; and tools/difftest.py --standalone can then
# reach them by name. The hook target keeps /Ob2: its code is what the verifier and the game
# have checked.
target_compile_options(nwd_standalone_objs PRIVATE ${NWD_CXX_FLAGS} /Ob1)

# the color cursors, taken from orig/NWindow.dll at build time (orig/ is not in git)
set(NWD_SA_RES_DIR "${CMAKE_BINARY_DIR}/standalone_res")
set(NWD_SA_RC "${NWD_SA_RES_DIR}/NWindowCursors.rc")
add_custom_command(OUTPUT "${NWD_SA_RC}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/extract_cursors.py" "${NWD_SA_RES_DIR}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/extract_cursors.py" "${CMAKE_CURRENT_SOURCE_DIR}/orig/NWindow.dll"
  COMMENT "color cursors from orig/NWindow.dll")
add_custom_target(nwd_standalone_res DEPENDS "${NWD_SA_RC}")
set_source_files_properties("${NWD_SA_RC}" PROPERTIES GENERATED TRUE)

# Same layout as the stock image: ImageBase 0x10000000 without ASLR, DllCharacteristics 0 and no
# manifest resource, the stock exports with their ordinals (NWindow.def), a map for
# tools/verify_standalone.py. The base only keeps file addresses comparable (our map/pdb against
# the stock image): Core.dll, loaded first, sits at 0x10100000 inside that range, so the loader
# relocates the stock NWindow.dll and ours alike.
set(NWD_SA_LINK_OPTIONS /DEBUG /OPT:REF /OPT:ICF /BASE:0x10000000 /DYNAMICBASE:NO /NXCOMPAT:NO
    /MANIFEST:NO /MAPINFO:EXPORTS ${NWD_CRT_COMPAT_OBJS})
function(nwd_standalone_dll_of objs target out_name out_dir)
  add_library(${target} SHARED EXCLUDE_FROM_ALL
    $<TARGET_OBJECTS:${objs}>
    src/standalone/NWindow.def
    "${NWD_SA_RC}")
  add_dependencies(${target} ue_import_libs nwd_standalone_res)
  # The C4 NWindow.dll imports (tools/verify_standalone.py --checks imports compares them): Core, Engine, kernel32
  # (a default library), user32, imm32, version, advapi32 (NKeyCrypt), ws2_32 (MSN), gdi32 (GetDeviceCaps),
  # shlwapi (MSN passport URLs), wininet (MSN passport login) and winmm (timeGetTime: only the nProtect library
  # calls it, so nothing imports it here) - plus npkcrypt.dll by ordinal, which the standalone does NOT import
  # (src/nwindow/NKeyCrypt.cpp uses LoadLibrary and GetProcAddress)
  target_link_libraries(${target} PRIVATE ${UE_IMPORT_LIBS} user32 imm32 version winmm advapi32 ws2_32 gdi32 shlwapi wininet)
  target_link_options(${target} PRIVATE ${NWD_SA_LINK_OPTIONS} ${ARGN}
    "/MAP:$<TARGET_FILE_DIR:${target}>/${out_name}.map")
  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "${out_name}"
    RUNTIME_OUTPUT_DIRECTORY "${out_dir}"
    ARCHIVE_OUTPUT_DIRECTORY "${out_dir}"
    PDB_OUTPUT_DIRECTORY "${out_dir}")
endfunction()
function(nwd_standalone_dll target out_name out_dir)
  nwd_standalone_dll_of(nwd_standalone_objs ${target} ${out_name} ${out_dir} ${ARGN})
endfunction()
nwd_standalone_dll(NWindowStandalone NWindow "${CMAKE_BINARY_DIR}/standalone")
# The probe: the same link with /FORCE:UNRESOLVED, so the export table, the resources and the map
# can be checked before the first real link. NEVER loaded anywhere: every unresolved symbol
# points at address 0.
nwd_standalone_dll(NWindowStandaloneProbe NWindow_probe "${CMAKE_BINARY_DIR}/standalone_probe"
  /FORCE:UNRESOLVED)

# --- offline smoke test: loads the patched stock NWindow.dll (and so our DLL) outside the game ----
# Own output folder: with NWindowDecomp.dll next to the exe the loader would take that copy.
# nwd_smoke_standalone.cpp is the phase 5 half (--standalone): loads a standalone NWindow.dll the
# engine's way; it needs nothing of the game source.
add_executable(nwd_smoke tools/smoke/nwd_smoke.cpp tools/smoke/nwd_smoke_standalone.cpp)
target_compile_definitions(nwd_smoke PRIVATE ${NWD_DEFS})
set_target_properties(nwd_smoke PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/smoke")

# --- differential test runner (tools/difftest.py): pieces of NWindow.dll run outside the game on
# fixed inputs, with the stock code or ours. Own output folder, like the smoke test. The cases
# still hard-code C1 addresses (docs/PLANO_C2.md): the runner builds, tools/difftest.py refuses to
# run them on the C2 image.
file(GLOB NWD_DIFFTEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/difftest/*.cpp")
add_executable(nwd_difftest ${NWD_DIFFTEST_SOURCES})
target_compile_definitions(nwd_difftest PRIVATE ${NWD_DEFS})
target_compile_options(nwd_difftest PRIVATE /W3 /utf-8 /EHa)
set_target_properties(nwd_difftest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/difftest")
