#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import re
from datetime import datetime
from optparse import OptionParser

def main ():
  usage = "%prog --input FILE --output FILE [--gap NUMBER]"
  parser = OptionParser(usage)
  parser.add_option("--input", "-i", action = "store", dest = "input", metavar = "FILE", help = "input file")
  parser.add_option("--output", "-o", action = "store", dest = "output", metavar = "FILE", help = "output file")
  parser.add_option("--gap", "-t", action = "store", dest = "gap", default = 100, metavar = "NUMBER", help = "gap in milliseconds (100 by default)")

  (options, args) = parser.parse_args()

  if not options.input:
    parser.error("input file not specified")
  if not options.output:
    parser.error("output file not specified")

  input_file = open(options.input, 'r')
  output_file = open(options.output, 'w')

  first_line = input_file.readline()
  if not first_line: # empty file
    return
  previous_time = line_to_time(first_line)
  output_file.write(first_line)

  for line in input_file:
    try:
      time = line_to_time(line)
      gap = time - previous_time
      gap_in_milliseconds = 1000 * gap.total_seconds()
      if gap_in_milliseconds >= options.gap:
        output_file.write("\n") # this is why we do all this
      previous_time = time
    except: # some JSON messages may come on separate lines - they cannot be parsed
      pass
    output_file.write(line)

  input_file.close()
  output_file.close()

def line_to_time (string): # convert time entry in log string to time struct
  substrings = re.split(r"(?:\[|\]){1}", string) # split by [ and ]
  time_string = substrings[1] # corresponds current logger layout
  time_struct = datetime.strptime(time_string, "%d %b %Y %H:%M:%S,%f") # corresponds current logger time format
  return time_struct

if __name__ == "__main__":
  main()
