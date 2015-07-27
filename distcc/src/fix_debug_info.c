/* -*- c-file-style: "java"; indent-tabs-mode: nil; tab-width: 4; fill-column: 78 -*-
 * Copyright 2007, 2009 Google Inc.
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

/* Authors: Fergus Henderson, Mark Mentovai */

/*
 * fix_debug_info.c:
 * Performs search-and-replace in the debug info section of an ELF or Mach-O
 * file.
 */

#include <config.h>

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_ELF_H
  #include <elf.h>
#else
  /* This aids in identifying ELF files when <elf.h> is not available, even
   * though they won't be processable. */
  #define ELFMAG "\177ELF"
  #define SELFMAG 4
#endif

#ifdef __APPLE__
  #include <mach-o/fat.h>
  #include <mach-o/loader.h>
#else
  #include "mach_o_stub.h"
#endif

#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
  #include <sys/mman.h>
#endif

#include "byte_swapping.h"
#include "trace.h"
#include "fix_debug_info.h"

/* XINDEX isn't defined everywhere, but where it is, it's always the
 * same as HIRESERVE, so I think this should be safe.
*/
#ifndef SHN_XINDEX
  #define SHN_XINDEX  SHN_HIRESERVE
#endif

/*
 * Map the specified file into memory with MAP_SHARED.
 * Returns the mapped address, and stores the file descriptor in @p p_fd.
 * It also fstats the file and stores the results in @p st.
 * Logs an error message and returns NULL on failure.
 */
