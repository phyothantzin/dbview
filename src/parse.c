#include "parse.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include <stdio.h>

int create_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == -1) {
    printf("Malloc failed to create db header\n");
    return STATUS_ERROR;
  };

  header->version = 0x1;
  header->count = 0;
  header->magic = HEADER_MAGIC;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;
  return STATUS_SUCCESS;
};

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("Got a bad file descriptor\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == -1) {
    printf("Malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  if (read(fd, header, sizeof(struct dbheader_t)) !=
      sizeof(struct dbheader_t)) {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }

  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->filesize = ntohl(header->filesize);
  header->magic = ntohl(header->magic);

  if (header->version != 1) {
    printf("Improper header version\n");
    free(header);
    return STATUS_ERROR;
  }

  if (header->magic != HEADER_MAGIC) {
    printf("Improper header magic\n");
    free(header);
    return STATUS_ERROR;
  }

  struct stat dbstat = {0};

  fstat(fd, &dbstat);
  if (header->filesize != dbstat.st_size) {
    printf("Corrupted database\n");
    free(header);
    return STATUS_ERROR;
  }

  *headerOut = header;
  return STATUS_SUCCESS;
};

void output_file(int fd, struct dbheader_t *header,
                 struct employee_t *employees) {
  if (fd < 0) {
    printf("Got a bad file descriptor\n");
    return;
  }

  int realCount = header->count;
  header->magic = htonl(header->magic);
  header->filesize =
      htonl(sizeof(struct dbheader_t) + sizeof(struct employee_t) * realCount);
  header->version = htons(header->version);
  header->count = htons(header->count);

  lseek(fd, 0, SEEK_SET);
  write(fd, header, sizeof(struct dbheader_t));

  int i = 0;

  for (; i < realCount; i++) {
    employees[i].hours = htonl(employees[i].hours);
    write(fd, employees, sizeof(struct employee_t));
  }

  return;
};

int read_employees(int fd, struct dbheader_t *header,
                   struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("Got a bad file descriptor\n");
    return STATUS_ERROR;
  }

  int count = header->count;

  struct employee_t *employees = calloc(count, sizeof(struct employee_t));

  if (employees == -1) {
    printf("Malloc failed\n");
    return STATUS_ERROR;
  }

  read(fd, employees, count * sizeof(struct employee_t));

  int i = 0;

  for (; i < count; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }

  *employeesOut = employees;

  return STATUS_SUCCESS;
}

int add_employee(struct dbheader_t *header, struct employee_t *employees,
                 char *addString) {
  char *name = strtok(addString, ",");
  char *addr = strtok(NULL, ",");
  char *hours = strtok(NULL, ",");

  strncpy(employees[header->count - 1].name, name,
          sizeof(employees[header->count - 1].name));
  strncpy(employees[header->count - 1].address, addr,
          sizeof(employees[header->count - 1].address));
  employees[header->count - 1].hours = atoi(hours);

  return STATUS_SUCCESS;
}
