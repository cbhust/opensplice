/*
 *                         OpenSplice DDS
 *
 *   This software and documentation are Copyright 2006 to 2009 PrismTech
 *   Limited and its licensees. All rights reserved. See file:
 *
 *                     $OSPL_HOME/LICENSE
 *
 *   for full copyright notice and license terms.
 *
 */
/*
   This module generates Splice type definitions related to
   an IDL input file.
*/

#include "idl_program.h"
/**
 * @file
 * This module generates Standalone C data types
 * related to an IDL input file.
*/

#include "idl_scope.h"
#include "idl_genSACSType.h"
#include "idl_genSACSHelper.h"
#include "idl_genSplHelper.h"
#include "idl_genCHelper.h"
#include "idl_tmplExp.h"
#include "idl_dependencies.h"
#include "idl_genLanguageHelper.h"
#include "idl_dll.h"

#include "os.h"
#include <ctype.h>
#include "c_typebase.h"

    /** indentation level */
static c_long indent_level = 0;
    /** enumeration element index */
static c_long enum_element = 0;

static void idl_arrayDimensions(idl_typeArray typeArray);

/** @brief generate name which will be used as a macro to prevent multiple inclusions
 *
 * From the specified basename create a macro which will
 * be used to prevent multiple inclusions of the generated
 * header file. The basename characters are translated
 * into uppercase characters and the append string is
 * appended to the macro.
 */
static c_char *
idl_macroFromBasename(
    const char *basename,
    const char *append)
{
    static c_char macro[200];
    c_long i;

    for (i = 0; i < (c_long)strlen(basename); i++) {
        macro[i] = toupper(basename[i]);
        macro[i+1] = '\0';
    }
    strncat(macro, append, (size_t)((int)sizeof(macro)-(int)strlen(append)));

    return macro;
}

static os_equality
defName(
    void *iterElem,
    void *arg)
{
    if (strcmp((char *)iterElem, (char *)arg) == 0) {
        return OS_EQ;
    }
    return OS_NE;
}

/* @brief callback function called on opening the IDL input file.
 *
 * Generate standard file header consisting of:
 * - inclusion of Splice type definition files
 *
 * @param scope Current scope (not used)
 * @param name Name of the IDL input file
 */
static idl_action
idl_fileOpen(
    idl_scope scope,
    const char *name,
    void *userData)
{
    /* Generate inclusion of standard OpenSplice DDS type definition files */
    idl_fileOutPrintf(idl_fileCur(), "using DDS;\n");
    idl_fileOutPrintf(idl_fileCur(), "using System.Runtime.InteropServices;\n");
    idl_fileOutPrintf(idl_fileCur(), "\n");

    /* return idl_explore to indicate that the rest of the file needs to be processed */
    return idl_explore;
}

/** @brief callback function called on module definition in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
   =>   module <module-name> {
            <module-contents>
        };
   @endverbatim
 *
 * @param scope Current scope (and scope of the module definition)
 * @param name Name of the defined module
 */
static idl_action
idl_moduleOpen(
    idl_scope scope,
    const char *name,
    void *userData)
{
    /* Generate the C# code that opens the namespace. */
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "namespace %s\n", idl_CsharpId(name));
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "{\n");

    /* Increase the indentation level. */
    indent_level++;

    /* return idl_explore to indicate that the rest of the module needs to be processed */
    return idl_explore;
}

/** @brief callback function called on module termination in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
        module <module-name> {
            <module-contents>
   =>   };
   @endverbatim
 *
 */
static void
idl_moduleClose(
    void *userData)
{

    /* Decrease the indentation level back to its original size. */
    indent_level--;

    /* Generate the C# code that closes the namespace. */
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "}\n");
    idl_fileOutPrintf(idl_fileCur(), "\n");
}

/** @brief callback function called on structure definition in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
   =>   struct <structure-name> {
            <structure-member-1>;
            ...              ...
            <structure-member-n>;
        };
   @endverbatim
 *
 * @param scope Current scope (and scope of the structure definition)
 * @param name Name of the structure
 * @param structSpec Specification of the struct holding the amount of members
 */
static idl_action
idl_structureOpen(
    idl_scope scope,
    const char *name,
    idl_typeStruct structSpec,
    void *userData)
{
    /* Add the metadata of this struct to a list for later processing. */
    idl_metaCharpAddType(scope, name, idl_typeSpec(structSpec), (os_iter *)userData);

    /* Generate the C# code that opens a sealed class. */
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "#region %s\n", name);
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "[StructLayout(LayoutKind.Sequential)]\n");
    idl_printIndent(indent_level);
    idl_fileOutPrintf(
        idl_fileCur(),
        "public sealed class %s\n", name);
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "{\n");

    /* Increase the indentation level. */
    indent_level++;

    /* return idl_explore to indicate that the rest of the structure needs to be processed */
    return idl_explore;
}

