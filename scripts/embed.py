import sys
import os


def create_c_header(input_path, output_path):
  try:
    with open(input_path, "rb") as f_in:
      data = f_in.read()
      if not data.endswith(b'\0'):
        data += b'\0' # ensure null-termination

  except FileNotFoundError:
    print(f"error: input file not found at '{input_path}'")
    sys.exit(1)
  except Exception as e:
    print(f"error reading input file: {e}")
    sys.exit(1)

  base_name = os.path.basename(input_path)
  var_name, _ = os.path.splitext(base_name)
  guard_name = f"{var_name.upper()}_H"

  try:
    with open(output_path, "w") as f_out:
      f_out.write(f"#ifndef {guard_name}\n")
      f_out.write(f"#define {guard_name}\n\n")
      f_out.write(f"unsigned char {var_name}[] = {{\n")

      hex_values = [f"0x{byte:02x}" for byte in data]

      lines = []
      for i in range(0, len(hex_values), 12):
        lines.append("  " + ", ".join(hex_values[i:i + 12]))

      f_out.write(",\n".join(lines))
      f_out.write("\n")

      f_out.write("};\n\n")
      f_out.write(f"unsigned int {var_name}_len = {len(data) - 1};\n\n")
      f_out.write(f"#endif /* {guard_name} */\n")

    print(f"successfully converted '{input_path}' to '{output_path}'.")

  except Exception as e:
    print(f"error writing output file: {e}")
    sys.exit(1)


if __name__ == "__main__":
  if len(sys.argv) != 3:
    print("usage: python embed.py <input_file> <output_file>")
    sys.exit(1)

  input_file = sys.argv[1]
  output_file = sys.argv[2]

  create_c_header(input_file, output_file)
