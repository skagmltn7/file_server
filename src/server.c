#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

#include "../include/message.h"
#include "../include/logging.h"

#define PORT 8080
#define MAX_BUF_SIZE 1024
#define FILE_HOME "./file/"
#define LOG_FILE_HOME "./log/"

FILE* log_file;

void connect_client();
void sync_file_io(_Message* message, char* file_path, _Response* response);
void exec_command(int new_socket);
void command_get(FILE* fp, int length, _Response* response);
void command_put(FILE** fp, _Message* message, char* file_path, _Response* response);
FILE* open_and_seek_file(_Message* message, char* file_path);
void command_delete(_Message* message, char* file_path, _Response* response);
void command_getall(_Response* response);
char* get_file_path(const char* file_home, ...);
size_t compute_length(const char* first, va_list args);
FILE* init_server();

int main(int argc, char const* argv[]){
    log_file = init_server();
    log_info(log_file, "Starting server...\n");

	connect_client();

	log_info(log_file, "Shutdown server...\n");
	fclose(log_file);
	return 0;
}

void connect_client(){
    int listening_socket, client_socket;
	struct sockaddr_in addr;
	int opt = 1;
	socklen_t addrlen = sizeof(addr);

	if((listening_socket = socket(AF_INET, SOCK_STREAM,0))<0){
        log_error(log_file);
		exit(EXIT_FAILURE);
	}

	if(setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        log_error(log_file);
        exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(listening_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0){
	    log_error(log_file);
		exit(EXIT_FAILURE);	
	}

	if(listen(listening_socket, SOMAXCONN) < 0){
		log_error(log_file);
		exit(EXIT_FAILURE);	
	}

	while((client_socket = accept(listening_socket, (struct sockaddr*)&addr, &addrlen)) >= 0 ){
        printf("\nConnected to %s\n", inet_ntoa(addr.sin_addr));
        exec_command(client_socket);

        printf("\nConnection to %s closed.\n", inet_ntoa(addr.sin_addr));
        close(client_socket);
	}

    close(listening_socket);

	if(client_socket < 0){
        log_error(log_file);
        exit(EXIT_FAILURE);
    }
}

void exec_command(int client_socket){
    ssize_t valread;
    char* file_path = NULL;

	while(1){
        _Message* message = (_Message*)calloc(1, sizeof(_Message));
        if(!message){
            perror("message calloc error");
            break;
        }

        valread = recv(client_socket, &message->header, sizeof(message->header), 0) ;
        if(valread < 0){
            perror("recv header error");
            continue;
        }else if(valread == 0){
            shutdown(client_socket, SHUT_RDWR);
            break;
        }

        valread = recv(client_socket, &message->body, sizeof(MessageBody), 0);
        if(valread < 0){
            perror("recv body error");
            break;
        }

        if(message->header.file_name_size > 0){
            message->body.file_name = (char*)calloc(message->header.file_name_size, sizeof(char));
            if (!message->body.file_name) {
                perror("Failed to allocate file name memory");
                break;
            }
            valread = recv(client_socket, message->body.file_name, message->header.file_name_size, 0);
            if(valread < 0){
                perror("recv name error");
                break;
            }
        }

        if(message->header.content_size > 0){
            message->body.content = (char*)calloc(message->header.content_size, sizeof(char));
            if (!message->body.content) {
                perror("Failed to allocate content memory");
                break;
            }
            valread = recv(client_socket, message->body.content, message->header.content_size, 0);
            if(valread < 0){
                perror("recv content error");
                break;
            }
        }

        _Response* response = (_Response*)calloc(1, sizeof(_Response));
        if(!response){
            perror("response calloc error");
            break;
        }

        switch(message->body.type){
            case GETALL:
                command_getall(response);
                break;
            case DELETE:
                file_path = get_file_path(FILE_HOME, message->body.file_name, NULL);
                command_delete(message, file_path, response);
                free(file_path);
                break;
            case GET: case PUT:
                file_path = get_file_path(FILE_HOME, message->body.file_name, NULL);
                sync_file_io(message, file_path, response);
                free(file_path);
                break;
            default:
                log(log_file, LOG_LEVEL_ERROR, "%s\n", "Invalid command");
                make_response(response, ERROR, "Invalid command");
                break;
        }

        send(client_socket, &response->header, sizeof(response->header), 0);
        send(client_socket, &response->body, sizeof(response->body), 0);
        if(response->header.data_size > 0){
            send(client_socket, response->body.data, response->header.data_size, 0);
        }

        if(message->header.file_name_size>0){
            free(message->body.file_name);
        }
        if(message->body.content>0){
            free(message->body.content);
        }
        if(!message)
            free(message);

        if(response->header.data_size > 0){
            free(response->body.data);
        }
        if(!response)
            free(response);
	}
}

void sync_file_io(_Message* message, char* file_path, _Response* response){
	FILE* fp = open_and_seek_file(message, file_path);

	if(message->body.type == PUT){
        command_put(&fp, message, file_path, response);
    }else if(message->body.type == GET){
        command_get(fp, message->body.length, response);
    }
	if(fp){
	    fclose(fp);
	}
}

void command_get(FILE* fp, int length, _Response* response){
    if(!fp){
        log_error(log_file);
        make_response(response, ERROR, "The file could not be found.");
        return;
    }

    char* ret = NULL;
    int length_left = length;
    response->body.data = (char*)malloc(length * sizeof(char));

    while(length_left > 0 && !feof(fp)){
        char buffer[MAX_BUF_SIZE] = {0};
        int length_read = length_left > MAX_BUF_SIZE ? MAX_BUF_SIZE : length_left+1;
        ret = fgets(buffer, length_read, fp);

        if(!ret){
            if(feof(fp)) {
                break;
            }
            log_error(log_file);
            return;
        }

        if(!strcmp(ret,"\n")) continue;
        strcat(response->body.data, ret);
        length_left -= strlen(ret);
        if(buffer[strlen(ret)-1]=='\n') length_left++;
    }
    strcat(response->body.data, "\0");
    response->header.data_size = strlen(response->body.data);
}

void command_put(FILE** fp, _Message* message, char* file_path, _Response* response){
    if(!*fp){
        log_info(log_file, "Creating file...\n");
        *fp = fopen(file_path, "w+");
    }
    fputs(message->body.content, *fp);
    make_response(response, SUCCESS, "File written successfully.\n");
}

FILE* open_and_seek_file(_Message* message, char* file_path){
    FILE* fp = fopen(file_path, "r+");

    if(fp){
        log_info(log_file, "The file is opened\n");
        fseek(fp, message->body.offset, SEEK_SET);
    }

    return fp;
}

void command_delete(_Message* message, char* file_path, _Response* response){
    if(remove(file_path) == 0){
        make_response(response, SUCCESS, "file deleted successfully");
    }else{
        log(log_file, LOG_LEVEL_ERROR, "%s : %s could not be found.\n", strerror(errno),file_path);
        make_response(response, ERROR, "The file could not be found.");
    }
}

void command_getall(_Response* response){
    DIR* dp = NULL;
    struct dirent* dir;
    struct stat buf;

    if((dp = opendir(FILE_HOME)) == NULL){
        log_error(log_file);
        make_response(response, ERROR, "The directory could not be opened.");
        return;
    }

    long long total_size = 0;

    while((dir = readdir(dp)) != NULL){
        char* path = get_file_path(FILE_HOME, dir->d_name, NULL);
        if (lstat(path, &buf) != 0 || S_ISDIR(buf.st_mode)) {
            continue;
        }
        total_size += strlen(dir->d_name)+1;
        free(path);
    }
    response->header.data_size = total_size;
    response->body.data = (char*)malloc(response->header.data_size * sizeof(char));
    if(!response->body.data){
        log_error(log_file);
        return;
    }

    dp = opendir(FILE_HOME);
    while((dir = readdir(dp)) != NULL){
        char* path = get_file_path(FILE_HOME, dir->d_name, NULL);

        if (lstat(path, &buf) != 0 || S_ISDIR(buf.st_mode)) {
            continue;
        }

        strcat(response->body.data, dir->d_name);
        strcat(response->body.data, "\n");
        free(path);
    }
    closedir(dp);
}

char* get_file_path(const char* file_home, ...){
    va_list args;
    va_start(args, file_home);

    va_list args_copy;
    va_copy(args_copy, args);
    size_t length = compute_length(file_home, args_copy);
    va_end(args_copy);

    char* ret = (char*)malloc((length + 1) * sizeof(char));
    if (!ret) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    strcpy(ret, file_home);
    const char* str;
    while ((str = va_arg(args, const char*)) != NULL) {
        strcat(ret, str);
    }

    va_end(args);
    return ret;
}

size_t compute_length(const char* first, va_list args) {
    size_t length = strlen(first);
    const char* str;
    while ((str = va_arg(args, const char*)) != NULL) {
        length += strlen(str);
    }
    return length;
}

FILE* init_server(){
    mkdir(FILE_HOME,0755);
    mkdir(LOG_FILE_HOME,0755);

    char pid_str[10];
    snprintf(pid_str, sizeof(pid_str), "%d", getpid());
    char* log_file_name = get_file_path(LOG_FILE_HOME, "server_", pid_str, NULL);
    log_file = fopen(log_file_name, "a+");
    free(log_file_name);
    return log_file;
}