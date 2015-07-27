/* -*- c-file-style: "java"; indent-tabs-mode: nil; tab-width: 4; fill-column: 78 -*-
 *
 * distcc -- A simple distributed compiler system
 *
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

/* Xcode and Apple gcc collude using a "header map" mechanism, whereby Xcode
 * can provide the compiler with mappings from #include directives to actual
 * files that need to be included, without requiring the compiler to search
 * directories in the include path.  A header map is a hash table keyed by
 * names used in #includes, and header map queries return pathnames to the
 * actual files.  The compiler is directed to a header map by passing it an
 * -I or -iquote argument referring to a header map file instead of a
 * directory.  The header map participates in normal include path sequencing,
 * but searching a header map for a file involves a hash table lookup rather
 * than a directory access.
 *
 * For the purposes of distcc, header maps need to be rewritten before being
 * used on the distccd server for two reasons:
 *  - Header map lookups result in absolute paths that are only valid on the
 *    system that created the header map.  These absolute paths must be fixed
 *    to point into the temporary root structure used on the distcc server for
 *    compilation.
 *  - Header maps are always written in native endianness.  Apple gcc
 *    recognizes other-endian header maps but rejects them, even though the
 *    byte-swapping operations would be trivial.  In order to get Apple gcc
 *    to use an other-endian header map, it needs to be rewritten with
 *    its bytes swapped to native endianness.
 *
 * The routines in this file perform both of the above operations on a header
 * map.
 *
 * Header maps have spoiled several perfectly good days for me, so I always
 * disable them in Xcode now by setting USE_HEADERMAP = NO.  However, they're
 * on by default and the setting to disable them doesn't have any UI, so most
 * projects wind up relying on the header map, whether they intend to or not.
 *
 * The header map file format isn't documented publicly as far as I know, but
 * the format is simple and is understood by Apple gcc.  Relevant source code
 * is available at:
 *
 * http://www.opensource.apple.com/darwinsource/DevToolsOct2008/gcc_42-5566/libcpp/include/cpplib.h
 * http://www.opensource.apple.com/darwinsource/DevToolsOct2008/gcc_42-5566/gcc/c-incpath.c
 *
 * Also refer to the header map reader in the distcc include_server at
 * include_server/headermap.py.
 */

#include <config.h>

#ifdef XCODE_INTEGRATION

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "byte_swapping.h"
#include "exitcode.h"
#include "trace.h"
#include "xci.h"

/* hmap_bucket, hmap_header_map, HMAP_MAGIC, and HMAP_NOT_A_KEY come from
 * Apple gcc's version of libcpp/include/cpplib.h. */

struct hmap_bucket {
    uint32_t key;
    struct {
        uint32_t prefix;
        uint32_t suffix;
    } value;
};

struct hmap_header_map {
    uint32_t magic;
    uint16_t version;
    uint16_t _reserved;
    uint32_t strings_offset;
    uint32_t count;
    uint32_t capacity;
    uint32_t max_value_length;
    struct hmap_bucket buckets[0];
};

#define HMAP_MAGIC (((((('h' << 8) | 'm') << 8) | 'a') << 8) | 'p')
#define HMAP_NOT_A_KEY 0

/* |replacement| is a linked list mapping old string offsets to the new
 * string offsets they are being replaced with.  The head element in a
 * list of replacements has old_offset = new_offset = not_offset. */
struct replacement {
    uint32_t old_offset;
    uint32_t new_offset;
    struct replacement *next;
};

static const uint32_t not_offset = (uint32_t)-1;

/* Follows a linked list of |replacement| structs and frees them. */
static void free_replacements(struct replacement *r) {
    struct replacement *next = r;
    while (r) {
        next = r->next;
        free(r);
        r = next;
    }
}

/**
 * Given a header map structure at headermap of size headermap_size,
 * potentially replaces the string at *string_offset within the string pool
 * with a version rooted in root_dir.  Absolute pathnames are replaced with
 * a string that prefixes root_dir to the old value.  If a valid replacement
 * is already found in replacements, it will be used, otherwise, the new
 * string is added to headermap's strings pool, which grows as needed.
 * headermap_capacity reflects the allocated size of the block at headermap.
 * If a replacement is performed, *string_offset is updated.  If the header
 * map's base address, size, or capacity change, *headermap, *headermap_size
 * and *headermap_capacity are adjusted accordingly.  On success, returns 0.
 **/
