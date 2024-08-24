#if __STDC_VERSION__ < 200000L
#  error C11 or later is required. \
         Please add -std=c11 or later to your compiler flags.
#endif

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/// ============ Chores of preprocessor directives ============

#if __STDC_VERSION__ < 202300L
#  include <stdbool.h>
#endif

#ifdef __APPLE__

#  define OFF_T_FORMAT  "lld"
#  define st_mtimefield st_mtimespec

#else
#  ifdef __linux__

#    define OFF_T_FORMAT  "ld"
#    define st_mtimefield st_mtim

#  else
#    error "Unsupported platform"
#  endif // __linux__
#endif   // __APPLE__

/// ============ CLI Implementation ============

/// Only keyword arguments i.e. options are contained.
typedef struct Arguments {
  // re-ordered to make compiler happy
  struct timespec modified_after;
  off_t lower_bound;
  off_t upper_bound;
  bool all;
  bool ordered;
  bool recursive;
  bool has_lower_bound;
  bool has_upper_bound;
  bool has_recent_limit;
} Arguments;

static Arguments _g_args;

typedef enum ArgumentError {
  ARGUMENT_ERROR_NONE = 0,
  ARGUMENT_ERROR_UNKNOWN_OPTION,
  ARGUMENT_ERROR_MISSING_ARGUMENT,
  ARGUMENT_ERROR_INVALID,
} ArgumentError;

__attribute__((noreturn)) void
print_usage(const ArgumentError error, const int fault_pos, const char **argv) {
  switch (error) {
    case ARGUMENT_ERROR_UNKNOWN_OPTION:
      fprintf(stderr, "%s: Unknown option: %s\n", argv[0], argv[fault_pos]);
      break;
    case ARGUMENT_ERROR_MISSING_ARGUMENT:
      fprintf(
          stderr, "%s: Missing argument for %s\n", argv[0], argv[fault_pos]
      );
      break;
    case ARGUMENT_ERROR_INVALID:
      fprintf(
          stderr, "%s: Argument error for %s: %s\n", argv[0],
          argv[fault_pos - 1], argv[fault_pos]
      );
      break;
    case ARGUMENT_ERROR_NONE:
      break;
  }

  fprintf(
      stderr,
      "\033[4mUsage:\033[0m %s [OPTIONS] [PATHS]...\n"
      "\n"
      "\033[4mOptions:\033[0m\n"
      "  -a         Show hidden files\n"
      "  -l <SIZE>  Only show files with size >= <SIZE> bytes\n"
      "  -h <SIZE>  Only show files with size <= <SIZE> bytes\n"
      "  -m <DAYS>  Only show files modified within the last <DAYS> days\n"
      "  -o         Order files by name alphabetically\n"
      "  -r         Recursively list files\n"
      "  -H         Print help\n"
      "  -v         Print version\n",
      argv[0]
  );

  exit((int)error);
}

__attribute__((noreturn)) void print_version(void) {
  printf("\033[1mlist 0.1.0\033[0m\n");
  puts("Copyright (C) 2023 fa_555 <fa_555@foxmail.com>");
  // puts("Source available at ");
  exit(0);
}

#define try_parse(_dest, _pos, _argv)                                      \
  do {                                                                     \
    const int __pos = (_pos);                                              \
    const char **__argv = (_argv);                                         \
    char *endptr = NULL;                                                   \
    errno = 0;                                                             \
    *(_dest) =                                                             \
        _Generic(*(_dest), int: strtol, long: strtol, long long: strtoll)( \
            __argv[__pos], &endptr, 10                                     \
        );                                                                 \
    if (errno != 0)                                                        \
      print_usage(ARGUMENT_ERROR_INVALID, __pos, __argv);                  \
  } while (false)

