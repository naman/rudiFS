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


void list_files(char* path) {

	DIR *_dir;
	struct dirent *_file;

	if ((_dir = opendir (path)) == NULL) {

		errno = ECANCELED;
		// printf("Can't access path %s\n", path);
		perror("Cannot open path!");
		exit(-1);
	}

	char* cell_1 = "\nType";
	char* cell_2 = "Name";
	char* cell_3 = "Owner";
	char* cell_4 = "User/Group Permissions";
	printf("%5s %10s %15s %20s\n", cell_1, cell_2, cell_3, cell_4);

	_dir = opendir(path);
	char tmp_path[ALLOWED_CHAR_LENGTH];
	while ((_file = readdir(_dir)) != NULL)
	{
		if (strcmp (_file->d_name, "..") != 0 && strcmp (_file->d_name, ".") != 0) {

			//grab permissions
			strcpy(tmp_path, path);
			strcat(tmp_path, "/");
			strcat(tmp_path, _file->d_name);

			strip_line_endings(tmp_path);
			resolve_path(tmp_path);

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

			while ((read = getline(&line, &len, file_pointer)) != -1) {
				char** line_break;
				line_break = splitline(line, ";");

				strip_line_endings(line_break[0]);

				if (strcmp(tmp_path, line_break[0]) == 0)
				{
					if (_file->d_type == 4)
						printf("%5s", "folder");
					else
						printf("%5s", "file");

					printf("%10s", _file->d_name);

					char copy_[ALLOWED_CHAR_LENGTH];
					strcpy(copy_, line_break[1]);

					char* owner = splitline(copy_, ",")[0];
					if (!owner)
						printf("%15s", "fakeroot");
					else
						printf("%15s", owner);

					printf("%25s", line_break[2]);
					break;
				}
			}

			if (line)
				free(line);

			fclose(file_pointer);
		}
	}

	puts("");
	closedir(_dir);
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


int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		// strip_line_endings(path);
		resolve_path(argv[1]);
		check_path_length(argv[1]);

		if (is_file(argv[1]) != 0) {
			printf("This is a file!\n");
			return -1;
		}

		int calling_id = getuid(); //id of A
		struct passwd* passwd;
		passwd = getpwuid(calling_id);

		char *caller_username;
		caller_username = passwd->pw_name;
		// printf("username is %s\n", caller_username);
		printf("UID is of %s\n", caller_username);
		printf("EUID is %d\n", geteuid());


		if (calling_id == FAKE_ROOT_ID)
		{
			list_files(argv[1]);
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
				list_files(argv[1]);
			} else {
				printf("File Inacessible! You don't have the required permissions.\n");
				return -1;
			}
		}

	} else {
		printf("Usage list [path] \n");
		return -1;
	}


	/*
	printf("Real User ID: %d\n", getuid());
	printf("Effective User ID: %d\n", geteuid());
	*/

	return 0;
}