/** @brief callback function called on end of a structure definition in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
        struct <structure-name> {
            <structure-member-1>
            ...              ...
            <structure-member-n>
   =>   };
   @endverbatim
 *
 * The structure is closed:
 * @verbatim
        };
   @endverbatim
 *
 * @param name Name of the structure (not used)
 */
static void
idl_structureClose (
    const char *name,
    void *userData)
{
    /* Decrease the indentation level back to its original size. */
    indent_level--;

    /* Generate the C# code that closes the class. */
    idl_printIndent(indent_level);
    idl_fileOutPrintf (idl_fileCur(), "};\n");
    idl_printIndent(indent_level);
    idl_fileOutPrintf (idl_fileCur(), "#endregion\n\n");
}

/** @brief callback function called on definition of a structure member in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
        struct <structure-name> {
   =>       <structure-member-1>;
   =>       ...              ...
   =>       <structure-member-n>;
        };
   @endverbatim
 *
 * @param scope Current scope
 * @param name Name of the structure member
 * @param typeSpec Type specification of the structure member
 */
static void
idl_structureMemberOpenClose (
    idl_scope scope,
    const char *name,
    idl_typeSpec typeSpec,
    void *userData)
{
    /* Dereference possible typedefs first. */
    while (idl_typeSpecType(typeSpec) == idl_ttypedef) {
        typeSpec = idl_typeDefRefered(idl_typeDef(typeSpec));
    }

    idl_printIndent(indent_level);
    switch (idl_typeSpecType(typeSpec))
    {
    case idl_tbasic:
        /* generate code for a standard empty ("") string.  */
        if (idl_typeBasicType(idl_typeBasic (typeSpec)) == idl_string) {
            idl_fileOutPrintf(
                idl_fileCur(),
                "public %s %s = \"\";\n",
                idl_CsharpTypeFromTypeSpec(typeSpec),
                idl_languageId(name));
        } else {
            /* generate code for a standard primitive type. */
            idl_fileOutPrintf(
                idl_fileCur(),
                "public %s %s;\n",
                idl_CsharpTypeFromTypeSpec(typeSpec),
                idl_languageId(name));
        }
        break;

    case idl_tenum:
        /* generate code for a standard enum. */
        idl_fileOutPrintf(
            idl_fileCur(),
            "public %s %s;\n",
            idl_CsharpTypeFromTypeSpec(typeSpec),
            idl_languageId(name));
        break;

    case idl_tstruct:
    case idl_tunion:
        /* generate code for a standard mapping of a struct or union user-type mapping */
        idl_fileOutPrintf(
            idl_fileCur(),
            "public %s %s = new %s();\n",
            idl_CsharpTypeFromTypeSpec(typeSpec),
            idl_languageId(name),
            idl_CsharpTypeFromTypeSpec(typeSpec));
        break;
    case idl_tarray:
    {
        /* Initialize to 2 ('[]') so that every other dimension just has to add 1 ','. */
        int emptyStrLen = 2;
        int indexStrLen = 2;

        /* generate code for an array mapping */
        idl_fileOutPrintf(
            idl_fileCur(),
            "public %s%s %s = new %s%s;\n",
            idl_CsharpTypeFromTypeSpec (typeSpec),
            idl_arrayCsharpIndexString(typeSpec, SACS_EXCLUDE_INDEXES, &emptyStrLen),
            idl_languageId(name),
            idl_CsharpTypeFromTypeSpec(typeSpec),
            idl_arrayCsharpIndexString(typeSpec, SACS_INCLUDE_INDEXES, &indexStrLen));
        break;
    }

    case idl_tseq:
    {
        /* Initialize to 2 ('[]') so that every other dimension just has to add 2 '[]'. */
        int emptyStrLen = 2;
        int indexStrLen = 2;

        /* generate code for a sequence mapping */
        idl_fileOutPrintf(
            idl_fileCur(),
            "public %s%s %s = new %s%s;\n",
            idl_CsharpTypeFromTypeSpec(typeSpec),
            idl_sequenceCsharpIndexString(typeSpec, SACS_EXCLUDE_INDEXES, &emptyStrLen),
            idl_languageId(name),
            idl_CsharpTypeFromTypeSpec(typeSpec),
            idl_sequenceCsharpIndexString(typeSpec, SACS_INCLUDE_INDEXES, &indexStrLen));
        break;
    }

    default:
        printf("idl_structureMemberOpenClose: Unsupported structure member type (member name = %s, type name = %s)\n",
            name, idl_scopedTypeName(typeSpec));
        break;
    }
}

