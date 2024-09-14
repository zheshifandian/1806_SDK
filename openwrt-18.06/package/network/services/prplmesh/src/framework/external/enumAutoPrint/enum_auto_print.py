#!/usr/bin/env python3

import time
import argparse
import json
import os
import io
import sys
from typing import List, Tuple
import re
from pprint import pprint as pprint


def to_number(n):
    '''
    Convert any number representation to a number
    This covers: float, decimal, hex, and octal numbers.
    '''

    # Convert AsciI character
    if type(n) is str and str(n).startswith("'") and str(n).endswith("'") and len(n) == 3:
        n = str(n).strip("'")
        n = ord(n)

    # Remove "U" or "u" which indicate unsigned value.
    if type(n) is str:
        n = str(n).strip("Uu")

    try:
        return int(str(n), 0)
    except Exception:
        try:
            # python 3 doesn't accept "010" as a valid octal.  You must use the '0o' prefix.
            return int('0o' + n, 0)
        except Exception:
            return float(n)


class Configuration():
    test_mode: bool = True
    test_mode_remove_out_files = True
    test_mode_override_files = True
    spaces_in_tab: int = 4
    code_snippet_open_message = "// Enum AutoPrint generated code snippet begining- DON'T EDIT!\n"
    code_snippet_close_message = "// Enum AutoPrint generated code snippet end\n"
    formatter_open_str = ""
    formatter_close_str = ""
    output_file = io.StringIO()


class EnumMetadata():
    def __init__(self):
        self.enum_name: str
        self.enum_values: List[str] = list()
        self.enum_class: bool
        self.in_class: bool
        self.in_header_file: bool

    def __str__(self):
        return str(self.__dict__)


class Coordinate(object):
    def __init__(self, line_idx: int, char_idx: int):
        self.line_idx: int = line_idx
        self.char_idx: int = char_idx

    def __str__(self):
        return str((self.line_idx, self.char_idx))


class CodePosition(object):

    def __init__(self, position_of_type: str, start_pos: Coordinate):
        # Can have values of "enum", "class" or "enum class".
        self.position_of_type: str = position_of_type

        self.start_pos: Coordinate = start_pos

        # Initiate the following since it cause to the class __dict__ to be ordered by
        # initialization order.
        self.open_curl_pos: Coordinate = Coordinate(0, 0)
        self.close_curl_pos: Coordinate = Coordinate(0, 0)

        # Represent the delta between open curl brackets (e.g "{") to curl brackets (e.g "}").
        # For example "{{}{{}" will have value of "4" (4-2).
        # when the balance is equal to 1, it means that the cursor is currently it he middle of
        # class/struct definition.
        self.curl_bracket_balance: int = 0

    def __str__(self):
        dict_copy = self.__dict__.copy()
        dict_copy["start_pos"] = str(self.start_pos)
        dict_copy["open_curl_pos"] = str(self.open_curl_pos)
        dict_copy["close_curl_pos"] = str(self.close_curl_pos)
        return str(dict_copy)


