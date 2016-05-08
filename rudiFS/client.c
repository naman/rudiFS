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

#ifndef ALLOWED_CHAR_LENGTH
#define ALLOWED_CHAR_LENGTH 5000
#endif

#ifndef DEFAULT_FOLDER_PERMISSIONS
#define DEFAULT_FOLDER_PERMISSIONS 0755
#endif

typedef int bool;
#define true 1
#define false 0


void check_username_length(char* username){
	if (strlen(username) >= 32)
	{
		errno = E2BIG; 
		perror("Username has to be less than 32 bytes!");
	}
}

void check_path_length(char* path){
	if (strlen(path) >= PATH_MAX) //4096 bytes
	{
		errno = E2BIG;
		perror("Path has to be less than 4096 bytes!");
	}
}

void check_file_length(char* filename){
	if (strlen(filename) >= 256)
	{
		errno = E2BIG;
		perror("Path has to be less than 256 bytes!");
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
		pos+=1;

		if (pos >= buffer) {
			buffer += buffer;
			args = realloc(args, buffer * sizeof(char*));
		}

		token = strtok(NULL, delimiter);
	}
	args[pos] = NULL;
	return args;
}


void sigproc()
{ 	
	signal(SIGQUIT, sigproc);
	printf(" Trap. Quitting.\n");
	pthread_exit(-1);
	raise(SIGINT);
	// kill(getpid(), SIGINT);
	// exit(-1);

}


void strip_line_endings(char* input){
	/* strip of /r and /n chars to take care of Windows, Unix, OSx line endings. */
	input[strcspn(input, "\r\n")] = 0;
}


void resolve_path(char* path)
{
	char resolved_path[ALLOWED_CHAR_LENGTH];
	realpath(path, resolved_path);
	strcpy(path, resolved_path);
}

