# NKGen/cmake/NKGen.cmake

cmake_minimum_required(VERSION 3.10)

function(GenerateModules target)

    # Get the list of modules passed to the function
    set(modules ${ARGN})
    
    # Create a directory for generated files
    set(GEN_DIR "${CMAKE_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY "${GEN_DIR}")
    
    if(EMSCRIPTEN OR DEFINED BUILD_IOS)
        # Set the nkgen executable path for Emscripten
        set(NKGEN "${CMAKE_BINARY_DIR}/nkgen/nkgen")
    else()
       # Get the path to the nkgen executable
        set(NKGEN "$<TARGET_FILE:nkgen>")
    endif()

    
    set(NANOKIT_DIR "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../lib")

    foreach(mod ${modules})
        # Get the base name: if mod is "src/Window", then mod_base becomes "Window"
        get_filename_component(mod_base ${mod} NAME)

        # Convert mod_base to uppercase
        string(TOUPPER ${mod_base} mod_base_upper)
        
        # Form full paths for the input files (we append the extension)
        set(xml_file "${CMAKE_CURRENT_SOURCE_DIR}/${mod}.xml")
        set(src_file "${CMAKE_CURRENT_SOURCE_DIR}/${mod}.c")
        
        # Generated file paths (we expect nkgen to produce these names)
        set(gen_header "${GEN_DIR}/${mod_base}.xml.h")
        set(gen_src    "${GEN_DIR}/${mod_base}.xml.c")
        
        add_custom_command(
            OUTPUT ${gen_header} ${gen_src}  # These files are the output of the custom command
            COMMAND ${NKGEN} ${mod_base} ${xml_file} ${gen_header} ${gen_src}
            COMMENT "RUNNING NKGEN ${mod_base} ${xml_file} ${gen_header} ${gen_src}"
            DEPENDS ${xml_file} nkgen            # nkgen depends on the .xml file
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            VERBATIM
        )

        # add the module as a static library
        add_library(${mod_base} STATIC 
            ${gen_header} 
            ${gen_src}
            ${src_file}
        )

        target_compile_definitions(${mod_base} PUBLIC "${mod_base_upper}_BUILD")
        target_include_directories(${mod_base} PUBLIC ${GEN_DIR} ${NANOKIT_DIR}/kit ${NANOKIT_DIR}/../NanoWin/lib)
        message("include directory: ${NANOKIT_DIR}")
        target_link_libraries(${target} ${mod_base} NanoWin)
        
    endforeach()

endfunction()
