#!/usr/bin/env python3

import cog
from .codegen import camel_to_snake, avoid_keyword, ENV


class Tag:
    def __init__(self, name, underlying_type, start_value=None, doc=None):
        self.kind = "tag"
        self.name = name
        self.underlying_type = underlying_type
        self.start_value = start_value
        self.doc = doc
        self.union = None


class Union:
    def __init__(self, name, tag, members=None, doc=None):
        self.name = name
        self.kind = "union"
        self.tag = tag
        self.doc = doc
        self.members = [] if members is None else members
        self.format = None
        self.equality = None
        self.hash = None
        self.doc_mode = "member"
        self.storage_mode = "trivial"
        self.is_final = True

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

    # define: declare and implement equality operators.
    def set_equality_mode(self, which):
        if which not in [None, "define"]:
            raise RuntimeError(f"Invalid value for 'which': {which}.")
        self.equality = which
        return self

    # define: declare and implement build_hash function.
    def set_hash_mode(self, which):
        if which not in [None, "define"]:
            raise RuntimeError(f"Invalid value for 'which': {which}.")
        self.hash = which
        return self

    # member: Use doc for the doc comment of the member type
    # tag: Document the type tag instead
    def set_doc_mode(self, which):
        if which not in ["member", "tag"]:
            raise RuntimeError(f"Invalid value for 'which': {which}")
        self.doc_mode = which
        return self

    # trivial: do not declare any special member functions
    # movable: declare and implement destroy and move
    def set_storage_mode(self, which):
        if which not in ["trivial", "movable"]:
            raise RuntimeError(f"Invalid value for 'which': {which}")
        self.storage_mode = which
        return self

    def set_final(self, which):
        if which not in [True, False]:
            raise RuntimeError(f"Invalid value for 'which': {which}")
        self.is_final = which
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


class Struct(UnionMember):
    def __init__(self, name, members=None, doc=None):
        super().__init__(name, "struct", doc)
        self.members = [] if members is None else members


class Alias(UnionMember):
    def __init__(self, name, target, pass_as="copy", doc=None):
        super().__init__(name, "alias", doc)
        self.target = target
        self.pass_as = pass_as


class Field:
    def __init__(self, name, type, pass_as="copy", doc=None):
        self.name = avoid_keyword(name)
        self.type = type
        self.pass_as = pass_as
        self.doc = doc


def _declare(T):
    if T.kind == "tag":
        templ = ENV.get_template("union_tag.jinja2")
        cog.outl(templ.module.tag_decl(T))
    elif T.kind == "union":
        templ = ENV.get_template("unions.jinja2")
        cog.outl(templ.module.union_decl(T))
    else:
        raise RuntimeError("Invalid type.")


def _define(T):
    if T.kind == "tag":
        templ = ENV.get_template("union_tag.jinja2")
        cog.outl(templ.module.tag_def(T))
    elif T.kind == "union":
        templ = ENV.get_template("unions.jinja2")
        cog.outl(templ.module.union_def(T))
    else:
        raise RuntimeError("Invalid type.")


def _implement_inlines(T):
    if T.kind == "tag":
        pass
    elif T.kind == "union":
        templ = ENV.get_template("unions.jinja2")
        cog.outl(templ.module.union_inline_impl(T))
    else:
        raise RuntimeError("Invalid type.")


def _implement(T):
    if T.kind == "tag":
        templ = ENV.get_template("union_tag.jinja2")
        cog.outl(templ.module.tag_impl(T))
    elif T.kind == "union":
        templ = ENV.get_template("unions.jinja2")
        cog.outl(templ.module.union_impl(T))
    else:
        raise RuntimeError("Invalid type.")


def _joined(Ts, each):
    for (index, T) in enumerate(Ts):
        if index != 0:
            cog.outl()
        each(T)


def declare(*Ts):
    _joined(Ts, lambda T: _declare(T))


def define(*Ts):
    _joined(Ts, lambda T: _define(T))


def implement_inlines(*Ts):
    _joined(Ts, lambda T: _implement_inlines(T))


def implement(*Ts):
    _joined(Ts, lambda T: _implement(T))
