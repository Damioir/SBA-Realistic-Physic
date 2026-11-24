#!/usr/bin/env python3
"""Prepare the small arm64/POSIX Dobby source bundle used by this mod."""
from pathlib import Path
import shutil
import sys

source = Path(sys.argv[1]).resolve()
target = Path(sys.argv[2]).resolve()
if target.exists():
    shutil.rmtree(target)
target.mkdir(parents=True)
for name in ("LICENSE", "include", "external", "source"):
    original = source / name
    if not original.exists():
        raise SystemExit(f"pinned Dobby checkout is missing {name}")
    destination = target / name
    if original.is_dir():
        shutil.copytree(original, destination)
    else:
        shutil.copy2(original, destination)

sources = """external/logging/logging.cc
source/Backend/UserMode/ExecMemory/clear-cache-tool-all.c
source/Backend/UserMode/ExecMemory/code-patch-tool-posix.cc
source/Backend/UserMode/PlatformUtil/Linux/ProcessRuntimeUtility.cc
source/Backend/UserMode/UnifiedInterface/platform-posix.cc
source/core/arch/CpuFeature.cc
source/core/arch/CpuRegister.cc
source/core/assembler/assembler-arm64.cc
source/core/assembler/assembler.cc
source/core/codegen/codegen-arm64.cc
source/dobby.cpp
source/InstructionRelocation/arm64/InstructionRelocationARM64.cc
source/InterceptEntry.cpp
source/Interceptor.cpp
source/InterceptRouting/InterceptRouting.cpp
source/InterceptRouting/Routing/FunctionInlineHook/FunctionInlineHook.cc
source/InterceptRouting/Routing/FunctionInlineHook/RoutingImpl.cc
source/InterceptRouting/Routing/InstructionInstrument/InstructionInstrument.cc
source/InterceptRouting/Routing/InstructionInstrument/instrument_routing_handler.cc
source/InterceptRouting/Routing/InstructionInstrument/RoutingImpl.cc
source/InterceptRouting/RoutingPlugin/RoutingPlugin.cc
source/MemoryAllocator/AssemblyCodeBuilder.cc
source/MemoryAllocator/CodeBuffer/CodeBufferBase.cc
source/MemoryAllocator/MemoryAllocator.cc
source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_bridge_arm64.cc
source/TrampolineBridge/ClosureTrampolineBridge/arm64/ClosureTrampolineARM64.cc
source/TrampolineBridge/ClosureTrampolineBridge/arm64/helper_arm64.cc
source/TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.cc
source/TrampolineBridge/Trampoline/arm64/trampoline_arm64.cc
"""
(target / "sources.txt").write_text(sources, encoding="utf-8")
(target / "CMakeLists.txt").write_text("""cmake_minimum_required(VERSION 3.22.1)
file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/sources.txt DOBBY_MINI_SOURCES)
list(TRANSFORM DOBBY_MINI_SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/)
add_library(dobby_mini STATIC ${DOBBY_MINI_SOURCES})
target_include_directories(dobby_mini
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/source
    ${CMAKE_CURRENT_LIST_DIR}/source/dobby ${CMAKE_CURRENT_LIST_DIR}/source/Backend/UserMode
    ${CMAKE_CURRENT_LIST_DIR}/external ${CMAKE_CURRENT_LIST_DIR}/external/logging)
target_compile_definitions(dobby_mini PRIVATE __DOBBY_BUILD_VERSION__="Dobby-mini-ed69ebe")
set_target_properties(dobby_mini PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON)
target_compile_options(dobby_mini PRIVATE -Os -fno-rtti -fomit-frame-pointer
    -ffunction-sections -fdata-sections -w)
""", encoding="utf-8")