static int dcc_xci_headermap_fix_string(char **headermap,
                                        size_t *headermap_size,
                                        size_t *headermap_capacity,
                                        uint32_t *string_offset,
                                        const char *root_dir,
                                        struct replacement *replacements) {
    struct hmap_header_map *hmap = (struct hmap_header_map *)*headermap;
    uint32_t hmap_string_offset;
    struct replacement *r, *last_r = replacements;
    size_t new_str_len, new_size, new_capacity;
    char *new_headermap;

    /* string_offset gives an offset within the strings table.  Compute the
     * offset within the headermap file contents. */
    hmap_string_offset = hmap->strings_offset + *string_offset;

    /* If this string does not begin with a '/', it's not an absolute path,
     * and shouldn't be "fixed."  Return the original offset. */
    if ((*headermap + hmap_string_offset)[0] != '/')
        return 0;

    /* Look through the replacements to see if this offset was already seen.
     * If so, just return the already-fixed offset. */
    for (r = replacements; r; r = r->next) {
        if (*string_offset == r->old_offset) {
            *string_offset = r->new_offset;
            return 0;
        }

        last_r = r;
    }

    /* This is a new replacement.  Remember it. */
    if (!(r = malloc(sizeof(struct replacement)))) {
        rs_log_error("malloc() failed: %s", strerror(errno));
        return EXIT_DISTCC_FAILED;
    }

    r->old_offset = *string_offset;
    r->new_offset = *headermap_size - hmap->strings_offset;
    r->next = NULL;
    last_r->next = r;

    /* Expand the header map to accommodate the new string.  It needs to grow
     * by the length of the existing string plus the length of the new root
     * prefix plus one, for the terminating NUL. */
    new_str_len = strlen(root_dir) + strlen(*headermap + hmap_string_offset);
    new_size = *headermap_size + new_str_len + 1;

    /* The allocated region may need to grow.  It's increased in blocks to
     * avoid doing too many reallocs on a potentially large region, but if
     * the block is too small it'll be increased to fit the new string
     * exactly. */
    new_capacity = *headermap_capacity;
    if (new_size > new_capacity) {
        new_capacity += 1024;
        if (new_size > new_capacity)
            new_capacity = new_size;
    }

    if (new_capacity > *headermap_capacity) {
        if (!(new_headermap = realloc(*headermap, new_capacity))) {
            rs_log_error("realloc() failed: %s", strerror(errno));
            return EXIT_DISTCC_FAILED;
        }
        /* Give the callee the new region and capacity. */
        *headermap = new_headermap;
        *headermap_capacity = new_capacity;

        hmap = (struct hmap_header_map *)*headermap;
    }

    /* Give the callee the new size and update the string table offset to
     * point to the string that's about to be stored. */
    *headermap_size = new_size;
    *string_offset = r->new_offset;

    /* Put the new string into the header map. */
    snprintf(*headermap + hmap->strings_offset + r->new_offset, new_str_len + 1,
             "%s%s", root_dir, *headermap + hmap_string_offset);

    /* Update the string count in the header map's header.  Apple gcc doesn't
     * use this field, but Xcode sets it properly. */
    ++hmap->count;

    return 0;
}

/**
 * Given a header map at headermap_path, "rehomes" the header map's contents
 * so that any referenced absolute paths point to the same path within
 * root_dir.  If the header map is other-endian, it will be rewritten to be
 * native-endian.  (Apple gcc does not understand other-endian header maps.)
 * Returns 0 on success, or on a "soft" failure, such as when an unrecognized
 * header map format is present.
 **/