/** @brief callback function called on definition of a union in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
   =>   union <union-name> switch(<switch-type>) {
            case label1.1; .. case label1.n;
                <union-case-1>;
            case label2.1; .. case label2.n;
                ...        ...
            case labeln.1; .. case labeln.n;
                <union-case-n>;
            default:
                <union-case-m>;
        };
   @endverbatim
 *
 * @param scope Current scope
 * @param name Name of the union
 * @param unionSpec Specifies the number of union cases and the union switch type
 */
static idl_action
idl_unionOpen(
    idl_scope scope,
    const char *name,
    idl_typeUnion unionSpec,
    void *userData)
{
    if (idl_definitionExists("definition", idl_scopeStack (scope, "_", name))) {
        return idl_abort;
    }
    idl_definitionAdd("definition", idl_scopeStack (scope, "_", name));

    /* Generate the C# code that opens a sealed class. */
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "#region %s\n", name);
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "[StructLayout(LayoutKind.Explicit)]\n");
    idl_printIndent(indent_level);
    idl_fileOutPrintf(
        idl_fileCur(),
        "public sealed struct %s\n", name);
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "{\n");

    /* Increase the indentation level. */
    indent_level++;

    /* Generate the discriminator and its getter property. */
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(),"[FieldOffset(0)]\n");
    idl_printIndent(indent_level);
    idl_fileOutPrintf(
        idl_fileCur(),
        "private %s _d;\n",
        idl_CsharpTypeFromTypeSpec(idl_typeUnionSwitchKind(unionSpec)));
    idl_printIndent(indent_level);
    idl_fileOutPrintf(
        idl_fileCur(),
        "public %s Discriminator\n",
        idl_CsharpTypeFromTypeSpec(idl_typeUnionSwitchKind(unionSpec)));
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(),"{\n");
    idl_printIndent(++indent_level);
    idl_fileOutPrintf(idl_fileCur(), "get { return _d; }\n");
    idl_printIndent(--indent_level);
    idl_fileOutPrintf(idl_fileCur(),"}\n");



//    /* define the prototype of the function allocate the union */
//    idl_fileOutPrintf(
//        idl_fileCur(),
//        "%s *%s__alloc (void);\n",
//        idl_scopeStack(scope, "_", name),
//        idl_scopeStack (scope, "_", name));
//    /* open the struct */
//    idl_printIndent(indent_level);
//    idl_fileOutPrintf(
//        idl_fileCur(),
//        "struct %s {\n",
//        idl_scopeStack(scope, "_", name));
//    indent_level++;
//    /* generate code for the switch */
//    if (idl_typeSpecType(idl_typeUnionSwitchKind(unionSpec)) == idl_tbasic) {
//        idl_printIndent(indent_level);
//         idl_fileOutPrintf(
//            idl_fileCur(),
//            "%s _d;\n",
//            idl_CsharpTypeFromTypeSpec(idl_typeUnionSwitchKind(unionSpec)));
//    } else if (idl_typeSpecType(idl_typeUnionSwitchKind(unionSpec)) == idl_tenum) {
//        idl_printIndent(indent_level);
//        idl_fileOutPrintf(
//            idl_fileCur(),
//            "%s _d;\n",
//            idl_CsharpTypeFromTypeSpec(idl_typeUnionSwitchKind(unionSpec)));
//    } else if (idl_typeSpecType(idl_typeUnionSwitchKind(unionSpec)) == idl_ttypedef) {
//        switch (idl_typeSpecType(idl_typeDefActual(idl_typeDef(idl_typeUnionSwitchKind(unionSpec))))) {
//        case idl_tbasic:
//        case idl_tenum:
//            idl_printIndent(indent_level);
//            idl_fileOutPrintf(
//                idl_fileCur(),
//                "%s _d;\n",
//                idl_scopedSacTypeIdent(idl_typeUnionSwitchKind(unionSpec)));
//        break;
//        default:
//            printf ("idl_unionOpen: Unsupported switckind\n");
//        }
//    } else {
//        printf ("idl_unionOpen: Unsupported switckind\n");
//    }
//    /* open the union */
//    idl_printIndent(indent_level);
//    idl_fileOutPrintf(idl_fileCur(), "union {\n");
//    indent_level++;

    /* return idl_explore to indicate that the rest of the union needs to be processed */
    return idl_explore;
}

