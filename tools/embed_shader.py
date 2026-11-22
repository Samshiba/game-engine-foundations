import argparse
import re
from pathlib import Path

INCLUDE_REGEX = re.compile(r'^\s*#include\s+"([^"]+)"\s*$')


def resolve_includes(
        file_path: Path,
        file_to_id: dict[Path, int],
        visited_stack: list[Path],
) -> str:
    file_path = file_path.resolve()

    # circular include
    if file_path in visited_stack:
        cycle = " -> ".join(p.name for p in visited_stack + [file_path])
        raise RuntimeError(f"Circular #include detected: {cycle}")

    # pragma once
    if file_path in file_to_id:
        return f"// [Skipped duplicate include: {file_path.name}]\n"

    file_id = len(file_to_id)
    file_to_id[file_path] = file_id
    visited_stack.append(file_path)

    if not file_path.exists():
        raise FileNotFoundError(f"Included shader file not found: {file_path}")

    lines = file_path.read_text(encoding="utf-8").splitlines(keepends=True)
    output: list[str] = []

    for line_idx, line in enumerate(lines, start=1):
        match = INCLUDE_REGEX.match(line)
        if match:
            rel_include = match.group(1)
            target_path = (file_path.parent / rel_include).resolve()

            if target_path in file_to_id:
                output.append(f"// [Skipped duplicate include: {target_path.name}]\n")
            else:
                target_id = len(file_to_id)
                included_content = resolve_includes(target_path, file_to_id, visited_stack)

                output.append(f"#line 1 {target_id}\n")
                output.append(included_content.rstrip("\r\n") + "\n")
                output.append(f"#line {line_idx + 1} {file_id}\n")
        else:
            output.append(line)

    visited_stack.pop()
    return "".join(output)


def inject_file_map(source: str, file_to_id: dict[Path, int]) -> str:
    id_lines = ["// ---------- Shader File IDs ----------"]
    for path, fid in sorted(file_to_id.items(), key=lambda item: item[1]):
        id_lines.append(f"// {fid}: {path.name}")

    lines = source.splitlines(keepends=True)
    for idx, line in enumerate(lines, start=1):
        if line.lstrip().startswith("#version"):
            header = "\n".join(id_lines + [f"#line {idx + 1} 0"]) + "\n"
            lines.insert(idx, header)
            return "".join(lines)

    # Fallback if no #version
    header = "\n".join(id_lines + ["#line 1 0"]) + "\n"
    return header + source


def write_cpp(output_cpp: Path, function_name: str, content: str, debug: bool) -> None:
    if debug:
        include = content
    else:
        data = content.encode("utf-8")
        hex_lines = []
        for i in range(0, len(data), 16):
            chunk = ", ".join(f"0x{b:02x}" for b in data[i:i + 16])
            hex_lines.append(f"    {chunk},")
        include = "\n".join(hex_lines)

    cpp_code = (
        "#include <Engine/Core/Embedded.hpp>\n\n"
        "namespace {\n"
        "constexpr unsigned char data[] = {\n"
        f"{include}\n"
        "};\n"
        "} // namespace\n\n"
        f"std::string_view GEF::Embedded::{function_name}()\n"
        "{\n"
        "    return { reinterpret_cast<const char*>(data), sizeof(data) };\n"
        "}\n"
    )
    output_cpp.parent.mkdir(parents=True, exist_ok=True)
    output_cpp.write_text(cpp_code, encoding="utf-8")


def write_depfile(depfile: Path, output_cpp: Path, dependencies: set[Path]) -> None:
    depfile.parent.mkdir(parents=True, exist_ok=True)
    deps_str = " ".join(p.as_posix().replace(" ", "\\ ") for p in sorted(dependencies))
    depfile.write_text(f"{output_cpp.as_posix()}: {deps_str}\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--depfile", required=True, type=Path)
    parser.add_argument("--func", required=True, type=str)
    parser.add_argument("--debug", required=False, type=bool, default=False)
    args = parser.parse_args()

    file_to_id: dict[Path, int] = {}
    resolved_source = resolve_includes(args.input, file_to_id, [])
    resolved_source = inject_file_map(resolved_source, file_to_id)

    write_cpp(args.output, args.func, resolved_source, args.debug)
    write_depfile(args.depfile, args.output, set(file_to_id.keys()))


if __name__ == "__main__":
    main()
