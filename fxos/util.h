//---
//	util: Utility functions
//---

#ifndef FXOS_UTIL
#define FXOS_UTIL

/* integer(): Convert base 8, 10 or 16 into integers
   Prints an error message and sets *error to 1 in case of conversion error or
   overflow.

   @str    Original integer representation ("10", "0x1f", "07")
   @error  Set to 1 on error, otherwise unchanged (can be NULL)
   Returns result of conversion (valid if *error is not 1) */
long long integer(const char *str, int *error);

/* match_table_name(): Some specific matching on filenames
   Returns the table name to associate with the file located at @path for a
   table of type @type, with file suffix @suffix.

   Essentially if the basename of the file at @path is on the form
     {@type}-{x}{@suffix}
   then an malloc'ed copy of x is returned. Otherwise an malloc'ed copy of the
   basename is returned. */
char *match_table_name(char const *path, char const *type, char const *suffix);

/* map(): Map a file to memory
   Maps a file given by its path to memory, and return the associated region,
   a file descriptor and the size.

   @path  File path
   @fd    Will be set to fd of open file
   @size  Will be set to file size
   Returns NULL no error, in which case the file is closed and [*fd] and
   [*size] are undefined; otherwise, returns a pointer to mapped data. */
void *map(char const *path, int *fd, size_t *size);

/* unmap(): Unmap a file loaded with map()
   @data  Region returned by map()
   @fd    File descriptor set by map()
   @size  Size set by map() */
void unmap(void *data, int fd, size_t size);

#endif /* FXOS_UTIL */