/** @brief callback function called on closure of a union in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
        union <union-name> switch(<switch-type>) {
            case label1.1; .. case label1.n;
                <union-case-1>;
            case label2.1; .. case label2.n;
                ...        ...
            case labeln.1; .. case labeln.n;
                <union-case-n>;
            default:
                <union-case-m>;
   =>   };
   @endverbatim
 *
 * The union is closed:
 * @verbatim
            } _u;
        };
   @endverbatim
 * @param name Name of the union
 */
static void
idl_unionClose (
    const char *name,
    void *userData)
{
    indent_level--;
    idl_printIndent(indent_level); idl_fileOutPrintf(idl_fileCur(), "} _u;\n");
    indent_level--;
    idl_printIndent(indent_level); idl_fileOutPrintf(idl_fileCur(), "};\n\n");
}

/** @brief callback function called on definition of a union case in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
        union <union-name> switch(<switch-type>) {
            case label1.1; .. case label1.n;
   =>           <union-case-1>;
            case label2.1; .. case label2.n;
   =>           ...        ...
            case labeln.1; .. case labeln.n;
   =>           <union-case-n>;
            default:
   =>           <union-case-m>;
        };
   @endverbatim
 *
 * @param scope Current scope (the union the union case is defined in)
 * @param name Name of the union case
 * @param typeSpec Specifies the type of the union case
 */
static void
idl_unionCaseOpenClose(
    idl_scope scope,
    const char *name,
    idl_typeSpec typeSpec,
    void *userData)
{
//    if (idl_typeSpecType(typeSpec) == idl_tbasic ||
//        idl_typeSpecType(typeSpec) == idl_ttypedef ||
//        idl_typeSpecType(typeSpec) == idl_tenum ||
//        idl_typeSpecType(typeSpec) == idl_tstruct ||
//        idl_typeSpecType(typeSpec) == idl_tunion) {
//        /* generate code for a standard mapping or a typedef, enum, struct or union user-type mapping */
//        idl_printIndent(indent_level);
//        idl_fileOutPrintf(
//            idl_fileCur(),
//            "%s %s;\n",
//            idl_scopedSacTypeIdent(typeSpec),
//            idl_languageId(name));
//    } else if (idl_typeSpecType(typeSpec) == idl_tarray) {
//        /* generate code for an array mapping */
//        idl_printIndent(indent_level);
//        if (idl_typeSpecType(idl_typeArrayActual (idl_typeArray(typeSpec))) != idl_tseq) {
//            idl_fileOutPrintf(
//                idl_fileCur(),
//                "%s %s",
//                idl_CsharpTypeFromTypeSpec(idl_typeArrayActual(idl_typeArray(typeSpec))),
//                idl_languageId (name));
//        } else {
//            idl_fileOutPrintf(
//                idl_fileCur(),
//                "%s %s",
//                idl_CsharpTypeFromTypeSpec(idl_typeSeq(idl_typeArrayActual(idl_typeArray(typeSpec)))),
//                idl_languageId (name));
//        }
//        idl_arrayDimensions(idl_typeArray(typeSpec));
//        idl_fileOutPrintf(idl_fileCur(), ";\n");
//    } else if (idl_typeSpecType(typeSpec) == idl_tseq) {
//        /* generate code for a sequence mapping */
//        idl_printIndent(indent_level);
//        idl_fileOutPrintf(
//            idl_fileCur(),
//            "%s %s;\n",
//            idl_sequenceIdent(idl_typeSeq(typeSpec)),
//            idl_languageId(name));
//    } else {
//        printf("idl_unionCaseOpenClose: Unsupported union case type (case name = %s, type = %s)\n",
//            name, idl_scopedTypeName(typeSpec));
//    }
}

/** @brief callback function called on definition of an enumeration.
 *
 * Generate code for the following IDL construct:
 * @verbatim
   =>   enum <enum-name> {
            <enum-element-1>;
            ...          ...
            <enum-element-n>;
        };
   @endverbatim
 *
 * @param scope Current scope
 * @param name Name of the enumeration
 * @param enumSpec Specifies the number of elements in the enumeration
 */
static idl_action
idl_enumerationOpen(
    idl_scope scope,
    const char *name,
    idl_typeEnum enumSpec,
    void *userData)
{
    if (idl_definitionExists("definition", idl_scopeStack(scope, ".", name))) {
        return idl_abort;
    }
    idl_definitionAdd("definition", idl_scopeStack(scope, ".", name));

    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "#region %s\n", name);
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "public enum %s\n", name);
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "{\n");
    enum_element = idl_typeEnumNoElements(enumSpec);
    indent_level++;

    /* return idl_explore to indicate that the rest of the enumeration needs to be processed */
    return idl_explore;
}

