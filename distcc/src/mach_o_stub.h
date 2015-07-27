/* -*- c-file-style: "java"; indent-tabs-mode: nil; tab-width: 4; fill-column: 7
8 -*-
 * Copyright 2009 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
*/

/* Author: Mark Mentovai */

/* This file contains definitions of structures and constants needed to access
 * Mach-O files and "fat" Mach-O containers.  The authoritative source of this
 * information is the headers supplied with Darwin/Mac OS X.  The definitions
 * in this file are minimal, and sufficient only for the present needs of
 * distcc. */

#ifndef DISTCC_MACH_O_STUB_H_
#define DISTCC_MACH_O_STUB_H_

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/* mach-o/fat.h
 * http://www.opensource.apple.com/darwinsource/DevToolsOct2008/cctools-698.1/include/mach-o/fat.h */

struct fat_header {
  uint32_t magic;
  uint32_t nfat_arch;
};

#define FAT_MAGIC 0xcafebabe

struct fat_arch {
  int32_t /* cpu_type_t */ cputype;
  int32_t /* cpu_type_t */ cpusubtype;
  uint32_t offset;
  uint32_t size;
  uint32_t align;
};

/* mach-o/loader.h
 * http://www.opensource.apple.com/darwinsource/DevToolsOct2008/cctools-698.1/include/mach-o/loader.h */

struct mach_header {
  uint32_t magic;
  int32_t /* cpu_type_t */ cputype;
  int32_t /* cpu_subtype_t */ cpusubtype;
  uint32_t filetype;
  uint32_t ncmds;
  uint32_t sizeofcmds;
  uint32_t flags;
};

#define MH_MAGIC 0xfeedface
#define MH_CIGAM 0xcefaedfe

struct mach_header_64 {
  uint32_t magic;
  int32_t /* cpu_type_t */ cputype;
  int32_t /* cpu_subtype_t */ cpusubtype;
  uint32_t filetype;
  uint32_t ncmds;
  uint32_t sizeofcmds;
  uint32_t flags;
  uint32_t reserved;
};

#define MH_MAGIC_64 0xfeedfacf
#define MH_CIGAM_64 0xcffaedfe

struct load_command {
  uint32_t cmd;
  uint32_t cmdsize;
};

#define LC_SEGMENT 0x1
#define LC_SYMTAB 0x2
#define LC_SEGMENT_64 0x19

struct segment_command {
  uint32_t cmd;
  uint32_t cmdsize;
  char segname[16];
  uint32_t vmaddr;
  uint32_t vmsize;
  uint32_t fileoff;
  uint32_t filesize;
  int32_t /* vm_prot_t */ maxprot;
  int32_t /* vm_prot_t */ initprot;
  uint32_t nsects;
  uint32_t flags;
};

struct segment_command_64 {
  uint32_t cmd;
  uint32_t cmdsize;
  char segname[16];
  uint64_t vmaddr;
  uint64_t vmsize;
  uint64_t fileoff;
  uint64_t filesize;
  int32_t /* vm_prot_t */ maxprot;
  int32_t /* vm_prot_t */ initprot;
  uint32_t nsects;
  uint32_t flags;
};

struct section {
  char sectname[16];
  char segname[16];
  uint32_t addr;
  uint32_t size;
  uint32_t offset;
  uint32_t align;
  uint32_t reloff;
  uint32_t nreloc;
  uint32_t flags;
  uint32_t reserved1;
  uint32_t reserved2;
};

struct section_64 {
  char sectname[16];
  char segname[16];
  uint64_t addr;
  uint64_t size;
  uint32_t offset;
  uint32_t align;
  uint32_t reloff;
  uint32_t nreloc;
  uint32_t flags;
  uint32_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;
};

struct symtab_command {
  uint32_t cmd;
  uint32_t cmdsize;
  uint32_t symoff;
  uint32_t nsyms;
  uint32_t stroff;
  uint32_t strsize;
};

#endif /* DISTCC_MACH_O_STUB_H_ */