def get_enum_metadata(position_data: CodePosition, in_class: bool, in_header_file: bool,
                      lines: List[str]) -> EnumMetadata:
    enum_metadata = EnumMetadata()

    enum_metadata.enum_class = (position_data.position_of_type == "enum class")
    enum_metadata.in_class = in_class
    enum_metadata.in_header_file = in_header_file

    # Since C/C++ code can be written all in one line, or start new line after each space, need
    # to create separate strings which one of them contain the enum deleration first character
    # until curly bracket opening which the name of the enum will be extracted from:
    # enum eEnumName : uint8_t {
    # <----enum declaration---->
    #
    # The second string will contain all enum values and which are between the curly brackets
    # { value1 = 0, value2, ... }
    # <-------enum values------->

    enum_decleration: str = ""
    enum_values: str = ""

    # enum deleration
    line_idx = position_data.start_pos.line_idx
    char_idx = position_data.start_pos.char_idx
    line_idx_end = position_data.open_curl_pos.line_idx
    char_idx_end = position_data.open_curl_pos.char_idx
    while line_idx <= line_idx_end:
        if line_idx == line_idx_end:
            enum_decleration += lines[line_idx][char_idx: char_idx_end].strip()
            break
        else:
            enum_decleration += lines[line_idx][char_idx:].strip()
            char_idx = 0
            line_idx += 1

    # print("decl:", enum_decleration)

    # Remove attribute if exist.
    enum_decleration = re.sub(r"[\[\[].*[\]\]]", "", enum_decleration)

    # Extract the enum name from the enum decleration line.
    deleration_split = enum_decleration.split()
    if (len(deleration_split) < 2):
        # Anonymous Enum, unhandled by definition.
        return False
    if deleration_split[1] == "class":
        enum_metadata.enum_name = deleration_split[2]
    else:
        # Anonymous Enum, unhandled by definition.
        if (deleration_split[0].endswith(':') or deleration_split[1].startswith(':')
                or deleration_split[0].endswith(':')):
            return False
        enum_metadata.enum_name = deleration_split[1].strip(':')
    # print("enum name:", enum_metadata.enum_name)

    # enum values
    line_idx = position_data.open_curl_pos.line_idx
    char_idx = position_data.open_curl_pos.char_idx + 1
    line_idx_end = position_data.close_curl_pos.line_idx
    char_idx_end = position_data.close_curl_pos.char_idx
    comment_block = False
    while line_idx <= line_idx_end:
        if line_idx == line_idx_end:
            enum_values += lines[line_idx][char_idx: char_idx_end].strip()
            break
        else:
            line = lines[line_idx][char_idx:]
            char_idx = 0
            line_idx += 1
            if len(line) == 0 or line.startswith("//"):
                continue
            idx = line.find("//")
            if idx != -1:
                line = line[:idx - 1]
            idx = line.find("/*")
            if idx != -1:
                comment_block = True
                idx2 = line.find("*/")
                if idx2 != -1:
                    line = line[:idx - 1] + line[idx2 + 2:]
                    comment_block = False
                else:
                    line = line[:idx - 1]

            if comment_block:
                idx2 = line.find("*/")
                if idx2 == -1:
                    continue
                else:
                    line = line[idx2 + 2:]
                    comment_block = False
            enum_values += line.strip()

    # print("values:", enum_values)
    enum_values_split = enum_values.split(",")
    enum_numeric_values = set()
    for value_line in enum_values_split:
        value_line_split = value_line.split()
        for idx, value in enumerate(value_line_split):
            if idx == 0:
                # The enum value string
                enum_metadata.enum_values.append(value)
            elif value_line_split[idx - 1] == "=":
                # The enum numeric value
                try:
                    enum_value_int = int(to_number(value_line_split[idx]))
                except Exception:
                    return False
                if enum_value_int not in enum_numeric_values:
                    enum_numeric_values.add(enum_value_int)
                else:
                    # This enum has identical values, discard it.
                    return False
    # print("enum values list:", enum_metadata.enum_values)

    # Discard enum with empty numerator-list.
    if len(enum_metadata.enum_values) == 0:
        return False

    return enum_metadata


def add_generated_code(enum_metadata: EnumMetadata, base_indentation: int, lines_out: List[str]):

    def getIndentation(level) -> str:
        return str(" ") * (base_indentation + level * Configuration.spaces_in_tab)

    def getStatic():
        if (enum_metadata.in_class or enum_metadata.in_header_file):
            return "static "
        return str()

    def getEnumClass():
        if (enum_metadata.enum_class):
            return enum_metadata.enum_name + "::"
        return str()

    def getFriend():
        if (enum_metadata.in_class):
            return "friend "
        return str()

    def get_inline():
        if (enum_metadata.in_header_file):
            return "inline "
        return str()

    max_enum_length = len(max(enum_metadata.enum_values, key=len))

    def spacesPad(enum_value: str):
        return (max_enum_length - len(enum_value) + 1) * " "

    lines_out.append("{0}{1}".format(getIndentation(0), Configuration.code_snippet_open_message))

    if len(Configuration.formatter_open_str) > 0:
        lines_out.append("{0}{1}\n".format(getIndentation(0), Configuration.formatter_open_str))

    lines_out.append('{0}{1}const char *{2}_str({2} enum_value) {{\n'.format(
        getIndentation(0), getStatic(), enum_metadata.enum_name))

    lines_out.append('{}switch (enum_value) {{\n'.format(getIndentation(1)))

    for enum_value in enum_metadata.enum_values:
        lines_out.append('{0}case {1}{2}:{3}return "{1}{2}";\n'.format(
            getIndentation(1), getEnumClass(), enum_value, spacesPad(enum_value)))

    lines_out.append('{0}}}\n'.format(getIndentation(1)))
    lines_out.append('{0}static std::string out_str = std::to_string(int(enum_value));\n'.format(
        getIndentation(1)))
    lines_out.append('{0}return out_str.c_str();\n{1}}}\n'.format(
        getIndentation(1), getIndentation(0)))
    lines_out.append('{0}{1}{2}std::ostream &operator<<(std::ostream &out, {3} value) {{ '
                     'return out << {3}_str(value); }}\n'.format(
                         getIndentation(0), getFriend(), get_inline(), enum_metadata.enum_name))

    if len(Configuration.formatter_close_str) > 0:
        lines_out.append("{0}{1}\n".format(getIndentation(0), Configuration.formatter_close_str))

    lines_out.append("{0}{1}".format(getIndentation(0), Configuration.code_snippet_close_message))


