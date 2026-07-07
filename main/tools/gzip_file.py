#!/usr/bin/env python3
"""Compress a single file with gzip at maximum compression. Used by CMake to
pre-compress web assets before embedding them in firmware.

Usage: python gzip_file.py <input> <output>
"""
import gzip
import sys

if len(sys.argv) != 3:
    sys.stderr.write("usage: gzip_file.py <input> <output>\n")
    sys.exit(1)

with open(sys.argv[1], "rb") as src:
    data = src.read()
with open(sys.argv[2], "wb") as dst:
    dst.write(gzip.compress(data, compresslevel=9))
