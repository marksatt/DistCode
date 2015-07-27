#!/usr/bin/python

# Copyright 2009 Google Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
# USA.

"""Apple gcc/Xcode header map reader"""

__author__ = 'Mark Mentovai'

"""By default, Xcode uses a "header map" to communicate includes to gcc.  The
header map is supposed to be an optimization to reduce repetitive on-disk
searching by the compiler.  A header map is structured essentially as a
hash table, keyed by names used in #includes, and providing pathnames to the
actual files.  Generally speaking, the header maps produced by Xcode provide
access to all of the header files in either a target or a project file.  Xcode
informs the compiler to use one or more header maps by passing an -I or
-iquote argument referencing the header map file instead of a directory.  The
compiler integrates header maps into its normal search order, except when
encountering an -I or -iquote corresponding to a header map, it queries the
header map file instead of searching a directory.

Header maps have spoiled several perfectly good days for me, so I always
disable them in Xcode now by setting USE_HEADERMAP = NO.  However, they're on
by default and the setting to disable them doesn't have any UI, so most
projects wind up relying on the header map, whether they intend to or not.

The header map file format isn't documented publicly as far as I know, but
the format is simple and is understood by Apple gcc.  Relevant source code
is available at:

http://www.opensource.apple.com/darwinsource/DevToolsOct2008/gcc_42-5566/libcpp/include/cpplib.h
http://www.opensource.apple.com/darwinsource/DevToolsOct2008/gcc_42-5566/gcc/c-incpath.c

Also refer to the header map rewriter in the distccd server at
src/xci_headermap.c.
"""

import string
import struct
import sys


def _Endian():
  """Returns a tuple of struct endian modifiers corresponding to native
  endianness ([0]) and other endianness ([1]).
  """
  (test,) = struct.unpack('H', '\1\2')
  if test == 513:
    return ('<', '>')
  elif test == 258:
    return ('>', '<')
  assert False


def _StringAtOffset(strings, offset):
  """Given a table of NUL-terminated strings in |strings|, returns the single
  string at offset |offset|.
  """
  for index in xrange(offset, len(strings)):
    if strings[index] == '\0':
      return strings[offset:index]

  # The string table ended without a NUL terminator, so take everything from
  # |offset|.
  return strings[offset:]


_lowercase_ascii_translate = string.maketrans(string.ascii_uppercase,
                                              string.ascii_lowercase)

def _LowercaseASCII(s):
  """Returns |s| with all uppercase ASCII characters changed to lowercase.
  Similar to string.lower() except it operates only on the ASCII set and is
  not subject to locale influence.
  """
  return string.translate(s, _lowercase_ascii_translate)


