#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Improves logs

  Usage:
    auto_trace.py --dir=<name of directory>
"""

import os
import re
from os import walk
from sys import argv
from argparse import ArgumentParser

file_extensions = ('.cc', '.cpp', '.hpp', '.h')

def modify_source(source_tuple, search_patter, replace_pattern):
  """Replaces LOG4CXX_TRACE_ENTER with LOG4CXX_AUTO_TRACE"""

  if(type(source_tuple) is not tuple):
      raise Exception('source_tuple', 'is not correct type')
  current_source = source_tuple[0]
  count_modification = source_tuple[1]

  regex = re.compile(search_patter)
  new_string, new_modification = regex.subn(replace_pattern, current_source)
  return (new_string, count_modification + new_modification)


def replace_trace_enter(source_tuple):
  """Replaces LOG4CXX_TRACE_ENTER with LOG4CXX_AUTO_TRACE"""

  return modify_source(source_tuple,
    r"(?P<space>\n\s*)LOG4CXX_TRACE_ENTER\((?P<logger>.*?)\);",
    r"\g<space>LOG4CXX_AUTO_TRACE(\g<logger>);")

def remove_trace_exit(source_tuple):
  """Removes LOG4CXX_TRACE_EXIT"""

  return modify_source(source_tuple,
    r"\n\s*LOG4CXX_TRACE_EXIT\(.*?\);",
    r"")

def replace_info_warn(source_tuple):
  """Replaces LOG4CXX_INFO with LOG4CXX_WARN in default section of switch"""

  return modify_source(source_tuple,
    r"(?P<begin>default\s*:\s*LOG4CXX_)INFO\(",
    r"\g<begin>WARN\(")

def replace_info_trace(source_tuple):
  """Replaces LOG4CXX_INFO with LOG4CXX_AUTO_TRACE"""

  token = r"[^;{},\s]+"
  const_override = r"(?:(?:const|OVERRIDE)\s*){0,2}"
  signature = r"[\s\w\d,<>:*&]*"
  function = r"%s\s+(%s::)?(?P<name>%s)\s*\(%s\)\s*%s\s*{" % (token, token, token, signature, const_override)
  info = r"LOG4CXX_(TRACE|DEBUG|INFO|WARN|ERROR|FATAL)\((?P<logger>\w*?),\s*\"\w*(::)?(?P=name)(\(\w*\))?\"\);"
  pattern = r"(?P<function>%s\s*)%s" % (function, info)

  repl = r"\g<function>LOG4CXX_AUTO_TRACE(\g<logger>);"

  return modify_source(source_tuple,
    pattern, repl)

def replace_legacy_ext(source_tuple):
  """Replaces LOG4CXX_*_EXT legacy macros"""

  return modify_source(source_tuple,
    r"(?P<log>LOG4CXX_)(?P<type>(TRACE|DEBUG|INFO|WARN|ERROR|FATAL))(?P<ext>(_STR)?_EXT)(?P<brace>\()",
    r"\g<log>\g<type>\g<brace>")

def replace_info(source_tuple):
  """Replaces LOG4CXX_INFO"""

  return replace_info_trace(replace_info_warn(source_tuple))

def swap_trace_lock(source_tuple):
  """Swaps LOG4CXX_AUTO_TRACE and AutoLock
  if they in wrong order and they are neighbors"""

  lock = r"(?:sync_primitives::){0,1}Auto(?:Read|Write){0,1}Lock\s*[^()]+\(.*?\);"
  trace = r"LOG4CXX_AUTO_TRACE\(.*?\);"
  pattern = r"(?P<lock>\n\s*%s)\s*(?P<trace>\n\s*%s)" % (lock, trace)
  return modify_source(source_tuple,
    pattern,
    r"\g<trace>\g<lock>")

def get_files_list(dir):
  """Read list of files in DIR"""

  list = []
  for dirpath, dirnames, filenames in os.walk(dir):
    for file in filenames :
      if os.path.splitext(file)[-1] in file_extensions:
        list.append(os.path.join(dirpath, file))
  return list

def process(file):
  """Processes one file"""

  input_file = file
  output_file = file
  print "File: %-70s" % input_file ,
  f = open(input_file, 'r')
  source = f.read()
  f.close()

  step0 = (source, 0)
  step1 = remove_trace_exit(replace_trace_enter(step0))
  step2 = replace_info(step1)
  step3 = replace_legacy_ext(step2)
  dest_source, count_modifications = swap_trace_lock(step3)

  if count_modifications > 0:
    fo = open(output_file, 'w')
    fo.write(dest_source)
    fo.close()
  print " - modifications: %s" % count_modifications

if __name__ == "__main__":
  arg_parser = ArgumentParser(description='Welcome to the world of AUTO TRACE!')
  arg_parser.add_argument('--dir', required=True, help="name of directory")
  args = arg_parser.parse_args()
  dir = args.dir

  list = get_files_list(dir)
  for file in list : process(file)
  print "Processed files: %s" % len(list)
