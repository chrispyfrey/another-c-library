/*
Copyright 2019 Andy Curtis

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _ac_io_H
#define _ac_io_H

#include "ac_common.h"
#include <inttypes.h>

typedef int ac_io_format_t;

ac_io_format_t ac_io_delimiter(int delim);
ac_io_format_t ac_io_fixed(int size);
ac_io_format_t ac_io_prefix();

typedef struct {
  char *record;
  uint32_t length;
  int32_t tag;
} ac_io_record_t;

size_t ac_io_file_size(const char *filename);

/* char *ac_io_read_file(size_t *len, const char *filename); */

#ifdef _AC_DEBUG_MEMORY_
#define ac_io_read_file(len, filename)                                         \
  _ac_io_read_file(len, filename, AC_FILE_LINE_MACRO("ac_io_read_file"))
char *_ac_io_read_file(size_t *len, const char *filename, const char *caller);
#else
#define ac_io_read_file(len, filename) _ac_io_read_file(len, filename)
char *_ac_io_read_file(size_t *len, const char *filename);
#endif

/*
  Make the given directory if it doesn't already exist.  Return false if an
  error occurred.
*/
bool ac_io_make_directory(const char *path);

/*
   This will take a full filename and temporarily place a \0 in place of the
   last slash (/).  If there isn't a slash, then it will return true. Otherwise,
   it will check to see if path exists and create it if it does not exist.  If
   an error occurs, false will be returned.
*/
bool ac_io_make_path_valid(char *filename);

/*
  test if filename has extension, extension is expected to have . (ex - ".lz4")
  If filename is NULL, false will be returned.
*/
bool ac_io_extension(const char *filename, const char *extension);

#endif
