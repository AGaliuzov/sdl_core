#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Removes LOG4CXX_TRACE_EXIT and
# replaces LOG4CXX_TRACE_ENTER to LOG4CXX_AUTO_TRACE

import os
import re
from os import walk
from sys import argv
from argparse import ArgumentParser

arg_parser = ArgumentParser(description='Welcome in the world of AUTO TRACE!')
arg_parser.add_argument('--dir', required=True, help="name of directory")
args = arg_parser.parse_args()

dir = args.dir

def replace_trace_enter(source):
  regex = re.compile(r"((\n\s*)LOG4CXX_TRACE_ENTER\((.*?)\);)")
  return regex.sub(r"\2LOG4CXX_AUTO_TRACE(\3);", source)

def remove_trace_exit(source):
  regex = re.compile(r"\n\s*LOG4CXX_TRACE_EXIT\(.*?\);")
  return regex.sub(r"", source)

def get_files_list(dir):
  return [os.path.join(dp, f) for dp, dn, filenames in os.walk(dir) \
          for f in filenames
            if os.path.splitext(f)[1] == '.cc' \
              or os.path.splitext(f)[1] == '.cpp' \
              or os.path.splitext(f)[1] == '.hpp' \
              or os.path.splitext(f)[1] == '.h']

def process(file):
  input_file = file
  output_file = file
  print "input: %s" % input_file
  f = open(input_file, 'r')
  source = f.read()
  f.close()

  dest = replace_trace_enter(remove_trace_exit(source))

  fo = open(output_file, 'w')
  fo.write(dest)
  fo.close()
  print "output: %s" % output_file

for file in get_files_list(dir): process(file)