static void *mmap_file(const char *path, int *p_fd, struct stat *st) {
  int fd;
  void *base;

  fd = open(path, O_RDWR);
  if (fd < 0) {
    rs_log_error("error opening file '%s': %s", path, strerror(errno));
    return NULL;
  }

  if (fstat(fd, st) != 0) {
    rs_log_error("fstat of file '%s' failed: %s", path, strerror(errno));
    close(fd);
    return NULL;
  }

  if (st->st_size <= 0) {
    rs_log_error("file '%s' has invalid file type or size", path);
    close(fd);
    return NULL;
  }

#ifdef HAVE_MMAP
  base = mmap(NULL, st->st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (base == MAP_FAILED) {
    rs_log_error("mmap of file '%s' failed: %s", path, strerror(errno));
    close(fd);
    return NULL;
  }
#else
  base = malloc(st->st_size);
  if (base == NULL) {
    rs_log_error("can't allocate buffer for %s: malloc failed", path);
    close(fd);
    return NULL;
  }
  errno = 0;
  if (read(fd, base, st->st_size) != st->st_size) {
    /* TODO: read in a loop that handles EINTR. */
    rs_log_error("can't read %ld bytes from %s: %s", (long) st->st_size, path,
                 strerror(errno));
    close(fd);
    return NULL;
  }
#endif
  *p_fd = fd;
  return base;
}

static int munmap_file(void *base, const char *path, int fd,
                const struct stat *st) {
  int status = 0;
#ifdef HAVE_MMAP
  if (munmap(base, st->st_size) != 0) {
    rs_log_error("munmap of file '%s' failed: %s", path, strerror(errno));
    status = 1;
  }
#else
  errno = 0;
  if (lseek(fd, 0, SEEK_SET) == -1) {
    rs_log_error("can't seek to start of %s: %s", path, strerror(errno));
    status = 1;
  } else if (write(fd, base, st->st_size) != st->st_size) {
    /* TODO: write in a loop that handles EINTR. */
    rs_log_error("can't write %ld bytes to %s: %s", (long) st->st_size, path,
                 strerror(errno));
    status = 1;
  }
#endif
  if (close(fd) != 0) {
    rs_log_error("close of file '%s' failed: %s", path, strerror(errno));
    status = 1;
  }
  return status;
}

/*
 * Search in a memory buffer (starting at @p base and of size @p size)
 * for a string (@p search), and replace @p search with @p replace
 * in all null-terminated strings that contain @p search.
 */
static int replace_string(void *base, size_t size,
                           const char *search, const char *replace) {
  char *start = (char *) base;
  char *end = (char *) base + size;
  int count = 0;
  char *p;
  size_t search_len = strlen(search);
  size_t replace_len = strlen(replace);

  assert(replace_len == search_len);

  if (size < search_len + 1)
    return 0;
  for (p = start; p < end - search_len - 1; p++) {
    if ((*p == *search) && !memcmp(p, search, search_len)) {
      memcpy(p, replace, replace_len);
      count++;
    }
  }
  return count;
}

#ifdef HAVE_ELF_H
/*
 * Search for an ELF section of the specified name and type.
 * Given an ELF file that has been mmapped (or read) into memory starting
 * at @p elf_mapped_base, find the section with the desired name and type,
 * and return (via the parameters) its start point and size.
 * Returns 1 if found, 0 otherwise.
 */
static int FindElfSection(const void *elf_mapped_base, off_t elf_size,
                           const char *desired_section_name,
                           const void **section_start, int *section_size) {
  const unsigned char *elf_base = (const unsigned char *) elf_mapped_base;
  /* The double cast below avoids warnings with -Wcast-align. */
  const Elf32_Ehdr *elf32_header = (const Elf32_Ehdr *) (const void *) elf_base;
  unsigned int i;
  unsigned int num_sections;

  assert(elf_mapped_base);
  assert(section_start);
  assert(section_size);

  *section_start = NULL;
  *section_size = 0;

  /*
   * There are two kinds of ELF files, 32-bit and 64-bit.  They have similar
   * but slightly different file structures.  It's OK to use the elf32_header
   * structure at this point, prior to checking whether this file is a 32 or
   * 64 bit ELF file, so long as we only access the e_ident field, because
   * the layout of the e_ident field is the same for both kinds: it's the
   * first field in the struct, so its offset is zero, and its size is the
   * same for both 32 and 64 bit ELF files.
   *
   * The ELF file layouts are defined using fixed-size data structures
   * in <elf.h>, so we don't need to worry about the host computer's
   * word size.  But we do need to worry about the host computer's
   * enddianness, because ELF header fields use the same endianness
   * as the target computer.  When cross-compiling to a target with
   * a different endianness, we would need to byte-swap all the fields
   * that we use.  Right now we don't handle that case.
   *
   * TODO(fergus):
   * handle object files with different endianness than the host.
   */
#if WORDS_BIGENDIAN
  if (elf32_header->e_ident[EI_DATA] != ELFDATA2MSB) {
    rs_trace("sorry, not fixing debug info: "
            "distcc server host is big-endian, object file is not");
    return 0;
  }
#else
  if (elf32_header->e_ident[EI_DATA] != ELFDATA2LSB) {
    rs_trace("sorry, not fixing debug info: "
            "distcc server host is little-endian, object file is not");
    return 0;
  }
#endif

  /*
   * Warning: the following code section is duplicated:
   * once for 32-bit ELF files, and again for 64-bit ELF files.
   * Please be careful to keep them consistent!
   */
  if (elf32_header->e_ident[EI_CLASS] == ELFCLASS32) {
    const Elf32_Ehdr *elf_header = elf32_header;
    const Elf32_Shdr *sections =
        /* The double cast below avoids warnings with -Wcast-align. */
        (const Elf32_Shdr *) (const void *) (elf_base + elf_header->e_shoff);
    const Elf32_Shdr *string_section = sections + elf_header->e_shstrndx;
    const Elf32_Shdr *desired_section = NULL;

    if (elf_size < (off_t) sizeof(*elf_header)) {
      rs_trace("object file is too small for ELF header; maybe got truncated?");
      return 0;
    }
    if (elf_header->e_shoff <= 0 ||
        elf_header->e_shoff > elf_size - sizeof(Elf32_Shdr)) {
      rs_trace("invalid e_shoff value in ELF header");
      return 0;
    }
    if (elf_header->e_shstrndx == SHN_UNDEF) {
      rs_trace("object file has no section name string table"
               " (e_shstrndx == SHN_UNDEF)");
      return 0;
    }
    /* Special case for more sections than will fit in e_shstrndx. */
    if (elf_header->e_shstrndx == SHN_XINDEX) {
      string_section = sections + sections[0].sh_link;
    }
    num_sections = elf_header->e_shnum;
    /* Special case for more sections than will fit in e_shnum. */
    if (num_sections == 0) {
      num_sections = sections[0].sh_size;
    }
    for (i = 0; i < num_sections; ++i) {
      const char *section_name = (char *)(elf_base +
                                          string_section->sh_offset +
                                          sections[i].sh_name);
      if (!strcmp(section_name, desired_section_name)) {
        desired_section = &sections[i];
        break;
      }
    }
    if (desired_section != NULL && desired_section->sh_size > 0) {
      int desired_section_size = desired_section->sh_size;
      *section_start = elf_base + desired_section->sh_offset;
      *section_size = desired_section_size;
      return 1;
    } else {
      return 0;
    }
  } else if (elf32_header->e_ident[EI_CLASS] == ELFCLASS64) {
    /* The double cast below avoids warnings with -Wcast-align. */
    const Elf64_Ehdr *elf_header = (const Elf64_Ehdr *) (const void *) elf_base;
    const Elf64_Shdr *sections =
        (const Elf64_Shdr *) (const void *) (elf_base + elf_header->e_shoff);
    const Elf64_Shdr *string_section = sections + elf_header->e_shstrndx;
    const Elf64_Shdr *desired_section = NULL;

    if (elf_size < (off_t) sizeof(*elf_header)) {
      rs_trace("object file is too small for ELF header; maybe got truncated?");
      return 0;
    }
    if (elf_header->e_shoff <= 0 ||
        elf_header->e_shoff > (size_t) elf_size - sizeof(Elf64_Shdr)) {
      rs_trace("invalid e_shoff value in ELF header");
      return 0;
    }
    if (elf_header->e_shstrndx == SHN_UNDEF) {
      rs_trace("object file has no section name string table"
               " (e_shstrndx == SHN_UNDEF)");
      return 0;
    }
    /* Special case for more sections than will fit in e_shstrndx. */
    if (elf_header->e_shstrndx == SHN_XINDEX) {
      string_section = sections + sections[0].sh_link;
    }
    num_sections = elf_header->e_shnum;
    if (num_sections == 0) {
      /* Special case for more sections than will fit in e_shnum. */
      num_sections = sections[0].sh_size;
    }
    for (i = 0; i < num_sections; ++i) {
      const char *section_name = (char*)(elf_base +
                                         string_section->sh_offset +
                                         sections[i].sh_name);
      if (!strcmp(section_name, desired_section_name)) {
        desired_section = &sections[i];
        break;
      }
    }
    if (desired_section != NULL && desired_section->sh_size > 0) {
      int desired_section_size = desired_section->sh_size;
      *section_start = elf_base + desired_section->sh_offset;
      *section_size = desired_section_size;
      return 1;
    } else {
      return 0;
    }
  } else {
    rs_trace("unknown ELF class - neither ELFCLASS32 nor ELFCLASS64");
    return 0;
  }
}

/*
 * Update the ELF file residing at @p path, replacing all occurrences
 * of @p search with @p replace in the section named @p desired_section_name.
 * The replacement string must be the same length or shorter than
 * the search string.
 */
static void update_elf_section(const char *path,
                               const void *base,
                               off_t size,
                               const char *desired_section_name,
                               const char *search,
                               const char *replace) {
  const void *desired_section = NULL;
  int desired_section_size = 0;

  if (FindElfSection(base, size, desired_section_name,
                     &desired_section, &desired_section_size)
      && desired_section_size > 0) {
    /* The local variable below works around a bug in some versions
     * of gcc (4.2.1?), which issues an erroneous warning if
     * 'desired_section_rw' is replaced with '(void *) desired_section'
     * in the call below, causing compile errors with -Werror.
     */
    void *desired_section_rw = (void *) desired_section;
    int count = replace_string(desired_section_rw, desired_section_size,
                               search, replace);
    if (count == 0) {
      rs_trace("\"%s\" section of file %s has no occurrences of \"%s\"",
               desired_section_name, path, search);
    } else {
      rs_log_info("updated \"%s\" section of file \"%s\": "
                  "replaced %d occurrences of \"%s\" with \"%s\"",
                  desired_section_name, path, count, search, replace);
      if (count > 1) {
        rs_log_warning("only expected to replace one occurrence!");
      }
    }
  } else {
    rs_trace("file %s has no \"%s\" section", path, desired_section_name);
  }
}
#endif /* HAVE_ELF_H */

/*
 * Update the ELF file residing at @p path, replacing all occurrences
 * of @p search with @p replace in that file's ".debug_info" or
 * ".debug_str" section.
 * The replacement string must be the same length or shorter than
 * the search string.
 * Returns 0 on success (whether or not ".debug_info" section was
 * found or updated).
 * Returns 1 on serious error that should cause distcc to fail.
 */
static void update_debug_info_elf(const char *path,
                                  const void *base, off_t size,
                                  const char *search, const char *replace) {
#ifndef HAVE_ELF_H
  /* Avoid warning about unused arguments. */
  (void) base;
  (void) size;
  rs_trace("no <elf.h>, so can't change %s to %s in debug info for %s",
           search, replace, path);
#else /* HAVE_ELF_H */
  update_elf_section(path, base, size, ".debug_info", search, replace);
  update_elf_section(path, base, size, ".debug_str", search, replace);
#endif /* HAVE_ELF_H */
}

/**
 * Locates a Mach-O section in a mapped Mach-O file.  mapped_base and
 * mapped_size identify the mapped region of a thin Mach-O file, which may
 * be a region within in a larger mapped fat file.  section_start and
 * section_size will be set to the subregion within the Mach-O file for the
 * corresponding section.  If want_symtab is true, this function will look
 * look for a region for the symbol table's string table.  If want_symtab
 * is false, this function will look for a section whose name matches
 * desired_section_name within a segment whose name matches
 * desired_segment_name.  The swap argument identifies whether the
 * Mach-O file needs to be byte-swapped, and macho_bits must be set to
 * 32 or 64 depending on the architecture that the Mach-O file was created
 * for.
 *
 * Returns 1 on success and 0 on failure.
 **/
static int FindMachOSection(const void *base, off_t size,
                            int want_symtab,
                            const char *desired_segment_name,
                            const char *desired_section_name,
                            int swap, int macho_bits,
                            const void **section_start, int *section_size) {
  assert(base);
  assert(section_start);
  assert(section_size);

  const uint8_t *mapped_base = (const uint8_t *)base;
  const uint8_t *mapped_end = mapped_base + size;

  if (mapped_end < mapped_base) {
    rs_trace("object file has to be kidding");
    return 0;
  }

  *section_start = NULL;
  *section_size = 0;

  /* Figure out how many load commands there are. */
  uint32_t ncmds;
  uint32_t sizeofcmds;
  const struct load_command *load_command_base;
  const uint8_t *load_command_end;

  if (macho_bits == 32) {
    const struct mach_header* header = (const struct mach_header*)mapped_base;
    if ((size_t)size < sizeof(*header)) {
      rs_trace("object file is too small for Mach-O 32 header");
      return 0;
    }

    ncmds = maybe_swap_32(swap, header->ncmds);
    sizeofcmds = maybe_swap_32(swap, header->sizeofcmds);
    load_command_base = (struct load_command*)(mapped_base + sizeof(*header));
    load_command_end = mapped_base + sizeof(*header) + sizeofcmds - 1;
  } else {
    const struct mach_header_64* header =
        (const struct mach_header_64*)mapped_base;
    if ((size_t)size < sizeof(*header)) {
      rs_trace("object file is too small for Mach-O 64 header");
      return 0;
    }

    ncmds = maybe_swap_32(swap, header->ncmds);
    sizeofcmds = maybe_swap_32(swap, header->sizeofcmds);
    load_command_base = (struct load_command*)(mapped_base + sizeof(*header));
    load_command_end = mapped_base + sizeof(*header) + sizeofcmds;
  }

  if (load_command_end > mapped_end ||
      load_command_end < (const uint8_t *)load_command_base) {
    rs_trace("object file is too small for load commands");
    return 0;
  }

  /* Look at each load command.  For any identifying a segment, walk through
   * all of the contained sections until finding a matching one.  If we're
   * looking for the symbol table's string table instead, skip segment
   * load commands but look for the symbol table. */
  const struct load_command *command = load_command_base;
  uint32_t cumulative_cmdsizes = 0;
  uint32_t command_index;
  for (command_index = 0; command_index < ncmds; ++command_index) {
    uint32_t cmd = maybe_swap_32(swap, command->cmd);
    uint32_t cmdsize = maybe_swap_32(swap, command->cmdsize);

    uint32_t new_cumulative_cmdsizes = cumulative_cmdsizes + cmdsize;
    if (new_cumulative_cmdsizes > sizeofcmds ||
        new_cumulative_cmdsizes < cumulative_cmdsizes) {
      rs_trace("load command size exceeds declared size of all commands");
      return 0;
    }
    cumulative_cmdsizes = new_cumulative_cmdsizes;

    uint32_t sect_off;
    uint32_t sect_size;
    int found_sect = 0;

    if (!want_symtab) {
      /* If we don't want the symbol table, we want a segment/section.
       * For the segment checks, don't check the name supplied in the
       * segment_command's segname field.  Object (.o) files, which are the
       * only type of file processed here, only contain a single segment
       * for compactness; the segment has a blank segname and the real
       * segment names are taken from each section header. */
      if (cmd == LC_SEGMENT) {
        if (macho_bits != 32) {
          rs_trace("32-bit segment command found in non-32-bit file");
          return 0;
        }
        const struct segment_command* sc =
            (const struct segment_command*)command;
        uint32_t nsects = maybe_swap_32(swap, sc->nsects);

        const struct section* sect_base =
            (const struct section*)((const uint8_t*)sc + sizeof(*sc));
        if (sizeof(*sc) + sizeof(*sect_base) * nsects > cmdsize) {
          rs_trace("32-bit segment command overflows space allocated for it");
          return 0;
        }

        uint32_t sect_index;
        for (sect_index = 0; sect_index < nsects; ++sect_index) {
          if (!strncmp(sect_base[sect_index].sectname, desired_section_name,
                       sizeof(sect_base[sect_index].sectname)) &&
              !strncmp(sect_base[sect_index].segname, desired_segment_name,
                       sizeof(sect_base[sect_index].segname))) {
            sect_size = maybe_swap_32(swap, sect_base[sect_index].size);
            sect_off = maybe_swap_32(swap, sect_base[sect_index].offset);
            found_sect = 1;
            break;
          }
        }
      } else if (cmd == LC_SEGMENT_64) {
        if (macho_bits != 64) {
          rs_trace("64-bit segment command found in non-64-bit file");
          return 0;
        }
        const struct segment_command_64* sc =
            (const struct segment_command_64*)command;
        uint32_t nsects = maybe_swap_32(swap, sc->nsects);

        const struct section_64* sect_base =
            (const struct section_64*)((const uint8_t*)sc + sizeof(*sc));
        if (sizeof(*sc) + sizeof(*sect_base) * nsects > cmdsize) {
          rs_trace("64-bit segment command overflows space allocated for it");
          return 0;
        }

        uint32_t sect_index;
        for (sect_index = 0; sect_index < nsects; ++sect_index) {
          if (!strncmp(sect_base[sect_index].sectname, desired_section_name,
                       sizeof(sect_base[sect_index].sectname)) &&
              !strncmp(sect_base[sect_index].segname, desired_segment_name,
                       sizeof(sect_base[sect_index].sectname))) {
            uint64_t sect_size_64 =
                maybe_swap_64(swap, sect_base[sect_index].size);

            if (sect_size_64 > UINT32_MAX) {
              rs_trace("a section can't possibly be this big");
              return 0;
            }
            sect_size = (uint32_t)sect_size_64;

            sect_off = maybe_swap_32(swap, sect_base[sect_index].offset);
            found_sect = 1;
            break;
          }
        }
      }
    } else if (cmd == LC_SYMTAB) {
      /* We're looking for the symbol table and we found it.  The symbol table
       * isn't really a section, but nobody really cares. */
      const struct symtab_command* sc = (const struct symtab_command*)command;

      sect_off = maybe_swap_32(swap, sc->stroff);
      sect_size = maybe_swap_32(swap, sc->strsize);
      found_sect = 1;
    }

    if (found_sect) {
      const uint8_t *section_base = mapped_base + sect_off;
      const uint8_t *section_end = section_base + sect_size;
      if (section_base < mapped_base ||
          section_end > mapped_end || section_end < section_base) {
        rs_trace("object file is too small for section");
        return 0;
      }

      /* If you've made it this far, you win a prize. */
      *section_start = (const void*)section_base;
      *section_size = sect_size;
      return 1;
    }

    command = (const struct load_command*)((const uint8_t*)command + cmdsize);
  }

  return 0;
}

/**
 * This function operates on a thin Mach-O file mapped in the region beginning
 * at |base|, of |size| bytes.  It will locate a subregion to operate on by
 * calling FindMachOSection with the relevant arguments, and within that
 * region, will replace all occurrences of |search| with |replace|.
 **/
static void update_macho_section(const char *path,
                                 const void *base,
                                 off_t size,
                                 int want_symtab,
                                 const char *desired_segment_name,
                                 const char *desired_section_name,
                                 const char *search,
                                 const char *replace,
                                 int swap, int macho_bits) {
  /* Allocate a sections struct just for access to the size of the segname
   * and sectname fields.  Allocate region_name based on these values, plus
   * space for a comma, the extra string " section", and a NUL terminator.
   * This should be large enough to cover a description of the desired
   * region whether want_symtab is true or false. */
  struct section a_sect;
  char region_name[sizeof(a_sect.segname) + sizeof(a_sect.sectname) + 10];
  const void *section = NULL;
  int section_size = 0;

  if (want_symtab) {
    strncpy(region_name, "LC_SYMTAB string table", sizeof(region_name));
    region_name[sizeof(region_name) - 1] = '\0';
  } else {
    snprintf(region_name, sizeof(region_name), "%s,%s section",
             desired_segment_name, desired_section_name);
  }

  if (!FindMachOSection(base, size,
                        want_symtab, desired_segment_name, desired_section_name,
                        swap, macho_bits, &section, &section_size)) {
    rs_trace("file \"%s\" has no %s", path, region_name);
    return;
  }

  void *section_rw = (void*)section;
  int count = replace_string(section_rw, section_size, search, replace);
  if (count == 0) {
    rs_trace("%s of file \"%s\" has no occurences of \"%s\"",
             region_name, path, search);
  } else {
    rs_log_info("updated %s of file \"%s\": "
                "replaced %d occurrence%s of of \"%s\" with \"%s\"",
                region_name, path, count, (count == 1) ? "" : "s",
                search, replace);
  }
}

/**
 * This function finds portions of a Mach-O file in the region identified by
 * |base| and |size| that might contain debug information pathnames, and
 * within those subregions, replaces |search| with |replace|.
 **/
static void update_debug_info_macho_thin(const char *path,
                                         const void *base, off_t size,
                                         const char *search,
                                         const char *replace) {
  if ((size_t)size < sizeof(uint32_t)) {
    rs_trace("object file has no magic, blacklisted by Magician's Alliance?");
    return;
  }

  uint32_t magic = *(uint32_t*)base;

  int swap = 0;
  if (magic == MH_CIGAM || magic == MH_CIGAM_64)
    swap = 1;

  int macho_bits = 32;
  if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64)
    macho_bits = 64;

  /* At this point in the program, there's no information that might help
   * determine if DWARF or STABS debugging information was generated, or even
   * if any debugging information was generated at all.  Look for debugging
   * information everywhere it might be hiding. */

  /* Fix pathnames in the relevant DWARF sections for DWARF debugging mode. */
  update_macho_section(path, base, size, 0, "__DWARF", "__debug_info",
                       search, replace, swap, macho_bits);
  update_macho_section(path, base, size, 0, "__DWARF", "__debug_str",
                       search, replace, swap, macho_bits);

  /* Fix pathnames in the symbol table's string table for STABS debugging
   * mode. */
  update_macho_section(path, base, size, 1, "", "",
                       search, replace, swap, macho_bits);
}

