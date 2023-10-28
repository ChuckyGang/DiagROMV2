#!/usr/bin/env python3

# SPDX-License-Identifier: MIT

# python3 -m pip import libclang==16.0.6

#
# Converts C-structs to Amiga OS assembler structure defines.
#
# h2i.py [-h] [-t] filename [args ...]
#
#   filename     .h/.c/.cpp file to parse
#   args         additional libclang arguments
#
# optional arguments:
#   -t, --tests  generate offset/size tests
#
# Latest code available at : https://github.com/erique/h2i
#
# Copyright (c) 2023 Erik Hemming
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

import sys
import re
from os.path import basename, dirname, splitext
from argparse import ArgumentParser, RawDescriptionHelpFormatter
from clang.cindex import Index, CursorKind, TypeKind, TokenKind, TranslationUnit

def map_type(var_type):
    """Map libclang type to AmigaOS type."""
    if var_type.kind == TypeKind.POINTER:
        return "APTR"
    if var_type.kind == TypeKind.CHAR_S:
        return "BYTE"
    if var_type.kind == TypeKind.SCHAR:
        return "BYTE"
    if var_type.kind == TypeKind.UCHAR:
        return "UBYTE"
    if var_type.kind == TypeKind.SHORT:
        return "WORD"
    if var_type.kind == TypeKind.USHORT:
        return "UWORD"
    if var_type.kind == TypeKind.INT:
        return "LONG"
    if var_type.kind == TypeKind.UINT:
        return "ULONG"
    if var_type.kind == TypeKind.LONG:
        return "LONG"
    if var_type.kind == TypeKind.ULONG:
        return "ULONG"
    if var_type.kind == TypeKind.FLOAT:
        return "FLOAT"
    if var_type.kind == TypeKind.DOUBLE:
        return "DOUBLE"
    if var_type.kind == TypeKind.RECORD:
        return "STRUCT"
    if var_type.kind == TypeKind.CONSTANTARRAY:
        return "STRUCT"
    sys.stderr.write(str(var_type.kind) + "(" + var_type.spelling + ') is not known')
    sys.exit (-1)

def get_elaborated_type(node):
    """Unpack an elaborated type until it's a basic type or struct/array."""
    node_type = node.type
    while node_type.kind == TypeKind.ELABORATED:
        node = node_type.get_declaration()
        if node.kind == CursorKind.TYPEDEF_DECL:
            node_type = node.underlying_typedef_type
        elif node.kind == CursorKind.ENUM_DECL:
            node_type = node.enum_type
        else:
            node_type = node.type
    return node_type

def write_field_decl(field):
    """Write out a struct memeber field."""
    var_type = field.type
    var_name = field.spelling

    if field.type.kind == TypeKind.CONSTANTARRAY:
        # STRUCT {name},{numElem}*{elemSize}
        elem_size = field.type.element_type.get_size()
        array_len = field.type.element_count
        var_name = field.spelling + "," + str(array_len) + "*" + str(elem_size)
    elif field.type.kind == TypeKind.ELABORATED:
        #  STRUCT {name},{typeSize}
        # or
        #  {type} {name}
        var_type = get_elaborated_type(field)
        if var_type.kind in (TypeKind.RECORD, TypeKind.CONSTANTARRAY):
            var_name = field.spelling + "," + str(var_type.get_size())
    else:
        #  {type} {name}
        var_type = field.type
        var_name = field.spelling

    type_name = map_type(var_type)

    # write out field w/ comments
    comment = field.brief_comment[:36] if field.brief_comment is not None else None
    if comment is None:
        fmt =  "{indent}{type_name:<7} {var_name}"
    else:
        fmt =  "{indent}{type_name:<7} {var_name:<22} ; {comment}"
    print(fmt.format(indent=" "*8, type_name=type_name, var_name=var_name, comment=comment))

def write_struct(struct):
    """Write out a struct definition."""
    # write leading comments (could be multiline)
    if struct.raw_comment is not None:
        print ("\n;" + "\n; ".join(struct.raw_comment.splitlines()))

    # open STRUCTURE
    fmt =  "{indent}{type_name:<11} {var_name}"
    print(fmt.format(indent=" "*4, type_name="STRUCTURE", var_name=struct.spelling + ",0"))

    # add members
    offset = 0
    for cursor in struct.get_children():
        if cursor.kind != CursorKind.FIELD_DECL:
            continue

        # C structs use 2-byte/word alignment; we may need to align to WORD boundary
        reported_offset = int(struct.type.get_offset(cursor.spelling)/8)
        if offset != reported_offset:
            assert reported_offset-offset == 1
            print (" "*8 + "ALIGNWORD")
            offset = (offset+1) & 0xfffffffe

        write_field_decl(cursor)
        offset += cursor.type.get_size()

    # write out "sizeof" label
    fmt =  "{indent}{typeName:<11} {varName:<22} ; {comment}\n"
    print(fmt.format(indent=" "*4, typeName="LABEL", varName=struct.spelling + "_sizeof",
                        comment=str(struct.type.get_size()) + " bytes"))

