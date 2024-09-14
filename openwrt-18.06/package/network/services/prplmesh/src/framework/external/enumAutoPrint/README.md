# CPP Enum Auto-Print

When defining an Enum on a CPP file it is impossible to print the enumerator string without adding manually a code snippet that does it - converting the enum value to a string, and adding an output stream operator for printing it.
Adding code that does it for every enum is a painful job.

CPP Enum Auto-Print is a tool written in Python3 which able to scan a CPP repository, detecting Enums, and automatically generate a code snippet below an Enum definition that will make the Enum printable.

Example:

```cpp
enum eBananaStates {
    GreenAndHardLikeHulk,
    LightGreenTasteLikeAvocado,
    SweetYellowHeaven,
    DontWaitAnotherDay,
    OmgItsRotten
};
```

The script will generate the following code:

```cpp
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
const char *eBananaStates_str(eBananaStates enum_value) {
    switch (enum_value) {
    case GreenAndHardLikeHulk:       return "GreenAndHardLikeHulk";
    case LightGreenTasteLikeAvocado: return "LightGreenTasteLikeAvocado";
    case SweetYellowHeaven:          return "SweetYellowHeaven";
    case DontWaitAnotherDay:         return "DontWaitAnotherDay";
    case OmgItsRotten:               return "OmgItsRotten";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
std::ostream &operator<<(std::ostream &out, eBananaStates value) { return out << eBananaStates_str(value); }
// Enum AutoPrint generated code snippet end
```

The script can be used inside a building script and modify the source code before compiling it.

## Limitations

1. The generated code use `std::ostream` and `std::string`. It is the user's responsibility to make sure `string` and `ostream` are included so the code will compile successfully.
2. The script will not generate code for the following cases:
   * Anonymous Enum (Enum with not name).
   * Enum with no enumerator-list.
   * Enum which has an enumerator-list with identical values.
   * Enum which on its enumerator-list there is an enumerator whose value is calculated using another enum or constexpr value.
   * Enum struct (enum class is supported though).

## Configuration File

The script uses a JSON configuration file with the following properties:

| Property | Mandatory | Default Value | Summary |
| --- | --- | --- | --- |
| `workspaceRoot` | yes | NA | The root folder of the project. Can be a relative path to the script location.|
| `spaces_in_tab` | yes | 4 | The width of a hard tab character in source code. |
| `testWorkspaceRoot` | Only for test mode | NA | An alternate root path for testing the script. |
| `formatter_open_direction`<br/>`formatter_close_direction` | No | "" | A direction to the formatter which will be added to the auto generated code beginning  and end. For example "// clang-format off". |
| `redirect_stdout_to_log_file` | No | False | If "True" script output will be directed to a log file "output.log", otherwise to the console. |
| `excluded_files` | No | NA | List of files and folders to ignore. |

## Disable code generation on a specific enum

To disable code generation for a specific enum, add the following comment to the code, and for the enum below it, auto-generated code will not be generated.

```cpp
// enum auto-print skip
```

## Running the script

To run the script on the project root:

```bash
./enum_auto_print/enum_auto_print.py
```

To run the script on test mode:

```bash
./enum_auto_print/enum_auto_print.py -t
```

## Disclaimer

The script works successfully on the common majority of cases (please see `test/test.h).
If there is an enum definition that the script fails to generate code for, please let me know.
morantr@gmail.com
