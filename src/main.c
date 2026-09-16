#include "common.h"
#include "file.h"
#include "parse.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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
  char *addString = NULL;
  int dbfd = -1;
  struct dbheader_t *header = NULL;
  struct employee_t *employees = NULL;

  while ((c = getopt(argc, argv, "nf:a:")) != -1) {

    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case 'a':
      addString = optarg;
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

    if (create_db_header(dbfd, &header) == STATUS_ERROR) {
      printf("Failed to create database header\n");
      return -1;
    };
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to open db file\n");
      return -1;
    }

    if (validate_db_header(dbfd, &header) == STATUS_ERROR) {
      printf("Failed to validate database header\n");
      return -1;
    }
  }

  if (read_employees(dbfd, header, &employees) != STATUS_SUCCESS) {
    printf("Failed to read employees\n");
    return -1;
  };

  if (addString) {
    header->count++;

    employees = realloc(employees, header->count * (sizeof(struct employee_t)));
    add_employee(header, employees, addString);
  }

  output_file(dbfd, header, employees);

  return 0;
}
