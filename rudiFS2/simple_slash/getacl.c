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

int is_file(char* path)
{
	DIR* directory = opendir(path);
	if (directory != NULL)
	{
		closedir(directory);
		return 0;
	}
	return -1;
}


void read_acl (char *path) {

	FILE * file_pointer;

	if (is_file(path) == 0)
	{
		//Directory.
		char acl_path[ALLOWED_CHAR_LENGTH];
		strcpy(acl_path, path);
		strcat(acl_path, "/directory.acl");

		file_pointer = fopen (acl_path, "r");


		if ( file_pointer == NULL )
		{
			perror("");
			exit(-1);
		}


	} else {
		//File.
		file_pointer = fopen (path, "r");


		if ( file_pointer == NULL )
		{
			perror("");
			exit(-1);
		}

	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	int line_number = 0;
	while ((read = getline(&line, &len, file_pointer)) != -1) {
		if (line_number > 0)
		{
			break;
		}
		printf("%s", line);
		line_number++;
	}

	if (line) {
		free(line);
	}

	fclose(file_pointer);
}


int file_exists(char *path) {
	struct stat st;
	int result = stat(path, &st);
	return result;
}


int main(int argc, char *argv[])
{

	if (argc > 1)
	{
		// strip_line_endings(path);
		resolve_path(argv[1]);
		check_path_length(argv[1]);

		if (file_exists(argv[1]) != 0)
		{
			int error = ENOENT;
			perror("File doesn't exist!\n");
			return -1;
		}


		int calling_id = getuid(); //id of A
		struct passwd* passwd;
		passwd = getpwuid(calling_id);

		char *caller_username;
		caller_username = passwd->pw_name;

		printf("UID is of %s\n", caller_username);
		printf("EUID is %d\n", geteuid());

		if (calling_id == FAKE_ROOT_ID)
		{
			read_acl(argv[1]);
			exit(0);
		} else {
			//check for permissions using B's id (Privileged Ops) or FAKEROOT's id
			//implicitly uid is set to B's id
			// printf("Effective User ID: %d\n", geteuid()); //should be B's

			//privileged operation
			int result = check_DAC(caller_username, argv[1]);

			//set it back to caller's id
			seteuid(calling_id); //change back to A's id
			printf("EUID changed to %d\n", geteuid());

			if (result > 0)
			{
				read_acl(argv[1]);
			} else {
				printf("Inacessible! You don't have the required permissions.\n");
				return -1;
			}
		}

	} else {
		printf("Usage getacl [path] \n");
		return -1;
	}

	return 0;
}