/**
 * |base| and |size| identify a mapped region corresponding to a "fat"
 * (multiple architecture) Mach-O file.  Within the fat file,
 * update_debug_info_macho_thin will be called for each contained
 * architecture's "thin" (single architecture) Mach-O image.
 **/
static void update_debug_info_macho_fat(const char *path,
                                        const void *base, off_t size,
                                        const char *search,
                                        const char *replace) {
  const uint8_t *mapped_base = (const uint8_t*)base;
  const uint8_t *mapped_end = mapped_base + size;

  if (mapped_end < mapped_base) {
    rs_trace("object file can't possibly be serious");
    return;
  }

  if ((size_t)size < sizeof(struct fat_header)) {
    rs_trace("object file too small for fat header");
    return;
  }

  const struct fat_header *fat = (const struct fat_header *)base;
  uint32_t nfat_arch = swap_big_to_cpu_32(fat->nfat_arch);

  if (sizeof(*fat) + sizeof(struct fat_arch) * nfat_arch > (size_t)size) {
    rs_trace("object file too small for all of this fat");
    return;
  }

  const struct fat_arch *fat_arch_base =
      (const struct fat_arch *)(mapped_base + sizeof(*fat));

  uint32_t fat_index;
  for (fat_index = 0; fat_index < nfat_arch; ++fat_index) {
    uint32_t arch_off = swap_big_to_cpu_32(fat_arch_base[fat_index].offset);
    uint32_t arch_size = swap_big_to_cpu_32(fat_arch_base[fat_index].size);

    if (arch_size == 0) {
      rs_trace("empty architecture");
      continue;
    }

    const uint8_t *arch_base = mapped_base + arch_off;
    const uint8_t *arch_end = arch_base + arch_size;

    if (arch_base < mapped_base ||
        arch_end > mapped_end || arch_end < arch_base) {
      rs_trace("arch is too big for its fat, if you would believe that");
      return;
    }

    update_debug_info_macho_thin(path, arch_base, arch_size, search, replace);
  }
}

