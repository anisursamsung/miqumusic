#!/usr/bin/env python3
import sys
import os

def main():
    if len(sys.argv) < 5:
        print("Usage: embed_asset.py <input_file> <output_cpp> <namespace> <symbol_base_name>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_cpp = sys.argv[2]
    namespace = sys.argv[3]
    symbol_name = sys.argv[4]

    if not os.path.exists(input_file):
        print(f"Error: Input asset file '{input_file}' not found.")
        sys.exit(1)

    os.makedirs(os.path.dirname(os.path.abspath(output_cpp)), exist_ok=True)

    with open(input_file, "rb") as f:
        data = f.read()

    with open(output_cpp, "w") as f:
        f.write("#include <cstddef>\n\n")
        f.write(f"namespace {namespace} {{\n\n")
        f.write(f"extern const unsigned char {symbol_name}[] = {{\n  ")
        
        bytes_formatted = [f"0x{b:02x}" for b in data]
        lines = []
        for i in range(0, len(bytes_formatted), 16):
            lines.append(", ".join(bytes_formatted[i:i+16]))
        
        f.write(",\n  ".join(lines))
        f.write("\n};\n\n")
        f.write(f"extern const size_t {symbol_name}Len = {len(data)};\n\n")
        f.write(f"}} // namespace {namespace}\n")

if __name__ == "__main__":
    main()