int check_groups(char* username, char* group_permission){

	FILE * file_pointer;
	file_pointer = fopen ("./simple_slash/.groups", "r");
	
	if( file_pointer == NULL )
	{
		errno = ECANCELED;
		perror("Cannot open file!");
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

			int i=0;
			while(users_break[i]!=NULL){
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

	if (line){
		free(line);
	}
	fclose(file_pointer);

	return return_val;
}


bool check_permission(char* username, char* path){

	FILE * file_pointer;
	file_pointer = fopen ("./simple_slash/.permissions", "r");
	
	if( file_pointer == NULL )
	{
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open file!");
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

			int i=0;
			while(user_permissions[i]!=NULL){
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

			i=0;
			while(group_permissions[i]!=NULL){
				strip_line_endings(group_permissions[i]);

				//now check all groups against .groups file.
				if (return_val!=1)
				{
					return_val = check_groups(username, group_permissions[i]);
				}

				i++;
			}
		}
	}

	if (line){
		free(line);
	}
	fclose(file_pointer);

	return return_val;
}


void write_permissions(char* path, char* username, char* group_permissions){

	FILE * file_pointer;
	file_pointer = fopen ("./simple_slash/.permissions", "a+");
	
	if( file_pointer == NULL )
	{
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open file!");
	}

	char file_perms[ALLOWED_CHAR_LENGTH];
	
	strcpy(file_perms, path);
	
	//users
	strcat(file_perms, ";");
	strcat(file_perms, username);

	//groups
	strcat(file_perms, ";root,");
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

	if (line){
		free(line);
	}

	if (append == true)
	{
		char *usage = "Usage %s, [options] ... ";
		fprintf(file_pointer, file_perms, usage);
		fclose(file_pointer);
	}
}


void init_permissions(char* path, char* username){

	FILE * file_pointer;
	file_pointer = fopen ("./simple_slash/.permissions", "a+");
	
	if( file_pointer == NULL )
	{
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open file!");
	}

	char file_perms[ALLOWED_CHAR_LENGTH];
	
	strcpy(file_perms, path);
	
	//users
	strcat(file_perms, ";");
	strcat(file_perms, username);

	//groups
	strcat(file_perms, ";root,");
	strcat(file_perms, username);
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

	if (line){
		free(line);
	}

	if (append == true)
	{
		char *usage = "Usage %s, [options] ... ";
		fprintf(file_pointer, file_perms, usage);
		fclose(file_pointer);
	}
}


void list_files(char* path){

	DIR *_dir;
	struct dirent *_file;

	if ((_dir = opendir (path)) == NULL) {
		
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open directory!");
	}

	char* cell_1 = "Type";
	char* cell_2 = "Name";
	char* cell_3 = "Owner";
	char* cell_4 = "User/Group Permissions";
	printf("%5s %10s %15s %20s\n", cell_1, cell_2, cell_3, cell_4);

	_dir = opendir(path);
	char tmp_path[ALLOWED_CHAR_LENGTH];
	while((_file = readdir(_dir)) != NULL)
	{
		if (strcmp (_file->d_name, "..") != 0 && strcmp (_file->d_name, ".") != 0) {

			//grab permissions
			strcpy(tmp_path, path);
			strcat(tmp_path, "/");
			strcat(tmp_path, _file->d_name);

			strip_line_endings(tmp_path);
			resolve_path(tmp_path);

			FILE * file_pointer;
			file_pointer = fopen ("./simple_slash/.permissions", "r");

			if( file_pointer == NULL )
			{		
				errno = ECANCELED;
				printf("Can't access path %s\n", path);
				perror("Cannot open file!");
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
					if(_file->d_type == 4)
						printf("%5s","folder");
					else
						printf("%5s","file");
					
					printf("%10s", _file->d_name);

					char copy_[ALLOWED_CHAR_LENGTH];
					strcpy(copy_, line_break[1]);

					char* owner = splitline(copy_,",")[0];
					if (!owner)
						printf("%15s", "root");
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


void create_file(char* path, char* content){

	FILE * file_pointer;
	file_pointer = fopen (path, "w");

	if( file_pointer == NULL )
	{
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open file!");
	}

	char *usage = "Usage %s, [options] ... ";

	fprintf(file_pointer, content, usage);
	fclose(file_pointer);
}


void cat(char* path){

	FILE * file_pointer;
	file_pointer = fopen (path, "r");

	if( file_pointer == NULL )
	{
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open file!");
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;


	while ((read = getline(&line, &len, file_pointer)) != -1) {
		printf("%s", line);
	}

	if (line){
		free(line);
	}

	fclose(file_pointer);
}


void create_directory(char* path, mode_t mode, char* username){
	/*Referenced from http://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix */
	/* Used the pseudocode with some modifications. */

	char* path_copy;
	path_copy = strdup(path);

	char* next_dir;
	
	next_dir = dirname(path_copy);

	if (access(next_dir, F_OK) == 0)
	{
		free(path_copy);
		return;
	}

	if (strcmp(next_dir, ".") == 0 || strcmp(next_dir, "/") == 0)
	{
		free(path_copy);
		return;
	}

	create_directory(next_dir, mode, username);
	int created = mkdir(next_dir, mode);


	if (created  == 0)
	{

		printf("Please don't use space in between\n");

		printf("User/Group permissions(csv) for %s: ", next_dir);
		char group_permissions[ALLOWED_CHAR_LENGTH];
		fgets(group_permissions, ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(group_permissions);

		resolve_path(next_dir);
		write_permissions(next_dir, username, group_permissions);
	}

}


void handle_command(char* command, char* username){
	if (strcmp(command,"ls") == 0){

		printf("Path: ");
		char path[ALLOWED_CHAR_LENGTH];
		fgets(path, ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(path);

		resolve_path(path);
		check_path_length(path);

		if(check_permission(username, path) > 0){
			list_files(path);
		}else{
			printf("Inacessible! You don't have the required permissions.\n");
		}


	}else if (strcmp(command,"fput") == 0)
	{
		printf("Path with filename: ");
		char path[ALLOWED_CHAR_LENGTH];
		fgets(path, ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(path);
		check_path_length(path);

		printf("Content: ");
		char content[5*ALLOWED_CHAR_LENGTH];
		fgets(content, ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(content);
		
		create_file(path, content);

	}else if (strcmp(command,"fget") == 0)
	{
		printf("Path with filename: ");

		char path[ALLOWED_CHAR_LENGTH];
		fgets(path, ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(path);
		resolve_path(path);

		check_path_length(path);

		if(check_permission(username, path) > 0){
			cat(path);
		}else{
			printf("Inacessible! You don't have the required permissions.\n");
		}

	}else if (strcmp(command,"create_dir") == 0)
	{
		printf("Path with directory name: ");

		char path[ALLOWED_CHAR_LENGTH];
		fgets(path, ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(path);
		check_path_length(path);

		char tmp_path[ALLOWED_CHAR_LENGTH];
		strcpy(tmp_path, path);
		strcat(tmp_path, "/test");
		check_path_length(path);

		create_directory(tmp_path, DEFAULT_FOLDER_PERMISSIONS, username);

	}else{
		printf("Command not supported!\n");
	}
	
}


void create_env(int* socket, char* username){
	printf("\n\n================== Authenticating.... ==================\n");

	char path[ALLOWED_CHAR_LENGTH];
	strcpy(path, "simple_slash/simple_home/");
	strcat(path, username);

	char tmp_path[ALLOWED_CHAR_LENGTH];
	strcpy(tmp_path, path);
	strcat(tmp_path, "/test");
	create_directory(tmp_path, DEFAULT_FOLDER_PERMISSIONS, username);

	resolve_path(path);
	check_path_length(path);

	init_permissions(path, username);

	sleep(1);
	system ("clear");

	printf("================== Welcome to rudiFS! ==================\n\n");

	char *wd = NULL;
	wd = getcwd(wd,ALLOWED_CHAR_LENGTH);
	check_path_length(wd);

	printf("Hello! You are at %s\n\n$ ",wd);

	while(1){

		pid_t pid;
		pid = fork(); //clones, parent gets child-id, child gets 0

		if(pid < 0)
		{
			errno = EACCES;
			perror("Fork Failed");

		}else if (pid == 0)
		{
			// child - execute command here
			char command[ALLOWED_CHAR_LENGTH];
			fgets(command,ALLOWED_CHAR_LENGTH, stdin);
			strip_line_endings(command);

			handle_command(command, username);

			exit(1);
		}else if(pid > 0)
		{
			// parent
			wait(NULL);
		}
	}
}

void init(char* username){
	printf("\n\n================== Authenticating.... ==================\n");

	char path[ALLOWED_CHAR_LENGTH];
	strcpy(path, "simple_slash/simple_home/");
	strcat(path, username);

	char tmp_path[ALLOWED_CHAR_LENGTH];
	strcpy(tmp_path, path);
	strcat(tmp_path, "/test");
	create_directory(tmp_path, DEFAULT_FOLDER_PERMISSIONS, username);

	resolve_path(path);
	check_path_length(path);

	init_permissions(path, username);

	sleep(1);

	system ("clear");

	printf("================== Welcome to rudiFS! ==================\n\n");

	char *wd = NULL;
	wd = getcwd(wd,ALLOWED_CHAR_LENGTH);
	check_path_length(wd);


	printf("Hello! You are at %s\n\n",wd);
}

void interface(char* username, int socket){
			// receive and send commands and paths

	printf("$ ");

	char cmd[ALLOWED_CHAR_LENGTH];
	fgets(cmd,ALLOWED_CHAR_LENGTH, stdin);
	strip_line_endings(cmd);	
	send(socket, cmd, ALLOWED_CHAR_LENGTH, 0);

	char success[ALLOWED_CHAR_LENGTH];
	recv(socket, success, ALLOWED_CHAR_LENGTH, 0);

	if (strcmp(success, "1") == 0){

		printf("Path: ");
		char path[ALLOWED_CHAR_LENGTH];
		fgets(path,ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(path);	
		send(socket, path, ALLOWED_CHAR_LENGTH, 0);

		char std_out[ALLOWED_CHAR_LENGTH];
		recv(socket, std_out, ALLOWED_CHAR_LENGTH, 0);
		printf("%s", std_out);

	}else if (strcmp(success, "2") == 0){

		printf("Path: ");
		char path[ALLOWED_CHAR_LENGTH];
		fgets(path,ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(path);	
		send(socket, path, ALLOWED_CHAR_LENGTH, 0);

		char result_code[ALLOWED_CHAR_LENGTH];
		recv(socket, result_code, ALLOWED_CHAR_LENGTH, 0);

		if (strcmp(result_code, "1") == 0)
		{
			printf("Content: ");
			char content[ALLOWED_CHAR_LENGTH];
			fgets(content,ALLOWED_CHAR_LENGTH, stdin);
			strip_line_endings(content);	
			send(socket, content, ALLOWED_CHAR_LENGTH, 0);

			char result_code[ALLOWED_CHAR_LENGTH];
			recv(socket, result_code, ALLOWED_CHAR_LENGTH, 0);
			if (strcmp(result_code, "1") == 0){
				printf("Please don't use space in between\n");
				printf("User/Group permissions(csv): ");

				char prmsns[ALLOWED_CHAR_LENGTH];
				fgets(prmsns,ALLOWED_CHAR_LENGTH, stdin);
				strip_line_endings(prmsns);	
				send(socket, prmsns, ALLOWED_CHAR_LENGTH, 0);

			}else if(strcmp(result_code, "-1") == 0){
				printf("Error!\n");
			}
		}else{
			printf("Path too long!\n");
		}

	}else if(strcmp(success, "3") == 0){

		printf("Path: ");
		char path[ALLOWED_CHAR_LENGTH];
		fgets(path,ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(path);	
		send(socket, path, ALLOWED_CHAR_LENGTH, 0);

		char result_code[ALLOWED_CHAR_LENGTH];
		recv(socket, result_code, ALLOWED_CHAR_LENGTH, 0);
		if (strcmp(result_code, "1") == 0){

			char std_out[ALLOWED_CHAR_LENGTH];
			recv(socket, std_out, ALLOWED_CHAR_LENGTH, 0);

			printf("%s", std_out);
		}

	}else if (strcmp(success, "4") == 0){

		printf("Path: ");
		char path[ALLOWED_CHAR_LENGTH];
		// fgets(path,ALLOWED_CHAR_LENGTH, stdin);
		strip_line_endings(path);	
		// send(socket, path, ALLOWED_CHAR_LENGTH, 0);

		char result_code[ALLOWED_CHAR_LENGTH];
		recv(socket, result_code, ALLOWED_CHAR_LENGTH, 0);

		if (strcmp(result_code, "1") == 0)
		{

			// printf("Please don't use space in between\n");
			// printf("User/Group permissions(csv): ");
			char prmsns[ALLOWED_CHAR_LENGTH];
			// fgets(prmsns, ALLOWED_CHAR_LENGTH, stdin);
			strip_line_endings(prmsns);	
			send(socket, prmsns, ALLOWED_CHAR_LENGTH, 0);

	/*		char tmp_path[ALLOWED_CHAR_LENGTH];
			strcpy(tmp_path, path);
			strcat(tmp_path, "/test");*/

			// create_directory(tmp_path, DEFAULT_FOLDER_PERMISSIONS, username, group_permissions);
		}else{
			printf("Path too long!\n");
		}

		handle_command("create_dir", username);


	}else{
		printf("Command not supported!\n");
	}

}

int main(int argc, char const *argv[])
{

	// signal(SIGQUIT, sigproc);
	int client_socket_descriptor;
	struct sockaddr_in server_address;
	socklen_t address_size;

	client_socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);
	if (client_socket_descriptor == -1)
	{
		errno = EBADR;
		perror("Error creating socket!");
	}else{
		printf("socket initialized...\n");
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_port = htons(8000);

	memset(server_address.sin_zero, '\0', sizeof(server_address.sin_zero));
	address_size = sizeof(server_address);

	if(connect(client_socket_descriptor, (struct sockaddr *)&server_address, address_size) < 0){
		errno = ECONNREFUSED;
		perror("Error connecting to the server!\n");
		exit(-1);
	}else{
		printf("you are connected to the server...\n");
	}

	printf("Hello! Please enter your username to login.\n\n> ");

	char username[ALLOWED_CHAR_LENGTH];
	fgets(username,ALLOWED_CHAR_LENGTH, stdin);
	strip_line_endings(username);

	check_username_length(username);


	if( send(client_socket_descriptor, username, ALLOWED_CHAR_LENGTH, 0) < 0)
	{
		perror("Send failed");
		exit(-1);
	}

	char success[ALLOWED_CHAR_LENGTH];

	if(recv(client_socket_descriptor, success, ALLOWED_CHAR_LENGTH, 0) < 0){
		perror("Recv failed");
		exit(-1);
	}

	strip_line_endings(success);

	printf("You are %s\n",success);

	if (strcmp(success,"disconnected") == 0)
	{
		errno = ECONNREFUSED;
		perror("Invalid username! Please try again!\n");
	}else{

		init(username);

		while(1){
			interface(username, client_socket_descriptor);
		}
	}

	close(client_socket_descriptor);
	return 0;
}