def upgrade_enum(file_path: str):

    print("Processing file:", file_path)

    if len(file_path) == 0:
        return

    f = open(file_path, "r")
    lines = f.readlines()
    f.close()

    header_file = file_path.endswith(".h")

    lines_processed: List[str] = list()
    lines_out: List[str] = list()

    # '$' is a Joker character that may be ' '(space) , '{' or ':'.
    joker_chars: str = " :{"
    joker_char: str = str()

    enum_str: str = "enum$"
    enum_cnt = 0

    class_str: str = "class$"
    class_cnt = 0

    struct_str: str = "struct$"
    struct_cnt = 0

    # Used to indicate that enum/struct/class has been found and we are pending until its block
    # will be opened by a curly bracket.
    # Once curly bracket has been found, this variable should be cleared.
    pending_for_open_curl: bool = False

    positions_stack: List[CodePosition] = list()

    comment_block: bool = False
    double_quotes_block: bool = False
    single_quotes_block: bool = False
    between_bracket_block: bool = False
    between_bracket_block_char: str = str()
    skip_next_enum: bool = False
    auto_generated_snippet_to_ignore: bool = False

    for line_idx, line in enumerate(lines):

        if Configuration.code_snippet_open_message in line:
            auto_generated_snippet_to_ignore = True
        elif Configuration.code_snippet_close_message in line:
            auto_generated_snippet_to_ignore = False
            continue

        if auto_generated_snippet_to_ignore:
            continue

        lines_processed.append(line)
        line_idx = len(lines_processed) - 1
        lines_out.append(line)

        # Count the spaces from the line begining until the first printable character.
        indentation = 0
        match_obj = re.search(r'\S', line)
        if match_obj is None:
            indentation = 0
        else:
            indentation = match_obj.start()

        # print(line_idx, line.rstrip())
        for char_idx, char in enumerate(line):

            if char_idx < indentation:
                continue

            #  Skip comment characters that comes after double slash.
            chars_pair = line[char_idx - 1] + line[char_idx]
            if char_idx > 0 and chars_pair == "//":
                skip_next_enum = "enum auto-print skip" in line
                # Skip line
                break

            # Skip comment characters between open and close comment block.
            if chars_pair == "/*":
                comment_block = True
                continue

            if comment_block:
                if chars_pair == "*/":
                    comment_block = False
                continue

            # Skip characters inside double or single quotes.
            if char == '"':
                double_quotes_block = not double_quotes_block

            if not double_quotes_block and char == "'":
                single_quotes_block = not single_quotes_block

            if double_quotes_block or single_quotes_block:
                continue

            # Skip characters inside brackets.
            if char in '[(<':
                between_bracket_block = True
                between_bracket_block_char = char

            if between_bracket_block:
                if ((between_bracket_block_char == '[' and char == ']') or
                    (between_bracket_block_char == '(' and char == ')') or
                        (between_bracket_block_char == '<' and char == '>')):
                    between_bracket_block = False
                else:
                    continue

            if enum_cnt < len(enum_str):
                if enum_str[enum_cnt] == char:
                    enum_cnt += 1
                elif enum_str[enum_cnt] == "$" and char in joker_chars:
                    enum_cnt += 1
                    joker_char = char
                else:
                    enum_cnt = 0
            elif enum_cnt < len(enum_str):
                enum_cnt = 0
            else:
                # In this case, we know "enum " has been found but we want to use it only if
                # enum_cnt > len(enum_str) which will help to make sure class_cnt hasn't been
                # incremented to 1, which imply this is enum class and not a regular enum.
                enum_cnt += 1

            if class_str[class_cnt] == char:
                class_cnt += 1
            elif class_str[class_cnt] == "$" and char in joker_chars:
                class_cnt += 1
                joker_char = char
            elif class_cnt < len(class_str):
                class_cnt = 0

            if struct_str[struct_cnt] == char:
                struct_cnt += 1
            elif struct_str[struct_cnt] == "$" and char in joker_chars:
                struct_cnt += 1
                joker_char = char
            elif struct_cnt < len(struct_str):
                struct_cnt = 0

            if enum_cnt > len(enum_str):
                if class_cnt == len(class_str):
                    # This is enum class.
                    positions_stack.append(CodePosition(
                        "enum class", Coordinate(line_idx, char_idx + 1
                                                 - len(enum_str)
                                                 - len(class_str))))

                    pending_for_open_curl = joker_char != "{"
                    class_cnt = 0
                    enum_cnt = 0
                    continue
                elif class_cnt > 0:
                    # It might be an enum class. Skip until we know for sure.
                    continue
                else:
                    # This is regular enum.
                    positions_stack.append(CodePosition(
                        "enum", Coordinate(line_idx, char_idx - len(enum_str))))
                    pending_for_open_curl = joker_char != "{"
                    enum_cnt = 0
                    if char != "{":
                        continue

            if struct_cnt == len(struct_str) or class_cnt == len(class_str):
                # This is a class or struct.
                if struct_cnt == len(struct_str):
                    positions_stack.append(CodePosition(
                        "class", Coordinate(line_idx, char_idx + 1 - len(struct_str))))
                else:
                    positions_stack.append(CodePosition(
                        "class", Coordinate(line_idx, char_idx + 1 - len(class_str))))
                pending_for_open_curl = joker_char != "{"
                class_cnt = 0
                struct_cnt = 0
                continue

            # Discard enum with no numerator-list.
            if pending_for_open_curl and char == ";":
                positions_stack.pop()
                pending_for_open_curl = False

            if len(positions_stack) > 0 and (char == "{" or joker_char == "{"):

                if pending_for_open_curl or joker_char == "{":
                    positions_stack[-1].open_curl_pos = Coordinate(line_idx, char_idx)
                    positions_stack[-1].curl_bracket_balance = 1
                    pending_for_open_curl = False
                elif len(positions_stack) > 0:
                    positions_stack[-1].curl_bracket_balance += 1

                joker_char = str()

            elif len(positions_stack) > 0 and char == "}":
                positions_stack[-1].curl_bracket_balance -= 1
                positions_stack[-1].close_curl_pos = Coordinate(line_idx, char_idx)

                # Block of enum, enum class, class, or struct has ended.
                if positions_stack[-1].curl_bracket_balance == 0:
                    if ("enum" in positions_stack[-1].position_of_type):
                        # TODO: Generate Code
                        print(positions_stack[-1])
                        in_class: bool = len(
                            positions_stack) > 1 and positions_stack[-2].position_of_type == "class"
                        enum_metadata = get_enum_metadata(
                            positions_stack[-1], in_class, header_file, lines_processed)
                        if (not enum_metadata):
                            positions_stack.pop()
                            continue
                        pprint(enum_metadata.__dict__)
                        print()  # Add 1 more empty line for readability

                        if skip_next_enum:
                            skip_next_enum = False
                        else:
                            add_generated_code(enum_metadata, indentation, lines_out)

                    # enum/class/struct block is closed and can be removed from the stack.
                    positions_stack.pop()

    # Write output file
    if ((not Configuration.test_mode) or
            (Configuration.test_mode and Configuration.test_mode_override_files)):
        f = open(file_path, "w")
    else:
        f = open(file_path + ".out", "w")

    f.writelines(lines_out)
    f.close()


