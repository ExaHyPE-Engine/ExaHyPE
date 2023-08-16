# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


LLVMSymbol = "__PACKED_ATTRIBUTES_LANGUAGE_EXTENSION__"


def get_include_guard(full_qualified_name):
    return "_INCLUDE_" + full_qualified_name.replace("::", "_").upper() + "_"


def get_unqualified_class_name(full_qualified_name):
    return full_qualified_name.split("::")[-1]


def get_namespaces(full_qualified_name):
    result = [x for x in full_qualified_name.split("::")]
    result.pop()
    return result


def construct_ifdef_string(symbol_list):
    """!

    Return empty string if symbol_list is empty. Otherwise construct
    the whole ifdef thing.

    """
    result = ""
    if symbol_list != []:
        result += "#if "
        added_one_ifdef = False
        for i in symbol_list:
            if added_one_ifdef:
                result += " and "
            added_one_ifdef = True
            result += i
        result += "\n"
    return result
