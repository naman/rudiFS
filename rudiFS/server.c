#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include<pthread.h> 

#ifndef ALLOWED_CHAR_LENGTH
#define ALLOWED_CHAR_LENGTH 5000
#endif

#ifndef DEFAULT_FOLDER_PERMISSIONS
#define DEFAULT_FOLDER_PERMISSIONS 0755
#endif

typedef int bool;
#define true 1
#define false 0




volatile bool should_terminate = false;



void check_username_length(char* username){
	if (strlen(username) >= 32)
	{
		errno = E2BIG; 
		perror("Username has to be less than 32 bytes!");
	}
}

int check_path_length(char* path){
	if (strlen(path) >= PATH_MAX) //4096 bytes
	{
		errno = E2BIG;
		perror("Path has to be less than 4096 bytes!");
		return -1;
	}
	return 1;
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




char* list_files(char* path){

	DIR *_dir;
	struct dirent *_file;

	char result[ALLOWED_CHAR_LENGTH];

	if ((_dir = opendir (path)) == NULL) {
		
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open directory!");
	}

	char* cell_1 = "Type";
	char* cell_2 = "Name";
	char* cell_3 = "Owner";
	char* cell_4 = "User/Group Permissions";


	sprintf(result, "%5s %10s %15s %20s\n", cell_1, cell_2, cell_3, cell_4);

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
				return "Cannot open file!";
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
					char tmp[ALLOWED_CHAR_LENGTH];
					if(_file->d_type == 4){

						sprintf(tmp,"%5s","folder");
						strcat(result,tmp);
					}
					else{
						sprintf(tmp,"%5s","file");
						strcat(result,tmp);
					}
					

					sprintf(tmp, "%10s", _file->d_name);
					strcat(result,tmp);

					char copy_[ALLOWED_CHAR_LENGTH];
					strcpy(copy_, line_break[1]);

					char* owner = splitline(copy_,",")[0];
					if (!owner){
						sprintf(tmp, "%15s", "root");
						strcat(result,tmp);
					}
					else{
						sprintf(tmp, "%15s", owner);
						strcat(result,tmp);
					}
					
					sprintf(tmp, "%25s", line_break[2]);	
					strcat(result,tmp);
					break;				
				}
			}

			if (line)
				free(line);

			fclose(file_pointer);
		}
	}
	closedir(_dir);
	return result;
}


int create_file(char* path, char* content){

	FILE * file_pointer;
	file_pointer = fopen (path, "w");

	if( file_pointer == NULL )
	{
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open file!");
		return -1;
	}

	char *usage = "Usage %s, [options] ... ";

	fprintf(file_pointer, content, usage);
	fclose(file_pointer);
	
	return 1;
}