def get_files_list(workspace_root: str, exclude_files_paths: str) -> list:

    files_list = list()

    for subdir, dirs, files in os.walk(workspace_root):
        # print(subdir)

        for file in files:
            file = os.path.join(subdir, file)

            if (not file.endswith(".h") and
                not file.endswith(".c") and
                not file.endswith(".cpp") and
                    not file.endswith(".hpp")):
                continue

            skip_file = False
            for excluded_path in exclude_files_paths:
                if file.startswith(excluded_path):
                    # print("file is excluded:", file)
                    skip_file = True
                    break
            if skip_file:
                continue

            if Configuration.test_mode_remove_out_files and file.endswith(".out"):
                os.remove(file)
                continue

            # print("file:", file)
            files_list.append(file)

    return files_list


def redirect_stdout_to_file():

    path = os.path.abspath(sys.path[0] + '/' + 'output.log')
    Configuration.output_file = open(path, 'w')
    sys.stdout = Configuration.output_file


def read_configuration() -> Tuple[str, str]:
    with open(os.path.join(sys.path[0], "./enum_auto_print_conf.json"), "r") as conf_file:
        configuration = json.load(conf_file)

    redirect_stdout_to_log_file_key = 'redirect_stdout_to_log_file'
    if (redirect_stdout_to_log_file_key in configuration.keys()):
        if configuration[redirect_stdout_to_log_file_key]:
            redirect_stdout_to_file()

    print("Configuration:")
    pprint(configuration)
    print('')

    test_mode_remove_out_files_key = 'test_mode_remove_out_files'
    if (test_mode_remove_out_files_key in configuration.keys()):
        Configuration.test_mode_remove_out_files = configuration[test_mode_remove_out_files_key]
    print("test_mode_remove_out_files:", Configuration.test_mode_remove_out_files)

    test_mode_override_files_key = 'test_mode_override_files'
    if (test_mode_override_files_key in configuration.keys()):
        Configuration.test_mode_override_files = configuration[test_mode_override_files_key]
    print("test_mode_override_files:", Configuration.test_mode_override_files)

    spaces_in_tab_key = 'spaces_in_tab'
    if (spaces_in_tab_key in configuration.keys()):
        Configuration.spaces_in_tab = configuration[spaces_in_tab_key]

    formatter_open_direction_key = 'formatter_open_direction'
    if (formatter_open_direction_key in configuration.keys()):
        Configuration.formatter_open_str = configuration[formatter_open_direction_key]

    formatter_close_direction_key = 'formatter_close_direction'
    if (formatter_close_direction_key in configuration.keys()):
        Configuration.formatter_close_str = configuration[formatter_close_direction_key]

    exclude_files_paths = set()
    workspace_root_key = str()

    workspace_root_key = 'workspaceRoot'

    if workspace_root_key not in configuration.keys():
        raise Exception('Bad configuration: Missing field "{}" on configuration file'.format(
                        workspace_root_key))
    workspace_root = os.path.abspath(sys.path[0] + "/" + configuration[workspace_root_key])
    print("Workspace Root:", workspace_root, end="\n\n")

    test_workspace_root_key = 'testWorkspaceRoot'
    test_workspace_root: str = str()

    if test_workspace_root_key not in configuration.keys() and Configuration.test_mode:
        raise Exception('Bad configuration: Missing field "{}" on configuration file'.format(
                        test_workspace_root_key))
    test_workspace_root = os.path.abspath(
        sys.path[0] + "/" + configuration[test_workspace_root_key])
    print("Test Workspace Root:", test_workspace_root, end="\n\n")

    excluded_files_key = 'excluded_files'
    if (excluded_files_key not in configuration.keys()) or len(configuration.keys()) == 0:
        print('Info: Configuration does not contain "{}" field'.format(excluded_files_key))
        return

    if not Configuration.test_mode:
        exclude_files_paths.add(os.path.abspath(test_workspace_root))
    else:
        workspace_root = test_workspace_root

    for path in configuration[excluded_files_key]:
        path = path.replace(
            "${" + workspace_root_key + "}", workspace_root)

        path = path.replace(
            "${" + test_workspace_root_key + "}", test_workspace_root)

        exclude_files_paths.add(os.path.abspath(path))

    print("Excluded Paths:", *exclude_files_paths, sep='\n', end="\n\n")

    # raise Exception()
    return workspace_root, exclude_files_paths


if __name__ == '__main__':
    start_time = time.process_time()
    parser = argparse.ArgumentParser(description='TODO: Write description',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("-t", "--test", action='store_true', default=False, help="Test script")

    args = parser.parse_args()

    Configuration.test_mode = args.test

    # Read configuration file. Get the workspace root folder and an excluded files list
    workspace_root, exclude_files_paths = read_configuration()

    # Get files list.
    files_list = get_files_list(workspace_root, exclude_files_paths)
    print("Files List:", *files_list, sep='\n', end="\n\n")

    for file_path in files_list:
        upgrade_enum(file_path)

    print(sys.argv[0], "execution time:", time.process_time() - start_time, "sec")

    try:
        Configuration.output_file.close()
    except Exception:
        pass