def write_enum(enum):
    """Write out an enum statement."""
    # write leading comments (could be multiline)
    if enum.raw_comment is not None:
        print ("\n;" + "\n; ".join(enum.raw_comment.splitlines()))
    else:
        # open ENUM
        fmt =  "{indent}; enum {enum}"
        print(fmt.format(indent=" "*4, enum=enum.spelling))

    old_value = None
    for cursor in enum.get_children():
        value = cursor.enum_value
        eitem = cursor.spelling
        if old_value is None or old_value != value:
            if value == 0:
                fmt =  "{indent}{enum}"
            else:
                fmt =  "{indent}{enum:<7} {value}"

            print(fmt.format(indent=" "*4, enum="ENUM", value=value))
        old_value = value + 1

        # write out enum w/ comments
        comment = cursor.brief_comment[:36] if cursor.brief_comment is not None else None
        if comment is None:
            fmt =  "{indent}{eitem:<7} {name}"
        else:
            fmt =  "{indent}{eitem:<7} {name:<22} ; {comment}"
        print(fmt.format(indent=" "*4, eitem="EITEM", name=eitem, comment=comment))

    # close ENUM
    fmt =  "{indent}; end of enum {enum}\n"
    print(fmt.format(indent=" "*4, enum=enum.spelling))

def write_define(define):
    """Write out a a define/equ statement."""
    tokens = list(define.get_tokens())
    if len(tokens) < 2:
        return
    value = ""
    for token in tokens[1:]:
        if token.kind == TokenKind.LITERAL and token.spelling[0] != '"':
            literal = int(re.sub('[ulzULZ]', '', token.spelling),0)
            if literal < 32:
                value += str(literal)
            else:
                value += "${:x}".format(literal)
        else:
            value += token.spelling
    fmt =  "{key:<24} EQU {value}"
    print(fmt.format(key=define.spelling, value=value, comment=define.brief_comment))

def recurse_children(cursor):
    """Process nodes"""
    if cursor.kind != CursorKind.TRANSLATION_UNIT:
        if str(cursor.location.file) != opts.filename:
            return

    if cursor.kind == CursorKind.STRUCT_DECL:
        write_struct(cursor)
    elif cursor.kind == CursorKind.ENUM_DECL:
        write_enum(cursor)
    elif cursor.kind == CursorKind.MACRO_DEFINITION:
        write_define(cursor)
    elif cursor.kind == CursorKind.TRANSLATION_UNIT:
        for child in cursor.get_children():
            recurse_children(child)

def write_struct_test(struct):
    """Write out build-time checks to make sure offsets match"""
    fmt =  """
    IF      {label}!={size}
    PRINTV  {label}
    PRINTV  {size}
    FAIL    {label} mismatch
    ENDC"""

    # check member offsets
    for cursor in struct.get_children():
        if cursor.kind != CursorKind.FIELD_DECL:
            continue
        print (fmt.format(label=cursor.spelling,
                            size=int(struct.type.get_offset(cursor.spelling)/8)))

    # check STRUCTURE size
    print(fmt.format(label=struct.spelling + "_sizeof", size=str(struct.type.get_size())))

def recurse_children_test(cursor):
    """Process nodes for tests"""
    if cursor.kind != CursorKind.TRANSLATION_UNIT:
        if str(cursor.location.file) != opts.filename:
            return

    if cursor.kind == CursorKind.STRUCT_DECL:
        write_struct_test(cursor)
    else:
        for child in cursor.get_children():
            recurse_children_test(child)

def main():
    global opts

    builtin_args = "-fparse-all-comments -std=c89 -m32 -fpack-struct=2 -nostdinc"

    parser = ArgumentParser(
                        description="Converts C-structs to Amiga OS assembler structure defines.",
                        formatter_class=RawDescriptionHelpFormatter,
                        epilog=f"Default libclang arguments : '{builtin_args}'\n"+
                                "Latest code available at   : https://github.com/erique/h2i")

    parser.add_argument("filename", help=".h/.c/.cpp file to parse")
    parser.add_argument("args", nargs="*", help="additional libclang arguments")
    parser.add_argument("-t", "--tests", dest="gen_tests", action="store_true",
                        default=False, help="generate offset/size tests")
    opts = parser.parse_args()

    index = Index.create()
    tu = index.parse(opts.filename,
                    args=builtin_args.split() + opts.args,
                    options=TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD +
                            TranslationUnit.PARSE_INCOMPLETE +
                            TranslationUnit.PARSE_SKIP_FUNCTION_BODIES )

    for diag in tu.diagnostics:
        sys.stderr.write (str(diag))
        sys.exit(-1)

    fmt = "; This file is GENERATED from {file} using {script}. Edits will be LOST!\n"
    print(fmt.format(file=basename(opts.filename), script=basename(__file__)))

    (path, ext) = splitext(opts.filename)
    parent = basename(dirname(path))
    base = basename(path)
    define = f"{parent}_{base}_i".upper()
    print (" "*4 + "IFND    " + define)
    print (define + " SET 1\n")
    print ('    NOLIST')
    print ('    INCLUDE "exec/types.i"')
    print ('    LIST\n')

    recurse_children(tu.cursor)
    if opts.gen_tests:
        recurse_children_test(tu.cursor)

    print ("\n" + " "*4 + "ENDC    ; " + define)

if __name__ == '__main__':
    main()