class Headermap(object):
  """A Headermap object reads an Xcode/Apple gcc header map and performs
  lookups on it.  The only public calls to Headermap are the constructor and
  the Resolve method.

  Headermap does not read the header map from disk until it is required.  Once
  needed, the entire header map is read and its data cached for subsequent
  uses.

  Headermap intentionally silently ignores errors.  If a header map does not
  exist or is malformed, a Headermap object will quietly return None in
  response to all calls to Resolve.

  Attributes:
    map: A dict mapping names as they would be #included to paths on disk.
    pathname: The on-disk pathname of the header map file.
    read: A bool indicating whether Read has been called yet.
  """
  def __init__(self, pathname):
    self.map = {}
    self.pathname = pathname
    self.read = False

  def _Read(self):
    # Set self.read early.  A failure in this method is final and cannot be
    # overcome by successive calls to Resolve.
    self.read = True

    try:
      hmap_file = None
      try:
        hmap_file = open(self.pathname, 'rb')
        hmap = hmap_file.read()
      finally:
        if hmap_file:
          hmap_file.close()

      (native_endian, other_endian) = _Endian()

      # Figure out endianness of the header map file so that it can be
      # byte-swapped as it is read.  Apple gcc doesn't even do this, it just
      # drops other-endian header maps.  That's weird because it's so easy
      # to get this right.

      # HMAP_SAME_ENDIANNESS_MAGIC
      (native_magic,) = struct.unpack('>L', 'hmap')
      # HMAP_OPPOSITE_ENDIANNESS_MAGIC
      (other_magic,) = struct.unpack('<L', 'hmap')
      (magic,) = struct.unpack(native_endian + 'L', hmap[0:4])

      if magic == native_magic:
        file_endian = native_endian
      elif magic == other_magic:
        file_endian = other_endian
      else:
        # Not a header map of any endianness.
        return

      # struct hmap_header_map
      header_format = file_endian + 'LHHLLLL'
      header_size = struct.calcsize(header_format)

      (magic, version, _reserved, strings_offset,
       count, capacity, max_value_length) = \
          struct.unpack(header_format, hmap[0:header_size])

      if version != 1 or _reserved != 0:
        # Not a recognized header map.  I wouldn't check _reserved but Apple
        # gcc does.
        return

      # Build up dict in new_map instead of self.map so that a failure from
      # this point forward leaves self.map empty.
      new_map = {}

      # struct hmap_bucket
      bucket_format = file_endian + 'LLL'
      bucket_size = struct.calcsize(bucket_format)

      # Header map lookups normally work by hashing the lowercased key and
      # then searching buckets beginning at that hash key index for a key
      # that's a case-insensitive match.  The result is the concatenation of
      # a prefix and suffix value.  Prefixes are typically directories.
      # This allows a space optimization: files in the same directory can share
      # the same prefix in the string table, and buckets can use the same
      # string for their key and suffix.
      #
      # This implementation loads the entire hash table at once and stores
      # the data in an equivalent Python dict.

      for bucket_index in xrange(0, capacity):
        # Look at each bucket in the hash table.
        bucket_offset = header_size + bucket_index * bucket_size
        (key_offset, prefix_offset, suffix_offset) = \
            struct.unpack(bucket_format,
                          hmap[bucket_offset:bucket_offset+bucket_size])

        # HMAP_NOT_A_KEY - Skip empty buckets.
        if key_offset == 0:
          continue

        # Apple gcc does a case-insensitive comparison on keys.  Match that
        # behavior by only storing lowercase keys.  gcc uses a strict ASCII
        # "tolower" for case-insensitivity to avoid locale influence on the
        # key, so use _LowercaseASCII here for the same reason.
        key = _LowercaseASCII(_StringAtOffset(hmap,
                                              strings_offset + key_offset))
        prefix = _StringAtOffset(hmap, strings_offset + prefix_offset)
        suffix = _StringAtOffset(hmap, strings_offset + suffix_offset)

        # Store the key.  The key may already be present in the map if the
        # header map contains duplicate keys.  It seems like that shouldn't
        # happen, but given that case conversion happens when the header map
        # is read and not written, it's actually possible to wind up with
        # duplicates.  When that happens, Apple gcc will only use the first
        # occurrence due to the hash lookup scheme.  To maintain bug
        # compatibility, this does the same, favoring the first occurrence of
        # a key.
        if not key in new_map:
          new_map[key] = prefix + suffix

      # Store the map.
      self.map = new_map

    except Exception:
      # Silently ignore problems.
      pass

  def Resolve(self, key):
    """Looks up |key|, an #include, in the header map.  Returns the path to the
    appropriate header file.  If |key| is not found in the header map or any
    other error occurs (including a missing or malformatted header map), returns
    None.
    """
    if not self.read:
      self._Read()

    # Apple gcc does a case-insensitive comparison on keys by doing a
    # locale-independent ASCII-only lowercasing, so this has to match.
    return self.map.get(_LowercaseASCII(key), None)


class HeadermapCollection(object):
  """A cache of a bunch of cached header maps.

  HeadermapCollection indexes Headermap objects by each header map's pathname.
  It has one public method, Resolve.

  Attributes:
    headermaps: A dict keyed by header map pathnames, providing Headermap
                objects.
  """
  def __init__(self):
    self.headermaps = {}

  def Resolve(self, headermap_path, key):
    """Queries the Headermap at headermap_path for key, returning the resolved
    path or None.
    """
    try:
      hmap = self.headermaps[headermap_path]
    except KeyError:
      hmap = Headermap(headermap_path)
      self.headermaps[headermap_path] = hmap

    return hmap.Resolve(key)


# Keep a global collection around for the include server to use.
global_collection = HeadermapCollection()


def main(args):
  assert len(args) == 1 or len(args) == 2

  headermap = Headermap(args[0])
  if len(args) == 1:
    headermap._Read()
    for key in sorted(headermap.map.keys()):
      print '%s\t%s' % (key, headermap.map[key])
  else:
    path = headermap.Resolve(args[1])
    if path:
      print path

  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