/** @brief callback function called on closure of an enumeration in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
        enum <enum-name> {
            <enum-element-1>;
            ...          ...
            <enum-element-n>;
   =>   };
   @endverbatim
 *
 * @param name Name of the enumeration
 */
static void
idl_enumerationClose (
    const char *name,
    void *userData)
{
    indent_level--;
    idl_printIndent(indent_level);
    idl_fileOutPrintf(idl_fileCur(), "};\n");
    idl_printIndent(indent_level);
    idl_fileOutPrintf (idl_fileCur(), "#endregion\n\n");
}

/** @brief callback function called on definition of an enumeration element in the IDL input file.
 *
 * Generate code for the following IDL construct:
 * @verbatim
        enum <enum-name> {
   =>       <enum-element-1>,
   =>       ...          ...
   =>       <enum-element-n>
        };
   @endverbatim
 *
 * For the last element generate:
 * @verbatim
        <element-name>
   @endverbatim
 * For any but the last element generate:
 * @verbatim
    <element-name>,
   @endverbatim
 *
 * @param scope Current scope
 * @param name Name of the enumeration element
 */
static void
idl_enumerationElementOpenClose (
    idl_scope scope,
    const char *name,
    void *userData)
{
    enum_element--;
    if (enum_element == 0) {
        idl_printIndent(indent_level);
        idl_fileOutPrintf(idl_fileCur(), "%s\n", name);
    } else {
        idl_printIndent(indent_level);
        idl_fileOutPrintf(idl_fileCur(), "%s,\n", name);
    }
}


static void
idl_constantOpenClose (
    idl_scope scope,
    idl_constSpec constantSpec,
    void *userData)
{
    idl_printIndent(indent_level);
    idl_fileOutPrintf(
            idl_fileCur(),
            "struct %s { static %s value = %s; }\n\n",
            idl_constSpecName(constantSpec),
            idl_CsharpTypeFromTypeSpec(idl_constSpecTypeGet(constantSpec)),
            idl_constSpecImage(constantSpec));
}

/**
 * Standard control structure to specify that inline
 * type definitions are to be processed prior to the
 * type itself in contrast with inline.
*/
static idl_programControl idl_genSACSLoadControl = {
    idl_prior
};

/** @brief return the program control structure for the splice type generation functions.
 */
static idl_programControl *
idl_getControl(
    void *userData)
{
    return &idl_genSACSLoadControl;
}

/**
 * Specifies the callback table for the splice type generation functions.
 */
static struct idl_program idl_genSACSType;

/** @brief return the callback table for the splice type generation functions.
 */
idl_program
idl_genSACSTypeProgram(
    os_iter *idlpp_metaList)
{
    idl_genSACSType.idl_getControl                  = idl_getControl;
    idl_genSACSType.fileOpen                        = idl_fileOpen;
    idl_genSACSType.fileClose                       = NULL;
    idl_genSACSType.moduleOpen                      = idl_moduleOpen;
    idl_genSACSType.moduleClose                     = idl_moduleClose;
    idl_genSACSType.structureOpen                   = idl_structureOpen;
    idl_genSACSType.structureClose                  = idl_structureClose;
    idl_genSACSType.structureMemberOpenClose        = idl_structureMemberOpenClose;
    idl_genSACSType.enumerationOpen                 = idl_enumerationOpen;
    idl_genSACSType.enumerationClose                = idl_enumerationClose;
    idl_genSACSType.enumerationElementOpenClose     = idl_enumerationElementOpenClose;
    idl_genSACSType.unionOpen                       = idl_unionOpen;
    idl_genSACSType.unionClose                      = idl_unionClose;
    idl_genSACSType.unionCaseOpenClose              = idl_unionCaseOpenClose;
    idl_genSACSType.unionLabelsOpenClose            = NULL;
    idl_genSACSType.unionLabelOpenClose             = NULL;
    idl_genSACSType.typedefOpenClose                = NULL;
    idl_genSACSType.boundedStringOpenClose          = NULL;
    idl_genSACSType.sequenceOpenClose               = NULL;
    idl_genSACSType.constantOpenClose               = idl_constantOpenClose;
    idl_genSACSType.artificialDefaultLabelOpenClose = NULL;
    idl_genSACSType.userData                        = idlpp_metaList;

    return &idl_genSACSType;
}