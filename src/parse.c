#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    if(dbhdr == NULL || employees == NULL){
        printf("Invalid parameters to list_employees\n");
        return;
    }

    for (int i = 0; i < dbhdr->count; i++)
    {
        print_employee(i, &employees[i]);
    }
}

void print_employee(int i, struct employee_t *employee){
    printf("Employee %d\n", i);
    printf("\tName: %s\n", employee->name);
    printf("\tAddress: %s\n", employee->address);
    printf("\tHours: %d\n", employee->hours);
}


int query_employees(struct dbheader_t *dbhdr, struct employee_t *employees, char *queryString) {

    char* column = strtok(queryString, "=");
    char* value = strtok(NULL, "=");
    if(value == NULL || column == NULL){
        printf("Invalid query string\n");
        return STATUS_ERROR;
    }

    for (int i = 0; i < dbhdr->count; i++)
    {
        if(strcmp(column, "name") == 0){
            if(strstr(employees[i].name, value) != NULL){
                print_employee(i, &employees[i]);
            }
        }
        else if(strcmp(column, "address") == 0){
            if(strstr(employees[i].address, value) != NULL){
                print_employee(i, &employees[i]);
            }
        }
        else if(strcmp(column, "hours") == 0){
            if(employees[i].hours == (unsigned int)atoi(value)){
                print_employee(i, &employees[i]);
            }
        }
        else{
            printf("Invalid query column\n");
            return STATUS_ERROR;
        }
    }
    return STATUS_SUCCESS;
}


int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring) {
    if(addstring == NULL || employees == NULL || dbhdr == NULL){
        printf("Invalid parameters to add_employee\n");
        return STATUS_ERROR;
    }
    
    dbhdr->count++;
    if (*employees == NULL) {
        *employees = calloc(dbhdr->count, sizeof(struct employee_t));
    } 
    else {
        *employees = realloc(*employees, dbhdr->count * sizeof(struct employee_t));
    }

    struct employee_t* empArray = *employees;

    char* name = strtok(addstring, ",");
    char* addr = strtok(NULL, ",");
    char* hours = strtok(NULL, ",");

    if(name == NULL || addr == NULL || hours == NULL){
        printf("Invalid add string\n");
        return STATUS_ERROR;
    }

    strncpy(empArray[dbhdr->count - 1].name, name, sizeof(empArray[dbhdr->count - 1].name));
    strncpy(empArray[dbhdr->count - 1].address, addr, sizeof(empArray[dbhdr->count - 1].address));
    
    empArray[dbhdr->count - 1].hours = atoi(hours);

    return STATUS_SUCCESS;
}

int remove_employees_by_name(struct dbheader_t *dbhdr, struct employee_t **employees, char* employeeToRemove){

    struct employee_t *newEmployees = NULL;
    struct employee_t *currentEmployees = *employees;
    int newCount = 0;

    for (int i = 0; i < dbhdr->count; i++)
    {
        if(strcmp(currentEmployees[i].name, employeeToRemove) == 0)
        {
            continue;
        }
        
        newCount++;

        newEmployees = realloc(newEmployees, sizeof(struct employee_t) * newCount);
        if(newEmployees == NULL){
            printf("Malloc failed\n");
            return STATUS_ERROR;
        }

        strncpy(newEmployees[newCount - 1].name, currentEmployees[i].name, sizeof(currentEmployees[i].name));
        strncpy(newEmployees[newCount - 1].address, currentEmployees[i].address, sizeof(currentEmployees[i].address));
        newEmployees[newCount - 1].hours = currentEmployees[i].hours;
    }

    if(newCount == dbhdr->count){
        printf("No employee found to remove with name: %s\n", employeeToRemove);
    }
    else{
        dbhdr->count = newCount;
        free(*employees);
        *employees = newEmployees; 
    }
    return STATUS_SUCCESS;
}

int update_employee_by_name(struct dbheader_t *dbhdr, struct employee_t *employees, char *updateString){
    char* name = strtok(updateString, ",");
    char* addr = strtok(NULL, ",");
    char* hours = strtok(NULL, ",");

    for (int i = 0; i < dbhdr->count; i++)
    {
        if(strcmp(employees[i].name, name) == 0)
        {
            strncpy(employees[i].address, addr, sizeof(employees[i].address));
            employees[i].hours = atoi(hours);
            return STATUS_SUCCESS;
        }
    }

    printf("No employee found to update with name: %s\n", name);
    return STATUS_SUCCESS;
    
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if(fd < 0){
        printf("Got a bad fd from the user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;

    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if(employees == NULL){
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    read(fd, employees, count * sizeof(struct employee_t));

    for(int i = 0; i < count; i++){
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *header, struct employee_t *employees) {
    if(fd < 0){
        printf("Got a bad fd from the user\n");
        return STATUS_ERROR;
    }

    int realcount = header->count;
    int newFileSize = sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount);

    header->magic = htonl(header->magic);
    header->filesize = htonl(newFileSize);
    header->count = htons(header->count);
    header->version = htons(header->version);

    lseek(fd, 0, SEEK_SET);
    ftruncate(fd, newFileSize);

    if(write(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)){
        perror("write");
        return STATUS_ERROR;
    }

    for (int i = 0; i < realcount; i++)
    {
        employees[i].hours = htonl(employees[i].hours);
        if(write(fd, &employees[i], sizeof(struct employee_t)) != sizeof(struct employee_t))
        {
            perror("write");
            return STATUS_ERROR;
        }
    }

    return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if(fd < 0){
        printf("Got a bad fd from the user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if(header == NULL){
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    if(read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)){
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if(header->magic != HEADER_MAGIC){
        printf("Improper header magic\n");
        free(header);
        return STATUS_ERROR;
    }
    
    if(header->version != 1){
        printf("Improper header version\n");
        free(header);
        return STATUS_ERROR;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if(header->filesize != dbstat.st_size){
        printf("Corrupted database\n");
        free(header);
        return STATUS_ERROR;
    }

    *headerOut = header;
    return STATUS_SUCCESS;
}

int create_db_header(struct dbheader_t **headerOut) {
	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if(header == NULL){
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);
    
    *headerOut = header;
    
    return STATUS_SUCCESS;
}


