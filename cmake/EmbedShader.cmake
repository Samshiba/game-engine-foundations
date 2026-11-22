find_package(Python3 COMPONENTS Interpreter REQUIRED)

# gef_embed_shader(<target> <input> <function>)

# Generates a .cpp defining std::string_view GEF::Embedded::<function>() with the file's bytes
# Resolves GLSL #includes at build-time
function(gef_embed_shader target input function)
    get_filename_component(abs_input "${input}" ABSOLUTE)
    set(out_cpp "${CMAKE_CURRENT_BINARY_DIR}/embedded/${function}.cpp")
    set(out_dep "${CMAKE_CURRENT_BINARY_DIR}/embedded/${function}.d")

    add_custom_command(
            OUTPUT "${out_cpp}"
            COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/embed_shader.py"
            --input "${abs_input}"
            --output "${out_cpp}"
            --depfile "${out_dep}"
            --func "${function}"
            MAIN_DEPENDENCY "${abs_input}"
            DEPFILE "${out_dep}"
            COMMENT "Embedding shader ${function} (${input})"
            VERBATIM
    )

    target_sources(${target} PRIVATE "${out_cpp}")
endfunction()