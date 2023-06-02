# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import io
import os

def write_byte(file, data):
    file.write("0x{:02x}, ".format(ord(data)).encode('utf-8'))

def sanitize(input):
    if input[0] != '_' and not input[0].isalpha():
        input = "_" + input[1:]
    return "".join([character if character.isalnum() or character == "_" else "_" for character in input])

def write_header(args):
    if args.symbol is not None:
        symbol = args.symbol
    else:
        symbol = sanitize(args.input)

    with io.open(args.input, "rb") as input:
        try:
            with io.open(args.output, "wb") as output:
                output.write(b"/*!! THIS FILE IS GENERATED !!*/\n\n")
                output.write(b"#pragma once\n\n")
                output.write(b"namespace {\n")
                output.write(b"namespace GLES {\n")
                output.write("    constexpr char {}[] = {{".format(
                    symbol).encode('utf-8'))

                count = 0

                while 1:
                    char = input.read(1)

                    if not char:
                        break

                    if count % 32 == 0:
                        output.write(b"\n        ")

                    write_byte(output, char)
                    count += 1

                output.write(b"0x00\n    };\n")
                output.write(b"} // namespace GLES\n")
                output.write(b"}\n")
        except Exception:
            os.unlink(args.output)
            raise

    print(f"Generated {args.output}")

def main():
    argParser = argparse.ArgumentParser(
        description='Generate C/C++ #include containing a file as a string',
        formatter_class=argparse.RawTextHelpFormatter)

    argParser.add_argument("-i",
                           "--input",
                           metavar="INPUT_FILE",
                           type=str,
                           action="store",
                           help="Input file to serialize")

    argParser.add_argument("-o",
                           "--output",
                           metavar="OUTPUT_FILE",
                           type=str,
                           action="store",
                           help="Output file that contains the string")

    argParser.add_argument("-s",
                           "--symbol",
                           metavar="SYMBOL_NAME",
                           type=str,
                           action="store",
                           help="Defines the symbol name.")

    args = argParser.parse_args()

    write_header(args)

if __name__ == "__main__":
    main()
