# gef_embed_text(<target> <input> <function>)

# Generates a .cpp defining std::string_view GEF::Embedded::<function>() with the file's bytes
function(gef_embed_text target input function)
    file(READ "${input}" filedata HEX)

    # Convert hex data to C-style byte array
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${filedata}")

    set(out "${CMAKE_CURRENT_BINARY_DIR}/embedded/${function}.cpp")
    file(WRITE "${out}"
            "#include <Engine/Core/Embedded.hpp>\n"
            "namespace { constexpr unsigned char data[] = { ${bytes} }; }\n"
            "std::string_view GEF::Embedded::${function}()\n"
            "{ return { reinterpret_cast<const char*>(data), sizeof(data) }; }\n")

    # Reconfigure when the shader changes
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${input}")
    target_sources(${target} PRIVATE "${out}")
endfunction()
