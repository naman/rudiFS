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
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/hmac.h>

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

int calling_id;


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
	file_pointer = fopen ("/home/naman/Desktop/rudiFS3/simple_slash/.groups", "r");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	int return_val = 0;
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


int check_ACL (char* username, char *path) {

	FILE * file_pointer;
	file_pointer = fopen (path, "r");


	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}


	int return_val = 0;

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	char permission[ALLOWED_CHAR_LENGTH];

	int line_number = 0;
	while ((read = getline(&line, &len, file_pointer)) != -1) {
		if (line_number > 0)
		{
			break;
		}
		// printf("%s", line);
		strcpy(permission, line);
		line_number++;
	}

	if (line) {
		free(line);
	}
	fclose(file_pointer);

	char** line_break;
	line_break = splitline(permission, ";");

	strip_line_endings(line_break[0]);

	if (strcmp(username, line_break[0]) == 0)
	{
		return_val = 1;
	}

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

	return return_val;
}


bool check_DAC(char* username, char* path) {

	FILE * file_pointer;
	file_pointer = fopen ("/home/naman/Desktop/rudiFS3/simple_slash/.permissions", "r");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	int return_val = 0;
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
	file_pointer = fopen ("/home/naman/Desktop/rudiFS3/simple_slash/.permissions", "a+");

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



void crypt_init(char* path) {
	printf("enter passphrase: ");
	char password[1000];
	fgets(password, 1000, stdin);
	strip_line_endings(password);

	const EVP_CIPHER *cipher;
	const EVP_MD *dgst = NULL;
	unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
	const unsigned char salt[256];
	SHA256(password, strlen(password), salt);

	OpenSSL_add_all_algorithms();

	cipher = EVP_get_cipherbyname("aes-256-cbc");
	if (!cipher)
		ERR_print_errors_fp(stderr);

	dgst = EVP_get_digestbyname("sha256");
	if (!dgst)
		ERR_print_errors_fp(stderr);

	if (!EVP_BytesToKey(cipher, dgst, salt,
	                    (unsigned char *) password,
	                    strlen(password), 1, key, iv))
		ERR_print_errors_fp(stderr);


	// Creating file for the first time!
	FILE * file_pointer;
	file_pointer = fopen (path, "w");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	int i;
	for (i = 0; i < cipher->key_len; ++i) {
		fprintf(file_pointer, "%02x", key[i]);
	}

	fprintf(file_pointer, "\n");

	for (i = 0; i < cipher->iv_len; ++i) {
		fprintf(file_pointer, "%02x", iv[i]);
	}

	fclose(file_pointer);
}

void readKey(char* path, char* key) {

	FILE * file_pointer;
	file_pointer = fopen (path, "r");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	// puts("");
	int line_number = 0;

	while ((read = getline(&line, &len, file_pointer)) != -1) {
		strip_line_endings(line);
		if (line_number == 0)
		{
			strcat(key, line);
			break;
		}
		line_number++;
	}

	if (line) {
		free(line);
	}

	fclose(file_pointer);
}



void readFile(char* path, char* content) {

	FILE * file_pointer;
	file_pointer = fopen (path, "r");

	if ( file_pointer == NULL )
	{
		perror("");
		exit(-1);
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	while ((read = getline(&line, &len, file_pointer)) != -1) {
		strip_line_endings(line);
		strcat(content, line);
	}

	if (line) {
		free(line);
	}

	fclose(file_pointer);
}


void sign(char* in, FILE* out, char* path) {
	char key[ALLOWED_CHAR_LENGTH];
	readKey(path, key);

	char plaintext[ALLOWED_CHAR_LENGTH];
	readFile(in, plaintext);
	// printf("%s", plaintext);

	// Be careful of the length of string with the choosen hash engine. SHA1 needed 20 characters.
	// Change the length accordingly with your choosen hash engine.
	unsigned char* result;
	unsigned int len = 20;

	result = (unsigned char*)malloc(sizeof(char) * len);

	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);

	HMAC_Init_ex(&ctx, key, strlen(key), EVP_sha1(), NULL);
	HMAC_Update(&ctx, (unsigned char*)&plaintext, strlen(plaintext));
	HMAC_Final(&ctx, result, &len);
	HMAC_CTX_cleanup(&ctx);

	// unsigned int * len = (unsigned int) plaintext_len;
	// result = HMAC(EVP_sha256(), key, strlen(key), plaintext, strlen(plaintext), result, len);

	// fwrite(result, sizeof(char), len, stdout);
	fwrite(result, sizeof(char), len, out);
}



int diff(FILE* in, FILE * out) {
	fseek(in, 0L, SEEK_END);
	int hmac_in_len = ftell(in);
	fseek(in, 0L, SEEK_SET);

	unsigned char* hmac_in = malloc(hmac_in_len);
	fread(hmac_in, sizeof(char), hmac_in_len, in);

	fseek(out, 0L, SEEK_END);
	int hmac_out_len = ftell(out);
	fseek(out, 0L, SEEK_SET);

	unsigned char* hmac_out = malloc(hmac_out_len);
	fread(hmac_out, sizeof(char), hmac_out_len, out);

	int x = memcmp(hmac_in, hmac_out, hmac_out_len);
	return x;
}

void verify_hmac(char* username, char* path)
{
	// char hmac_path[ALLOWED_CHAR_LENGTH];
	char* hmac_path =  "/home/naman/Desktop/rudiFS3/simple_slash/.hmac_temp";

	char* enc_file  = "/home/naman/Desktop/rudiFS3/simple_slash/.enc_temp";
	crypt_init(enc_file);

	char enc_path[ALLOWED_CHAR_LENGTH];
	strcpy(enc_path, path);
	strcat(enc_path, ".enc");

	printf("Checking HMAC...\n");

	FILE * f3;
	f3 = fopen (hmac_path, "wb");

	sign(enc_path, f3, enc_file);

	fclose(f3);

	char hmac_file[ALLOWED_CHAR_LENGTH];
	strcpy(hmac_file, path);
	strcat(hmac_file, ".hmac");

	FILE * fin;
	fin = fopen (hmac_file, "rb");

	FILE * hmac;
	hmac = fopen (hmac_path, "rb");

	if (diff(fin, hmac) == 0) {
		printf("Hurray! File contents are same! :) \n");
	} else {
		printf("Error! File has been tampered with! :( \n");
	}

	fclose(fin);
	fclose(hmac);
}


int permit(char* username, char* path) {
	if (strcmp(username, "fakeroot") == 0)
	{
		return 1;
	}

	int DAC_result = check_DAC(username, path);
	int ACL_result = 1;
	return ACL_result && DAC_result;
}


int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		resolve_path(argv[1]);
		check_path_length(argv[1]);

		calling_id = getuid(); //id of A
		struct passwd* passwd;
		passwd = getpwuid(calling_id);
		char *caller_username;
		caller_username = passwd->pw_name;
		printf("UID is of %s\n", caller_username);
		printf("EUID is %d\n", geteuid());

		if (file_exists(argv[1]) == 0)		{
			if (permit(caller_username, argv[1]) == 0) {
				printf("Inacessible! You don't have the required permissions.\n");
				return -1;
			}
		}

		if (calling_id == FAKE_ROOT_ID)
		{
			// printf("Content: ");
			// unsigned char content[ALLOWED_CHAR_LENGTH];
			// fgets(content, ALLOWED_CHAR_LENGTH, stdin);
			// strip_line_endings(content);

			// printf("%s\n", content);
			verify_hmac(caller_username, argv[1]);
			exit(0);
		} else {
			//check for permissions using B's id (Privileged Ops) or FAKEROOT's id
			//implicitly uid is set to B's id
			// printf("Effective User ID: %d\n", geteuid()); //should be B's

			//privileged operation
			// int result = check_DAC(caller_username, argv[1]);

			//set it back to caller's id
			seteuid(calling_id); //change back to A's id
			printf("EUID changed to %d\n", geteuid());

			// if (result > 0)
			// {

			// printf("Content: ");
			// unsigned char content[ALLOWED_CHAR_LENGTH];
			// fgets(content, ALLOWED_CHAR_LENGTH, stdin);
			// strip_line_endings(content);

			// printf("%s\n", content);

			verify_hmac(caller_username, argv[1]);
		}
	} else {
		printf("Usage fput [path]\n");
		return -1;
	}

	return 0;
}
