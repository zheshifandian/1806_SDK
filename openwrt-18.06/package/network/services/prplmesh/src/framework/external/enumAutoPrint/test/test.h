// clang-format off
#include <ostream>
#include <string>

// Adding comment line containing the string 'enum auto-print skip' will tell the script to skip
// the next enum, whether is is possible to generate code for or not.
// enum auto-print skip
enum eSkipMe {
    eSkipMe1,
    eSkipMe2,
};

enum eClassExternalEnum {
    eClassExternalEnum_1 = 0,
    eClassExternalEnum_2,

    // Comment in the middle
    eClassExternalEnum_3,

    eClassExternalEnum_4, // Comment on the side
    
    /* comment */ eClassExternalEnum_5,
    
    /*
    eClassExternalEnum_6,
    */
};

int function()
{
    // Anonymous enum - The script shall not generate code for it.
    enum {
        ANONYMOUS1 = 0,
        ANONYMOUS2 = 1,
    } eAnonymousEnum = ANONYMOUS1;

    enum{
        ANONYMOUS_COUPLED1 = 0,
        ANONYMOUS_COUPLED2 = 1,
    } eAnonymousCoupled = ANONYMOUS_COUPLED1;

    enum:uint8_t{
        ANONYMOUS_COUPLED_COLONS1 = 0,
        ANONYMOUS_COUPLED_COLONS2 = 1,
    } eAnonymousCoupledColons = ANONYMOUS_COUPLED_COLONS1;

    enum : uint8_t{
        ANONYMOUS_COLONS1 = 0,
        ANONYMOUS_COLONS2 = 1,
    } eAnonymousColons = ANONYMOUS_COLONS1;

    {
    }

    return eAnonymousEnum;
}

enum eNameColonsCoupled: uint8_t {
    NAME_COLONS_COUPLED1,
    NAME_COLONS_COUPLED2,
};

enum eNoList1 {};

enum eNoList2;

enum eHexValues {
    HEX_VALUE1 = 0x10,
    HEX_VALUE2 = 0x20,
};

enum eChar {
    A = 'A',
    B = 'B',
};

// Enum with idnedical values - The script shall not generate code for it.
// The reason is, a switch-case shall not have a idnetical cases. In such case, need to consider to
// replace the enum with 'constexpr'.
enum eIdenticalValues {
    IDENTICAL_VALUE1 = 0,
    IDENTICAL_VALUE2 = 0,
};

// Enum with internal enum value calculation which are based on different enum value - The script
// shall not generate code for it.
// In such case, need to consider to replace the enum with 'constexpr'.
enum eInternalCalc_1 {
    INTERNAL_CALC1 = 0,
    INTERNAL_CALC2 = IDENTICAL_VALUE2 + 1,
};

// For simple calculation no based on a differnet enum value, the script will generate code.
enum eInternalCalc_2 {
    INTERNAL_CALC1 = 1 + 1,
    INTERNAL_CALC2 = 2 + 2,
};

// Enum struct - The script shall not generate code for it, since it is same as "enum class" so
// define "enum class" if you want auto-generated code.
enum struct eEnumStruct {
    eEnumStruct_1,
    eEnumStruct_2,
};

enum [[maybe_unused]] eWithAttr {
    eWithAttr1,
    eWithAttr2,
};

class TestClass {

    // This script test enum and enum class { COMMENT };

    /* Commented enum
    enum eInClassCommentedEnum {
        eInClassCommentedEnum_1 = 0,
        eInClassCommentedEnum_2,
    };
    */
    enum eInClassEnum : int {
        eInClassEnum_1 = 0,
        eInClassEnum_2,
    };

    enum class eInClassEnumClass : int { eInClassEnumClass_1 = 0, eInClassEnumClass_2 };
};

// Enum values alignent test
enum eAlignmentTest 
{
    VeryVeryVeryLongName = 0,
    ShortName = 1,
};
