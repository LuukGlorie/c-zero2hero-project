#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file> -a <add string> -l -r <name to remove> -u <name to update and hours>\n", argv[0]);
    printf("\t -n   - create new database file\n");
    printf("\t -f   - (required) path to database file\n");
    printf("\t -a   - add new employe\n");
	printf("\t -r   - remove an employee by name\n");
	printf("\t -u   - update employee hours by name\n");
    printf("\t -l   - list all employees\n");
}

int main(int argc, char *argv[]) { 
	char *filepath = NULL;
	char *addstring = NULL;
	char *removeName = NULL;
	char *updateString = NULL;
	char *queryString = NULL;
	unsigned short port = 0;
	bool newfile = false;
	bool list = false;
	int c;

	int dbfd = -1;
	struct dbheader_t *header = NULL;
	struct employee_t *employees = NULL;

	while ((c = getopt(argc, argv, "nf:a:lr:u:q:")) != -1) {
		switch (c) {
			case 'n':
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 'a':
				addstring = optarg;
				break;
			case 'l':
				list = true;
				break;
			case 'r':
				removeName = optarg;
				break;
			case 'u':
				updateString = optarg;
				break;
			case 'q':
				queryString = optarg;
				break;
			case '?':
				printf("Unknown option -%c\n", c);
				break;
			default:
				return -1;
		}
	}

    if(filepath == NULL){
        printf("Filepath is a required argument\n");
        print_usage(argv);
        return 0;
    }

	if(newfile){
		dbfd = create_db_file(filepath);
		if(dbfd == STATUS_ERROR){
			printf("Unable to create database file\n");
			return -1;
		}
		if(create_db_header(&header) == STATUS_ERROR){
			printf("Failed to create database header\n");
			return -1;
		}
	}
	else{
		dbfd = open_db_file(filepath);
		if(dbfd == STATUS_ERROR){
			printf("Unable to open database file\n");
			return -1;
		}
		if(validate_db_header(dbfd, &header) == STATUS_ERROR){
			printf("Failed to validate database header\n");
			return -1;
		}
	}

	if(read_employees(dbfd, header, &employees) != STATUS_SUCCESS){
		printf("Failed to read employees\n");
		return -1;
	}

	if(addstring){
		header->count++;
		if (employees == NULL) {
        	employees = calloc(header->count, sizeof(struct employee_t));
    	} 
		else {
        	employees = realloc(employees, header->count * sizeof(struct employee_t));
		}

		add_employee(header, employees, addstring);
	}
	else if (removeName)
	{
		if(remove_employees_by_name(header, &employees, removeName) != STATUS_SUCCESS){
			printf("Failed to remove employees");
			return -1;
		}
	}
	else if (updateString)
	{
		if(update_employee_by_name(header, employees, updateString) != STATUS_SUCCESS){
			printf("Failed to update employee");
			return -1;
		}
	}
	

	if(list){
		list_employees(header, employees);
	}
	else if (queryString)
	{
		printf("Querying for: %s\n", queryString);
		query_employees(header, employees, queryString);
	}

	if(output_file(dbfd, header, employees) == STATUS_ERROR){
		printf("Failed to output database file");
		return -1;
	}
	free(header);

    return 0;
}