/**
 * update_debug_info scans the file at |path| for debug information sections
 * that might contain pathnames, and within those sections, replaces |search|
 * with |replace|.  This function can identify and operate on Mach-O files of
 * any endianness and bitting, including fat files, while running on any
 * system.  It can identify ELF files, but can only operate on like-endian
 * ELF files, and only on systems with native ELF support.
 **/
static int update_debug_info(const char *path, const char *search,
                             const char *replace) {
  struct stat st;
  int fd;
  void *base;

  base = mmap_file(path, &fd, &st);
  if (base == NULL) {
    return 0;
  }

  if (st.st_size >= SELFMAG && memcmp(base, ELFMAG, SELFMAG) == 0) {
    /* The magic number which identifies an ELF file is stored in the
     * first few bytes of the e_ident field, which is also the first few
     * bytes of the file. */
    update_debug_info_elf(path, base, st.st_size, search, replace);
  } else if ((size_t)st.st_size >= sizeof(uint32_t) &&
             (*(uint32_t*)base == MH_MAGIC || *(uint32_t*)base == MH_CIGAM ||
              *(uint32_t*)base == MH_MAGIC_64 ||
              *(uint32_t*)base == MH_CIGAM_64)) {
    /* The magic number for thin Mach-O files is the first four bytes of
     * the file, and is stored in the native endianness of the system for
     * which the binary was produced. */
    update_debug_info_macho_thin(path, base, st.st_size, search, replace);
  } else if ((size_t)st.st_size >= sizeof(uint32_t) &&
             swap_big_to_cpu_32(*(uint32_t*)base) == FAT_MAGIC) {
    /* The magic number for fat Mach-O files is the first four bytes of
     * the file, and is always stored big-endian. */
    update_debug_info_macho_fat(path, base, st.st_size, search, replace);
  } else {
    rs_trace("unknown object file format");
  }

  return munmap_file(base, path, fd, &st);
}

