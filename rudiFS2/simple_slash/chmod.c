#define _GNU_SOURCE


#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/resource.h>
#include <pwd.h>


#ifndef ALLOWED_CHAR_LENGTH
#define ALLOWED_CHAR_LENGTH 5000
#endif

#ifndef DEFAULT_FOLDER_PERMISSIONS
#define DEFAULT_FOLDER_PERMISSIONS 0755
#endif

typedef int bool;
#define true 1
#define false 0

#ifndef FAKE_ROOT_ID
#define FAKE_ROOT_ID 1001
#endif


void strip_line_endings(char* input) {
	/* strip of /r and /n chars to take care of Windows, Unix, OSx line endings. */
	input[strcspn(input, "\r\n")] = 0;
}


void resolve_path(char* path)
{
	char resolved_path[ALLOWED_CHAR_LENGTH];
	realpath(path, resolved_path);
	strcpy(path, resolved_path);
}

void check_path_length(char* path) {
	if (strlen(path) >= PATH_MAX) //4096 bytes
	{
		errno = E2BIG;
		perror("Path has to be less than 4096 bytes!");
		exit(-1);
	}
}

char** splitline(char *line, char* delimiter)
{
	int buffer = 64;
	int pos = 0;
	char **args = malloc(buffer * sizeof(char*));
	char *token;

	token = strtok(line, delimiter);
	while (token != NULL) {
		args[pos] = token;
		pos += 1;

		if (pos >= buffer) {
			buffer += buffer;
			args = realloc(args, buffer * sizeof(char*));
		}

		token = strtok(NULL, delimiter);
	}
	args[pos] = NULL;
	return args;
}



int check_groups(char* username, char* group_permission) {

	FILE * file_pointer;
	file_pointer = fopen ("/home/naman/Desktop/rudiFS2/simple_slash/.groups", "r");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	int return_val = -1;
	while ((read = getline(&line, &len, file_pointer)) != -1) {

		char** line_break;
		line_break = splitline(line, ";");

		strip_line_endings(line_break[0]);

		if (strcmp(group_permission, line_break[0]) == 0)
		{

			char** users_break;
			users_break = splitline(line_break[1], ",");

			int i = 0;
			while (users_break[i] != NULL) {
				strip_line_endings(users_break[i]);

				if (strcmp(username, users_break[i]) == 0)
				{
					return_val = 1;
					break;
				}
				i++;
			}
		}
	}

	if (line) {
		free(line);
	}
	fclose(file_pointer);

	return return_val;
}


bool check_DAC(char* username, char* path) {

	FILE * file_pointer;
	file_pointer = fopen ("/home/naman/Desktop/rudiFS2/simple_slash/.permissions", "r");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	int return_val = -1;
	while ((read = getline(&line, &len, file_pointer)) != -1) {

		char** line_break;
		line_break = splitline(line, ";");

		strip_line_endings(line_break[0]);
		if (strcmp(path, line_break[0]) == 0)
		{
			char** user_permissions;
			user_permissions = splitline(line_break[1], ",");

			int i = 0;
			while (user_permissions[i] != NULL) {
				strip_line_endings(user_permissions[i]);
				if (strcmp(username, user_permissions[i]) == 0)
				{
					return_val = 1;
					break;
				}
				i++;
			}

			char** group_permissions;
			group_permissions = splitline(line_break[2], ",");

			i = 0;
			while (group_permissions[i] != NULL) {
				strip_line_endings(group_permissions[i]);

				//now check all groups against .groups file.
				if (return_val != 1)
				{
					return_val = check_groups(username, group_permissions[i]);
				}

				i++;
			}
		}
	}

	if (line) {
		free(line);
	}
	fclose(file_pointer);

	return return_val;
}


