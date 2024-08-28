#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../include/message.h"

#define PORT 8080
#define MAX_BUF_SIZE 1024
#define FILE_HOME "./file/"

void connect_client();
void sync_file_io(_Message* message, char* file_path, _Response* response);
void exec_command(int new_socket);
void command_get(FILE** fp, int length, _Response* response);
void command_put(FILE** fp, _Message* message, char* file_path, _Response* response);
FILE* open_and_seek_file(_Message* message, char* file_path);
void command_delete(_Message* message, char* file_path, _Response* response);
void command_getall(_Response* response);
char* get_file_path(char* path, char* file_name);

int main(int argc, char const* argv[]){
	connect_client();
	return 0;
}

void connect_client(){
    int listening_socket, new_socket;
	struct sockaddr_in addr;
	int opt = 1;
	socklen_t addrlen = sizeof(addr);

	if((listening_socket = socket(AF_INET, SOCK_STREAM,0))<0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if(setsockopt(listening_socket, SOL_SOCKET, 
				SO_REUSEADDR,
				&opt, sizeof(opt))){
			perror("setsockopt");
			exit(EXIT_FAILURE);	
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(listening_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		perror("bind failed");
		exit(EXIT_FAILURE);	
	}

	if(listen(listening_socket, 1) <0){
		perror("listen");
		exit(EXIT_FAILURE);	
	}

	if((new_socket = accept(listening_socket, (struct sockaddr*)&addr, &addrlen)) < 0 ){
		perror("accept");
		exit(EXIT_FAILURE);
	}

    printf("\nConnected to client: %s\n", inet_ntoa(addr.sin_addr));
    exec_command(new_socket);

    printf("\nCLOSE CONNECTION: %s\n\n", inet_ntoa(addr.sin_addr));
    close(new_socket);
    // TODO: 멀티스레드 환경 변경시 수정
    close(listening_socket);
}

void exec_command(int new_socket){
    ssize_t valread;
    char* file_path = NULL;
    _Message* message = (_Message*)malloc(sizeof(_Message));
    if(message == NULL){
        printf("\nMemory allocation failed\n");
        return;
    }

    _Response* response = (_Response*)malloc(sizeof(_Response));
    if(response == NULL){
        printf("\nMemory allocation failed\n");
        return;
    }

	while(1){
	    memset(message, 0, sizeof(*message));
	    memset(response, 0, sizeof(*response));

        valread = recv(new_socket, message, sizeof(*message), 0);
        if(valread<=0){
            break;
        }

        switch(message->header.type){
            case GETALL:
                command_getall(response);
                break;
            case DELETE:
                file_path = get_file_path(FILE_HOME, message->header.file_name);
                command_delete(message, file_path, response);
                free(file_path);
                break;
            case GET: case PUT:
                file_path = get_file_path(FILE_HOME, message->header.file_name);
                sync_file_io(message, file_path, response);
                free(file_path);
                break;
            default:
                perror("\nInvalid command\n");
                make_response(response, ERROR, "Invalid command");
                break;
        }
        send(new_socket, response, sizeof(*response), 0);
	}
    free(message);
    free(response);
}

void sync_file_io(_Message* message, char* file_path, _Response* response){
	FILE* fp = open_and_seek_file(message, file_path);

	if(message->header.type == PUT){
        command_put(&fp, message, file_path, response);
    }else if(message->header.type == GET){
        command_get(&fp, message->body.length, response);
    }
	fclose(fp);
}

void command_get(FILE** fp, int length, _Response* response){
    if(*fp == NULL){
        perror("\nFile not found");
        make_response(response, ERROR, "File not found");
        return;
    }

    char* ret = NULL;
    int length_left = length;

    while(length_left > 0 && !feof(*fp)){
        char buffer[MAX_BUF_SIZE] = {0};
        int length_read = length > MAX_BUF_SIZE ? MAX_BUF_SIZE : length_left + 1;
        ret = fgets(buffer, length_read, *fp);

        if(ret == NULL){
            if(feof(*fp)) {
                break;
            }
            perror("fgets");
            return;
        }

        if(!strcmp(ret,"\n")) continue;
        strcat(response->body.data, ret);
        length_left -= strlen(ret);
        if(buffer[strlen(ret)-1]=='\n') length_left++;
    }
    strcat(response->body.data, "\n");
}

void command_put(FILE** fp, _Message* message, char* file_path, _Response* response){
    if(*fp==NULL){
        printf("\nCreating file...\n\n");
        *fp = fopen(file_path, "w+");
    }
    fputs(message->body.content, *fp);
    make_response(response, SUCCESS, "write file successfully\n");
}

FILE* open_and_seek_file(_Message* message, char* file_path){
    FILE* fp = fopen(file_path, "r+");

    if(fp!=NULL){
        printf("\nThe file is opened\n\n");
        fseek(fp, message->body.offset, SEEK_SET);
    }

    return fp;
}

void command_delete(_Message* message, char* file_path, _Response* response){
    if(remove(file_path) == 0){
        make_response(response, SUCCESS, "delete file successfully");
    }else{
        perror("\nError deleting file");
        make_response(response, ERROR, "File not found");
    }
}

void command_getall(_Response* response){
    DIR* dp = NULL;
    struct dirent* dir;
    struct stat buf;

    if((dp = opendir(FILE_HOME)) == NULL){
        perror("directory cannot be opened");
        make_response(response, ERROR, "directory cannot be opened");
        return;
    }
    char* path = NULL;

    while((dir = readdir(dp)) != NULL){
        char* path = get_file_path(FILE_HOME, dir->d_name);

        if (lstat(path, &buf) != 0 || S_ISDIR(buf.st_mode)) {
            continue;
        }

        strcat(response->body.data, dir->d_name);
        strcat(response->body.data, "\n");
        free(path);
    }
    closedir(dp);
}

char* get_file_path(char* path, char* file_name){
    size_t path_len = strlen(path) + strlen(file_name) + 1;
    char* ret = (char*)malloc(path_len * sizeof(char));
    sprintf(ret, "%s%s", path, file_name);

    return ret;
}