char* cat(char* path){

	FILE * file_pointer;
	file_pointer = fopen (path, "r");

	if( file_pointer == NULL )
	{
		errno = ECANCELED;
		printf("Can't access path %s\n", path);
		perror("Cannot open file!");
		return "Cannot open file!";
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	char result[ALLOWED_CHAR_LENGTH];

	while ((read = getline(&line, &len, file_pointer)) != -1) {
		// printf("%s", line);
		strcat(result, line);
	}

	if (line){
		free(line);
	}

	fclose(file_pointer);

	return result;
}


void create_directory(char* path, mode_t mode, char* username, char* group_permissions){
	/*Referenced from http://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix */

	/* Used the pseudocode with some modifications. */

	char* path_copy = strdup(path);
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

	create_directory(next_dir, mode, username, group_permissions);
	
	int created = mkdir(next_dir, mode);

	if (created  == 0)
	{
		printf("%s\n", next_dir);
		resolve_path(next_dir);
		write_permissions(next_dir, username, group_permissions);
	}
}

char* getowner(){

}


void handle_command(char* command, char* username, int socket){
	
	if (strcmp(command,"ls") == 0){
		send(socket, "1", ALLOWED_CHAR_LENGTH, 0);

		// printf("Path: ");
		char path[ALLOWED_CHAR_LENGTH];
		// fgets(path, ALLOWED_CHAR_LENGTH, stdin);
		recv(socket, path, ALLOWED_CHAR_LENGTH, 0);
		strip_line_endings(path);

		resolve_path(path);
		check_path_length(path);

		if(check_permission(username, path) > 0){
			char* result;
			result = list_files(path);
			send(socket, result, ALLOWED_CHAR_LENGTH, 0);
		}else{
			send(socket, "Inacessible! You don't have the required permissions.\n", ALLOWED_CHAR_LENGTH, 0);
		}

	}else if (strcmp(command,"fput") == 0)
	{
		send(socket, "2", ALLOWED_CHAR_LENGTH, 0);

		// printf("Path with filename: ");
		char path[ALLOWED_CHAR_LENGTH];
		recv(socket, path, ALLOWED_CHAR_LENGTH, 0);
		strip_line_endings(path);

		if (check_path_length(path) == 1)
		{
			send(socket, "1", ALLOWED_CHAR_LENGTH, 0);	
		}else{
			send(socket, "-1", ALLOWED_CHAR_LENGTH, 0);			
		}		

		char content[ALLOWED_CHAR_LENGTH];
		recv(socket, content, ALLOWED_CHAR_LENGTH, 0);
		strip_line_endings(content);
		
		
		char tmp_path[ALLOWED_CHAR_LENGTH];
		strcpy(tmp_path, path);
		resolve_path(tmp_path);

		
		if (create_file(path, content) == 1 && check_permission(username, tmp_path) > 0)
		{
			send(socket, "1", ALLOWED_CHAR_LENGTH, 0);	

			char group_permissions[ALLOWED_CHAR_LENGTH];
			recv(socket, group_permissions, ALLOWED_CHAR_LENGTH, 0);
			strip_line_endings(group_permissions);
		
			write_permissions(path, username, group_permissions);		

		}else{
			send(socket, "-1", ALLOWED_CHAR_LENGTH, 0);			
		}

	}else if (strcmp(command,"fget") == 0)
	{
		
		send(socket, "3", ALLOWED_CHAR_LENGTH, 0);
		// printf("Path with filename: ");

		char path[ALLOWED_CHAR_LENGTH];
		recv(socket, path, ALLOWED_CHAR_LENGTH, 0);
		strip_line_endings(path);
		resolve_path(path);

		if (check_path_length(path) == 1)
		{
			send(socket, "1", ALLOWED_CHAR_LENGTH, 0);	
		}else{
			send(socket, "-1", ALLOWED_CHAR_LENGTH, 0);			
		}	

		if(check_permission(username, path) > 0){
			char* result;
			result = cat(path);
			send(socket, result, ALLOWED_CHAR_LENGTH, 0);
		}else{
			send(socket, "Inacessible! You don't have the required permissions.\n", ALLOWED_CHAR_LENGTH, 0);
		}

	}else if (strcmp(command,"create_dir") == 0)
	{
		send(socket, "4", ALLOWED_CHAR_LENGTH, 0);

		// printf("Path with filename: ");
		char path[ALLOWED_CHAR_LENGTH];
		// recv(socket, "path", ALLOWED_CHAR_LENGTH, 0);
		strip_line_endings(path);

		char tmp_path[ALLOWED_CHAR_LENGTH];
		strcpy(tmp_path, path);
		strcat(tmp_path, "/test");

		if (check_path_length(tmp_path) == 1)
		{
			send(socket, "1", ALLOWED_CHAR_LENGTH, 0);	
		}else{
			send(socket, "-1", ALLOWED_CHAR_LENGTH, 0);			
		}	


		char group_permissions[ALLOWED_CHAR_LENGTH];
		// recv(socket, group_permissions, ALLOWED_CHAR_LENGTH, 0);
		strip_line_endings(group_permissions);

		// create_directory(tmp_path, DEFAULT_FOLDER_PERMISSIONS, username, group_permissions);
	}else{
		send(socket, "0", ALLOWED_CHAR_LENGTH, 0);
		perror("Command not supported!\n");
	}
	
}


void create_env(int socket, char* username){

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
			recv(socket, command, ALLOWED_CHAR_LENGTH, 0);

			handle_command(command, username, socket);
			close(socket);

			fflush(stdout);
			exit(1);
		}else if(pid > 0)
		{
			// parent
			waitpid(pid,NULL,0);
		}
	}
}



