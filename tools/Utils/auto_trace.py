#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Removes LOG4CXX_TRACE_EXIT and
   replaces LOG4CXX_TRACE_ENTER to LOG4CXX_AUTO_TRACE

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
  """Replaces LOG4CXX_TRACE_ENTER to LOG4CXX_AUTO_TRACE"""

  regex = re.compile(r"((\n\s*)LOG4CXX_TRACE_ENTER\((.*?)\);)")
  return regex.sub(r"\2LOG4CXX_AUTO_TRACE(\3);", source)

def remove_trace_exit(source):
  """Removes LOG4CXX_TRACE_EXIT"""

  regex = re.compile(r"\n\s*LOG4CXX_TRACE_EXIT\(.*?\);")
  return regex.sub(r"", source)

def swap_trace_lock(source):
  """Swaps LOG4CXX_AUTO_TRACE and AutoLock
  if they in wrong order and they are neighbors"""

  lock = r"(sync_primitives::){0,1}AutoLock\s*[^()]+\(.*?\);"
  trace = r"LOG4CXX_AUTO_TRACE\(.*?\);"
  pattern = r"((\n\s*%s)\s*(\n\s*%s))" % (lock, trace)
  regex = re.compile(pattern)
  return regex.sub(r"\4\2", source)

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

  dest = swap_trace_lock(replace_trace_enter(remove_trace_exit(source)))

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
