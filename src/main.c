#include "common.h"
#include "file.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <databse file>\n", argv[0]);
  printf("\t -n   - create new databse file\n");
  printf("\t -f   - (require) path to dababase file\n");
  return;
}

int main(int argc, char *argv[]) {
  int c;
  bool newfile = false;
  char *filepath = NULL;
  int dbfd = -1;

  while ((c = getopt(argc, argv, "nf:")) != -1) {

    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case '?':
      printf("Unknown option -%c\n", c);
      break;
    default:
      return -1;
    }
  }

  if (filepath == NULL) {
    printf("filepath is the require argument\n");
    print_usage(argv);
    return 0;
  }

  if (newfile) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to create db file\n");
      return -1;
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to open db file\n");
      return -1;
    }
  }

  printf("Newfile: %d\n", newfile);
  printf("filepath: %s\n", filepath);
  return 0;
}
