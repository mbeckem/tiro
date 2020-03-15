#!/usr/bin/env python3

import cog

from codegen import camel_to_snake, avoid_keyword, ENV


class Tag:
    def __init__(self, name, underlying_type, doc=None):
        self.kind = "tag"
        self.name = name
        self.underlying_type = underlying_type
        self.doc = doc
        self.union = None


class TaggedUnion:
    def __init__(self, name, tag, members=None, doc=None):
        self.name = name
        self.kind = "union"
        self.tag = tag
        self.doc = doc
        self.members = [] if members is None else members
        self.format = None
        self.equality = None
        self.hash = None
        self.trivial = True
        self.doc_mode = "member"

        if tag.union:
            raise RuntimeError("Tag already belongs to a different union")
        tag.union = self

    # declare: only declare, but do not define the format(FormatStream&) function.
    # define: do both
    def set_format_mode(self, which):
        if which not in [None, "declare", "define"]:
            raise RuntimeError(f"Invalid value for 'which': {which}.")
        self.format = which
        return self

    def set_equality_mode(self, which):
        if which not in [None, "define"]:
            raise RuntimeError(f"Invalid value for 'which': {which}.")
        self.equality = which
        return self

    def set_hash_mode(self, which):
        if which not in [None, "define"]:
            raise RuntimeError(f"Invalid value for 'which': {which}.")
        self.hash = which
        return self

    def set_doc_mode(self, which):
        if which not in ["member", "tag"]:
            raise RuntimeError(f"Invalid value for 'which': {which}")
        self.doc_mode = which
        return self


class UnionMember:
    def __init__(self, name, kind, doc=None):
        self.name = name

        snake = camel_to_snake(name)
        self.argument_name = avoid_keyword(snake)
        self.accessor_name = "as_" + snake
        self.factory_name = "make_" + snake
        self.field_name = snake + "_"
        self.visit_name = "visit_" + snake
        self.kind = kind
        self.doc = doc

    def set_name(self, name):
        self.name = name
        return self

    def set_argument_name(self, argument_name):
        self.argument_name = argument_name
        return self

    def set_accessor_name(self, accessor_name):
        self.accessor_name = accessor_name
        return self


class UnionMemberStruct(UnionMember):
    def __init__(self, name, members=None, doc=None):
        super().__init__(name, "struct", doc)
        self.members = [] if members is None else members


class UnionMemberAlias(UnionMember):
    def __init__(self, name, target, doc=None):
        super().__init__(name, "alias", doc)
        self.target = target


class StructMember:
    def __init__(self, name, type, doc=None):
        self.name = avoid_keyword(name)
        self.type = type
        self.doc = doc


def declare_type(T):
    templ = ENV.get_template("unions.jinja2")
    cog.outl(templ.module.declare_type(T))


def define_inlines(T):
    templ = ENV.get_template("unions.jinja2")
    cog.outl(templ.module.define_inline(T))


def define_type(T):
    templ = ENV.get_template("unions.jinja2")
    cog.outl(templ.module.define_type(T))


def implement_type(T):
    templ = ENV.get_template("unions.jinja2")
    cog.outl(templ.module.implement_type(T))