void write_permissions(char* path, char* username, char* group_permissions) {

	FILE * file_pointer;
	file_pointer = fopen ("/home/naman/Desktop/rudiFS2/simple_slash/.permissions", "a+");
	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char file_perms[ALLOWED_CHAR_LENGTH];

	strcpy(file_perms, path);

	//users
	strcat(file_perms, ";");
	strcat(file_perms, username);

	//groups
	strcat(file_perms, ";fakeroot,");
	strcat(file_perms, username);
	strcat(file_perms, ",");
	strcat(file_perms, group_permissions);
	strcat(file_perms, "\n");


	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	bool append = true;
	while ((read = getline(&line, &len, file_pointer)) != -1) {

		// strip_line_endings(line);
		if (strcmp(line, file_perms) == 0)
		{
			append = false;
			break;
		}
	}

	if (line) {
		free(line);
	}

	if (append == true)
	{
		char *usage = "Usage %s, [options] ... ";
		fprintf(file_pointer, file_perms, usage);
		fclose(file_pointer);
	}
}


int file_exists(char *path) {
	struct stat st;
	int result = stat(path, &st);
	return result;
}


void cat(char* path, char* content) {

	FILE * file_pointer;
	file_pointer = fopen (path, "r");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char* line = NULL;
	size_t len = 0;
	ssize_t read;

	char temp[ALLOWED_CHAR_LENGTH];

	int line_number = 0;
	while ((read = getline(&line, &len, file_pointer)) != -1) {
		if (line_number > 0)
		{
			// printf("%s", line);
			strcat(temp, line);
		}
		line_number++;
	}

	if (line) {
		free(line);
	}
	fclose(file_pointer);

	strcpy(content, temp);
}

char* get_owner(char* path) {

	FILE * file_pointer;
	file_pointer = fopen ("/home/naman/Desktop/rudiFS2/simple_slash/.permissions", "r");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	int return_val = -1;
	while ((read = getline(&line, &len, file_pointer)) != -1) {

		char** line_break;
		line_break = splitline(line, ";");

		strip_line_endings(line_break[0]);
		if (strcmp(path, line_break[0]) == 0)
		{
			strip_line_endings(line_break[1]);
			return line_break[1];
			break;
		}
	}

	if (line) {
		free(line);
	}
	fclose(file_pointer);

	return "-";
}

void modify_permissions(char* path, char* content, char* owner) {

	FILE* file_pointer = fopen ("/home/naman/Desktop/rudiFS2/simple_slash/.permissions", "a+");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}


	printf("Enter new permissions: ");
	char new_permissions[ALLOWED_CHAR_LENGTH];
	fgets(new_permissions, ALLOWED_CHAR_LENGTH, stdin);
	strip_line_endings(new_permissions);

	char perms[ALLOWED_CHAR_LENGTH];
	strcpy(perms, path);
	strcat(perms, ";");
	strcat(perms, owner);
	strcat(perms, ";fakeroot,");
	strcat(perms, new_permissions);
	strcat(perms, "\n");

	char *usage = "Usage %s, [options] ... ";
	fprintf(file_pointer, perms, usage);

	// printf("Permissions Updated! Clesydanup Reqd.\n");
	fclose(file_pointer);
}


int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		resolve_path(argv[1]);
		check_path_length(argv[1]);

		int calling_id = getuid(); //id of A
		struct passwd* passwd;
		passwd = getpwuid(calling_id);
		char *caller_username;
		caller_username = passwd->pw_name;
		printf("UID is of %s\n", caller_username);

		char* dac_path =  "/home/naman/Desktop/rudiFS2/simple_slash/.permissions";
		char content[ALLOWED_CHAR_LENGTH];
		cat(dac_path, content);

		char* owner = get_owner(argv[1]);

		if (calling_id == FAKE_ROOT_ID)
		{
			modify_permissions(argv[1], content, owner);
			exit(0);
		} else {
			//check for permissions using B's id (Privileged Ops) or FAKEROOT's id
			//implicitly uid is set to B's id
			printf("Effective User ID: %d\n", geteuid()); //should be B's

			//privileged operation
			// int result = check_DAC(caller_username, argv[1]);
			printf("Owner of the file/directory is %s\n", get_owner(argv[1]));


			//set it back to caller's id
			seteuid(calling_id); //change back to A's id
			printf("EUID changed to %d\n", geteuid());

			if (strcmp(owner, caller_username) == 0)
			{
				// printf("You have the permissions!\n");
				modify_permissions(argv[1], content, caller_username);

			} else {
				printf("You don't have the necessary permissions!\n");
				return -1;
			}
		}
	} else {
		printf("Usage modify [path]\n");
		return -1;
	}

	return 0;
}