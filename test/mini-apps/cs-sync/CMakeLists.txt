SET(EXAMPLE "gp-cs-sync-clr")

macro(build_csharp outfile source)
  if(CSC)
    set(exe "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${outfile}.exe")
    add_custom_target(${outfile} ALL DEPENDS ${exe})
    set(source_winpath "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    string(REGEX REPLACE "/" "\\\\" source_winpath "${source_winpath}")
    add_custom_command(OUTPUT ${exe} DEPENDS ${source}
        COMMAND ${CSC} ARGS /debug /platform:x64 /t:exe /out:${outfile}.exe
        "${source_winpath}" VERBATIM)
  endif()
endmacro()

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    # use cmake c# integration
    enable_language("CSharp")
    # this builds application using roslyn
    add_executable(${EXAMPLE} "main.cs")
else()
    # native c# integration not available in ninja
    # If we up our cmake minimum to 3.8.2 we can use enable_language(CSharp).
    build_csharp(${EXAMPLE} "main.cs")

    # TODO: Implement same for CoreCLR
endif()

if(${DRACE_INSTALL_TESTS})
    install(FILES ${EXAMPLE}.exe ${EXAMPLE}.pdb DESTINATION bin OPTIONAL)
endif()