int dcc_xci_headermap_fix(const char *headermap_path,
                          const char *root_dir) {
    int ret = EXIT_DISTCC_FAILED;
    FILE *headermap_file = NULL;
    char *headermap = NULL;
    size_t headermap_size, headermap_original_size, headermap_capacity;
    struct hmap_header_map *hmap;
    int swap = 0;
    struct replacement replacements = {not_offset, not_offset, NULL};
    uint32_t bucket_index;
    struct hmap_bucket *bucket;
    uint32_t key_offset, prefix_offset, suffix_offset, str_offset;
    size_t value_len;

    /* Open the header map. */
    if (!(headermap_file = fopen(headermap_path, "r+"))) {
        rs_log_error("fopen(\"%s\") failed: %s",
                     headermap_path, strerror(errno));
        goto leave;
    }

    if (!(headermap = dcc_xci_read_whole_file(headermap_file,
                                              &headermap_size))) {
        rs_log_error("dcc_xci_read_whole_file(\"%s\") failed", headermap_path);
        goto leave;
    }

    /* Remember the header map's original size, because it might grow but
     * offsets found in the header map should be validated against the original
     * value. */
    headermap_original_size = headermap_size;

    /* headermap_capacity is the allocated size of the headermap memory
     * region. */
    headermap_capacity = headermap_size;

    /* Validate the header map's header. */
    if (headermap_size < sizeof(struct hmap_header_map)) {
        /* Warn and leave, but don't signal failure. */
        rs_log_warning("\"%s\" is too small to be a header map",
                       headermap_path);
        ret = 0;
        goto leave;
    }

    hmap = (struct hmap_header_map *)headermap;

    if (hmap->magic != HMAP_MAGIC) {
        if (swap_32(hmap->magic) != HMAP_MAGIC) {
            /* Warn and leave, but don't signal failure. */
            rs_log_warning("\"%s\" doesn't have that header map magic",
                           headermap_path);
            ret = 0;
            goto leave;
        }

        /* Other-endian magic: everything needs to be swapped. */
        swap = 1;
    }

    /* Swap the header map's header if needed. */
    if (swap) {
        hmap->magic = swap_32(hmap->magic);
        hmap->version = swap_16(hmap->version);
        hmap->_reserved = swap_16(hmap->_reserved);
        hmap->strings_offset = swap_32(hmap->strings_offset);
        hmap->count = swap_32(hmap->count);
        hmap->capacity = swap_32(hmap->capacity);
        hmap->max_value_length = swap_32(hmap->max_value_length);
    }

    /* Validate the header. */
    if (hmap->version != 1 || hmap->_reserved != 0) {
        /* Warn and leave, but don't signal failure. */
        rs_log_error("\"%s\" has unknown header map version", headermap_path);
        ret = 0;
        goto leave;
    }

    if (headermap_size < hmap->strings_offset) {
        rs_log_error("\"%s\" pool lies outside the file", headermap_path);
        goto leave;
    }

    if (sizeof(struct hmap_header_map) +
        hmap->capacity * sizeof(struct hmap_bucket) > hmap->strings_offset) {
        rs_log_error("\"%s\" has buckets in the pool", headermap_path);
        goto leave;
    }

    /* Walk through the buckets. */
    for (bucket_index = 0; bucket_index < hmap->capacity; ++bucket_index) {
        bucket = &hmap->buckets[bucket_index];
        if (swap) {
            bucket->key = swap_32(bucket->key);
            bucket->value.prefix = swap_32(bucket->value.prefix);
            bucket->value.suffix = swap_32(bucket->value.suffix);
        }

        if (bucket->key == HMAP_NOT_A_KEY)
            continue;

        key_offset = hmap->strings_offset + bucket->key;
        prefix_offset = hmap->strings_offset + bucket->value.prefix;
        suffix_offset = hmap->strings_offset + bucket->value.suffix;

        if (key_offset >= headermap_original_size ||
            prefix_offset >= headermap_original_size ||
            suffix_offset >= headermap_original_size) {
            rs_log_error("\"%s\" bucket %d has string outside of the pool",
                         headermap_path, bucket_index);
            goto leave;
        }

        /* Always check the prefix string, which is always an absolute path
         * in header maps written by Xcode. */
        str_offset = bucket->value.prefix;
        if (dcc_xci_headermap_fix_string(&headermap, &headermap_size,
                                         &headermap_capacity,
                                         &str_offset, root_dir,
                                         &replacements)) {
            rs_log_error("Couldn't fix prefix in \"%s\"", headermap_path);
            goto leave;
        }

        /* headermap may have moved, recalculate hmap and bucket.
         * prefix_offset may be different if a new string was allocated. */
        hmap = (struct hmap_header_map *)headermap;
        bucket = &hmap->buckets[bucket_index];
        bucket->value.prefix = str_offset;
        prefix_offset = hmap->strings_offset + bucket->value.prefix;

        if (headermap[prefix_offset] == '\0') {
            /* In the event that the prefix string is empty, check the suffix
             * string.  This never happens with Xcode-generated header maps. */
            str_offset = bucket->value.suffix;
            if (dcc_xci_headermap_fix_string(&headermap, &headermap_size,
                                             &headermap_capacity,
                                             &str_offset, root_dir,
                                             &replacements)) {
                rs_log_error("Couldn't fix suffix in \"%s\"", headermap_path);
                goto leave;
            }

            /* headermap may have moved, recalculate hmap and bucket.
             * suffix_offset may be different if a new string was allocated. */
            hmap = (struct hmap_header_map *)headermap;
            bucket = &hmap->buckets[bucket_index];
            bucket->value.suffix = str_offset;
            suffix_offset = hmap->strings_offset + bucket->value.suffix;
        }

        /* Figure out how long the length of the value string, the
         * concatenation of the prefix and suffix, would be.  If necessary,
         * update the maximum value length in the header map's header.  Apple
         * gcc doesn't use this field, but Xcode sets it properly. */
        value_len = strlen(headermap + prefix_offset) +
                    strlen(headermap + suffix_offset);
        if (value_len > hmap->max_value_length)
            hmap->max_value_length = value_len;
    }

    if (swap || headermap_size > headermap_original_size) {
        /* Changes were made.  The file needs to be rewritten.  Go back to the
         * beginning of the file. */
        if (fseek(headermap_file, 0, SEEK_SET) == -1) {
            rs_log_error("fseek() failed for \"%s\": %s",
                         headermap_path, strerror(errno));
            goto leave;
        }

        if (dcc_xci_write(headermap_file, headermap, headermap_size)) {
            rs_log_error("dcc_xci_write() failed for \"%s\"", headermap_path);
            goto leave;
        }

        rs_log_info("rewrote header map \"%s\"%s%s",
                    headermap_path,
                    swap ? ", byte-swapped" : "",
                    (headermap_size > headermap_original_size) ?
                        ", fixed paths" : "");
    }

    /* Great success. */
    ret = 0;

  leave:
    if (replacements.next)
        free_replacements(replacements.next);
    if (headermap)
        free(headermap);
    if (headermap_file)
        fclose(headermap_file);

    return ret;
}

#endif /* XCODE_INTEGRATION */