void sigproc()
{ 	
	signal(SIGINT, sigproc);

    // To terminate child thread:
    should_terminate = true;

	printf(" Trap. Try Ctrl+\\ to quit.\n");
}


void quitproc()
{ 	
	printf(" Quitting. zzzzz\n");
	// pthread_kill(listener_thread, SIGQUIT);
	exit(0);
}


int sanity_check(char* username){

	FILE * file_pointer;
	file_pointer = fopen ("simple_slash/.allowed_users", "r");
	
	if( file_pointer == NULL )
	{
		errno = EBADF;
		perror("Error while opening the file.\n");
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	int return_val = -1;
	while ((read = getline(&line, &len, file_pointer)) != -1) {
		strip_line_endings(line);
		if (strcmp(line, username) == 0)
		{
			return_val = 1;
			break;	
		}
	}

	if (line){
		free(line);
	}
	fclose(file_pointer);

	return return_val;
}


void * connection_handler(void *connection_socket){

	int _socket = *(int*)connection_socket;
	char username[ALLOWED_CHAR_LENGTH];

	recv(_socket, username, ALLOWED_CHAR_LENGTH, 0);

	if (sanity_check(username) == 1){
		printf("%s connected.\n",username);
		send(_socket, "connected", ALLOWED_CHAR_LENGTH, 0);

		create_env(_socket, username);

	}else{
		//terminate connection here
		printf("Invalid username!\n");
		send(_socket, "disconnected", ALLOWED_CHAR_LENGTH, 0);
	}

	free(connection_socket);
	return 0;
}


int main(int argc, char const *argv[])
{

	signal(SIGINT, sigproc);
	signal(SIGQUIT, quitproc);

	int server_socket_descriptor;
	struct sockaddr_in server_address, client_address;
	struct sockaddr_storage server_storage;
	socklen_t address_size;

	server_socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket_descriptor == -1)
	{
		errno = EBADR;
		perror("Error creating socket!\n");
	}else{
		// printf("Socket initialized...\n");
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_port = htons(8000);

	memset(server_address.sin_zero, '\0', sizeof(server_address.sin_zero));

	if (bind(server_socket_descriptor, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		errno = EBADRQC;
		perror("Bind Failed!");
		exit(-1);
	}else{
		// printf("server socket bound to address...\n");
	}

	if (listen(server_socket_descriptor, 50) == 0)
	{
		// printf("Listening to connections...\n");
	}else{
		errno = ENETRESET;
		perror("Error!");
	}

	int connection_socket, *another_socket;
	address_size = sizeof(server_storage);

	while(connection_socket  = accept(server_socket_descriptor, (struct sockaddr *) &server_storage, &address_size)){

		
		printf("A new client has connected.\n");
		another_socket = (int *) malloc(sizeof(int));
		*another_socket = connection_socket; //dereference and assign client socket
		

		pthread_t listener_thread;

		if (pthread_create( &listener_thread, NULL, connection_handler, (void *) another_socket) < 0)
		{
			errno = EACCES;
			perror("Unable to create a thread!");
			exit(-1);
		}else{
			printf("Handling connection!\n");
		}

		if (should_terminate == true)
		{
			pthread_cancel(listener_thread);
		}
		// pthread_join(listener_thread, NULL);
	}

	if (connection_socket < 0)
	{
		errno = ECONNREFUSED;
		perror("Cannot accept connection from cient!");
	}

	return 0;
}