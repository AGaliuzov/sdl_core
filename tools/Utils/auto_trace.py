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

def replace_trace_enter(source):
  """Replaces LOG4CXX_TRACE_ENTER with LOG4CXX_AUTO_TRACE"""

  regex = re.compile(r"(?P<space>\n\s*)LOG4CXX_TRACE_ENTER\((?P<logger>.*?)\);")
  return regex.sub(r"\g<space>LOG4CXX_AUTO_TRACE(\g<logger>);", source)

def remove_trace_exit(source):
  """Removes LOG4CXX_TRACE_EXIT"""

  regex = re.compile(r"\n\s*LOG4CXX_TRACE_EXIT\(.*?\);")
  return regex.sub(r"", source)

def replace_info_warn(source):
  """Replaces LOG4CXX_INFO with LOG4CXX_WARN in default section of switch"""

  regex = re.compile(r"(?P<begin>default\s*:\s*LOG4CXX_)INFO\(")
  return regex.sub(r"\g<begin>WARN(", source)

def replace_info_trace(source):
  """Replaces LOG4CXX_INFO with LOG4CXX_AUTO_TRACE"""

  token = r"[^;{},\s]+"
  const_override = r"((?:const|OVERRIDE)\s*){0,2}"
  signature = r"[\s\w\d,<>:*&]*"
  function = r"%s\s+(?P<name>%s)\s*\(%s\)\s*%s\s*{" % (token, token, signature, const_override)
  info = r"LOG4CXX_INFO\((?P<logger>.*?),\s*\"(?P=name)\"\);"
  pattern = r"^(?P<function>.*?%s\s*)%s" % (function, info)
  regex = re.compile(pattern, re.DOTALL | re.MULTILINE | re.IGNORECASE)
  repl = r"\g<function>LOG4CXX_AUTO_TRACE(\g<logger>);"
  return regex.sub(repl, source)

def replace_info(source):
  """Replaces LOG4CXX_INFO"""

  return replace_info_trace(replace_info_warn(source))

def swap_trace_lock(source):
  """Swaps LOG4CXX_AUTO_TRACE and AutoLock
  if they in wrong order and they are neighbors"""

  lock = r"(?:sync_primitives::){0,1}Auto(?:Read|Write){0,1}Lock\s*[^()]+\(.*?\);"
  trace = r"LOG4CXX_AUTO_TRACE\(.*?\);"
  pattern = r"(?P<lock>\n\s*%s)\s*(?P<trace>\n\s*%s)" % (lock, trace)
  regex = re.compile(pattern)
  return regex.sub(r"\g<trace>\g<lock>", source)

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
  print "File: %s" % input_file
  f = open(input_file, 'r')
  source = f.read()
  f.close()

  step1 = remove_trace_exit(source)
  step2 = replace_info(replace_trace_enter(step1))
  dest = swap_trace_lock(step2)

  fo = open(output_file, 'w')
  fo.write(dest)
  fo.close()

if __name__ == "__main__":
  arg_parser = ArgumentParser(description='Welcome to the world of AUTO TRACE!')
  arg_parser.add_argument('--dir', required=True, help="name of directory")
  args = arg_parser.parse_args()
  dir = args.dir

  list = get_files_list(dir)
  for file in list : process(file)
  print "Processed files: %s" % len(list)