int parse_arguments(const int argc, const char **argv) {
  Arguments args;
  args = (Arguments){0};

  int i = 1;
  for (; i < argc; ++i) {
    if (argv[i][0] != '-')
      goto end;
    if (strcmp(argv[i], "--") == 0) {
      ++i;
      goto end;
    }

    if (strcmp(argv[i], "-a") == 0)
      args.all = true;
    else if (strcmp(argv[i], "-r") == 0)
      args.recursive = true;
    else if (strcmp(argv[i], "-o") == 0)
      args.ordered = true;
    else if (strcmp(argv[i], "-v") == 0)
      print_version();
    else if (strcmp(argv[i], "-H") == 0) {
      print_usage(ARGUMENT_ERROR_NONE, i, argv);
    } else if (strcmp(argv[i], "-l") == 0) {
      if (i + 1 >= argc)
        print_usage(ARGUMENT_ERROR_MISSING_ARGUMENT, i, argv);

      args.has_lower_bound = true;
      try_parse(&args.lower_bound, ++i, argv);
    } else if (strcmp(argv[i], "-h") == 0) {
      if (i + 1 >= argc)
        print_usage(ARGUMENT_ERROR_MISSING_ARGUMENT, i, argv);

      args.has_upper_bound = true;
      try_parse(&args.upper_bound, ++i, argv);
    } else if (strcmp(argv[i], "-m") == 0) {
      if (i + 1 >= argc)
        print_usage(ARGUMENT_ERROR_MISSING_ARGUMENT, i, argv);

      args.has_recent_limit = true;
      int recent_days;
      try_parse(&recent_days, ++i, argv);

      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      args.modified_after = (struct timespec){
          .tv_sec = now.tv_sec - recent_days * 86400,
          .tv_nsec = now.tv_nsec,
      };
    } else
      print_usage(ARGUMENT_ERROR_UNKNOWN_OPTION, i, argv);
  }

end:
  _g_args = args;
  return i;
}

/// ============ List Implementation ============

void print_dir_entry(off_t size, const char *path, const char *end) {
  printf("%12" OFF_T_FORMAT "   \033[34m%s\033[0m%s\n", size, path, end);
}

void print_non_dir_entry(off_t size, const char *path) {
  printf("%12" OFF_T_FORMAT "   %s\n", size, path);
}

int filter_all_entry(const struct dirent *entry) {
  // Not considering . and ..
  if (!_g_args.all && entry->d_name[0] == '.')
    return false;

  return true;
}

int filter_file(const struct stat *st) {
  if (_g_args.has_lower_bound && st->st_size < _g_args.lower_bound)
    return false;
  if (_g_args.has_upper_bound && st->st_size > _g_args.upper_bound)
    return false;
  if (_g_args.has_recent_limit &&
      st->st_mtimefield.tv_sec < _g_args.modified_after.tv_sec)
    return false;

  return true;
}

void list_dirent(const struct dirent *entry, const char *path, const char *sep);

void list(const char *path, const bool root) {
  struct stat st;
  if (stat(path, &st) == -1) {
    perror(path);
    return;
  }

  if (!S_ISDIR(st.st_mode)) {
    if (filter_file(&st))
      print_non_dir_entry(st.st_size, path);
    return;
  }

  DIR *dir = opendir(path);
  if (dir == NULL) {
    perror(path);
    return;
  }

  const char *sep = *path == '/' && path[1] == '\0' ? "" : "/";
  print_dir_entry(st.st_size, path, sep);

  if (!root && !_g_args.recursive)
    goto end;

  if (!_g_args.ordered) {
    for (struct dirent *entry; (entry = readdir(dir)) != NULL;)
      list_dirent(entry, path, sep);
    goto end;
  }

  struct dirent **namelist;
  int n = scandir(path, &namelist, filter_all_entry, alphasort);
  if (n == -1) {
    perror("scandir");
    return;
  }
  for (int i = 0; i < n; ++i) {
    list_dirent(namelist[i], path, sep);
    free(namelist[i]);
  }
  free(namelist);

end:
  closedir(dir);
}

void list_dirent(
    const struct dirent *entry, const char *path, const char *sep
) {
  if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    return;
  if (!filter_all_entry(entry))
    return;

  char new_path[4096];
  snprintf(new_path, sizeof(new_path), "%s%s%s", path, sep, entry->d_name);
  list(new_path, false);
}

/// ============ Utility & Main ============

void trim_slash(char *path) {
  int i = 0;
  while (path[i])
    ++i;
  while (i > 1 && path[i - 1] == '/')
    --i;
  path[i] = '\0';
}

int main(int argc, char **argv) {
  int first_positional_index = parse_arguments(argc, (const char **)argv);

  if (first_positional_index == argc) {
    list(".", true);
    return 0;
  }

  for (int i = first_positional_index; i < argc; ++i) {
    trim_slash(argv[i]);
    list(argv[i], true);
  }
}