/*
 * Edit the ELF or Mach-O file residing at @p path, changing all occurrences
 * of the path @p server_path to @p client_path in the debugging info.
 *
 * We're a bit sloppy about that; rather than properly parsing
 * the DWARF debug info, finding the DW_AT_comp_dir (compilation working
 * directory) field and the DW_AT_name (source file name) field,
 * we just do a search-and-replace in the ".debug_info" and ".debug_str"
 * sections.  But this is good enough.
 *
 * Returns 0 on success (whether or not the ".debug_info" and ".debug_str"
 * sections were found or updated).
 * Returns 1 on serious error that should cause distcc to fail.
 */
int dcc_fix_debug_info(const char *path, const char *client_path,
                              const char *server_path)
{
  /*
   * We can only safely replace a string with another of exactly
   * the same length.  (Replacing a string with a shorter string
   * results in errors from gdb.)
   * So we replace the server path with the client path, and fill the remaining
   * portion with slashes to keep the sizes the same for the search-and-replace.
   * For example, with |client_path| "/a" and |server_path| "/tmp/distccd", any
   * occurrence of "/tmp/distccd" in the relevant sections of the file at
   * |path| will become "/a//////////", which ought to be interpreted the same
   * as the desired |client_path| "/a" on the client.
   */
  size_t client_path_len = strlen(client_path);
  size_t server_path_len = strlen(server_path);
  assert(client_path_len <= server_path_len);
  char *client_path_plus_slashes = malloc(server_path_len + 1);
  if (!client_path_plus_slashes) {
    rs_log_crit("failed to allocate memory");
    return 1;
  }
  strcpy(client_path_plus_slashes, client_path);
  while (client_path_len < server_path_len) {
    client_path_plus_slashes[client_path_len++] = '/';
  }
  client_path_plus_slashes[client_path_len] = '\0';
  rs_log_info("client_path_plus_slashes = %s", client_path_plus_slashes);
  return update_debug_info(path, server_path, client_path_plus_slashes);
}

#ifdef TEST
const char *rs_program_name;

int main(int argc, char **argv) {
  rs_program_name = argv[0];
  rs_add_logger(rs_logger_file, RS_LOG_DEBUG, NULL, STDERR_FILENO);
  rs_trace_set_level(RS_LOG_DEBUG);
  if (argc != 4) {
    rs_log_error("Usage: %s <filename> <client-path> <server-path>",
                 rs_program_name);
    exit(1);
  }
  return dcc_fix_debug_info(argv[1], argv[2], argv[3]);
}
#endif
