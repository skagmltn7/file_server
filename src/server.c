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
#include <pthread.h>
#include <sys/ioctl.h>

#include "../include/message.h"
#include "../include/logging.h"
#include "../include/job.h"

#define PORT 8080
#define MAX_BUF_SIZE 1024
#define FILE_HOME "./file/"
#define LOG_FILE_HOME "./log/"
#define WTHR_NUM 5

FILE* log_file;

void sync_file_io(_Message* message, char* file_path, _Response* response);
_Message* create_message(int new_socket);
void command_get(FILE* fp, int length, _Response* response);
void command_put(FILE** fp, _Message* message, char* file_path, _Response* response);
FILE* open_and_seek_file(_Message* message, char* file_path);
void command_delete(_Message* message, char* file_path, _Response* response);
void command_getall(_Response* response);
char* get_file_path(const char* file_home, ...);
size_t compute_length(const char* first, va_list args);
FILE* init_server();
void* cthr_func(void* arg);
void* wthr_func(void* arg);
void process_job(Job* job);
void send_response(Job* job, _Response* response);

int main(int argc, char const* argv[]){
    log_file = init_server();
    log_info(log_file, "Starting server...\n");

    pthread_t cthr;

    pthread_create(&cthr, NULL, cthr_func, NULL);
    pthread_join(cthr, NULL);

	log_info(log_file, "Shutdown server...\n");
	fclose(log_file);
	return 0;
}

void* cthr_func(void* arg){
    int listening_socket, client_socket;
	struct sockaddr_in addr;
	int opt = 1;
	socklen_t addrlen = sizeof(addr);
	fd_set read_fds, copy_rdfds;
	struct timeval timeout;

    pthread_t wthr[WTHR_NUM];
    JobQueue* queue = (JobQueue*)malloc(sizeof(JobQueue));
    init_queue(queue);

    for(int i=0; i < WTHR_NUM; i++){
        pthread_create(&wthr[i], NULL, wthr_func, queue);
    }

	if((listening_socket = socket(AF_INET, SOCK_STREAM,0))<0){
        log_error(log_file);
		exit(EXIT_FAILURE);
	}

	if(setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        log_error(log_file);
        exit(EXIT_FAILURE);
	}

    if(ioctl(listening_socket, FIONBIO, (char*)&opt) < 0){
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

    FD_ZERO(&read_fds);
    FD_SET(listening_socket, &read_fds);
    int max_sd = listening_socket;

    while (1) {
        copy_rdfds = read_fds;

        if ((select(max_sd + 1, &copy_rdfds, NULL, NULL, NULL) < 0) && (errno != EINTR)) {
            log_error(log_file);
            break;
        }
        for(int i = 0; i <= max_sd; i++){
            if (FD_ISSET(i, &copy_rdfds)) {
                if(i == listening_socket){
                    if ((client_socket = accept(listening_socket, (struct sockaddr*)&addr, &addrlen)) < 0) {
                        log_error(log_file);
                        continue;
                    }
                    FD_SET(client_socket, &read_fds);
                    if (client_socket > max_sd) {
                        max_sd = client_socket;
                    }
                    printf("\nConnected\n");
                }else{
                    _Message* message = create_message(i);
                    if(!message){
                        close(i);
                        FD_CLR(i, &read_fds);
                        continue;
                    }
                    printf("\n===========\n");
                    print_message(message);
                    printf("\n===========\n");
                    Job* job = (Job*)malloc(sizeof(Job));
                    if(!job){
                        perror("job malloc error");
                        continue;
                    }
                    job->client_socket = i;
                    job->message = message;
                    push(queue, job);
                }
            }
        }
    }
    close(listening_socket);
    free(queue);
    return NULL;
}

_Message* create_message(int client_socket){
    ssize_t valread;

    _Message* message = (_Message*)calloc(1, sizeof(_Message));
    if(!message){
        perror("message calloc error");
        return NULL;
    }

    valread = recv(client_socket, &message->header, sizeof(message->header), 0);
    if(valread < 0){
        perror("recv header error");
        return NULL;
    }else if(valread == 0){
        shutdown(client_socket, SHUT_RDWR);
        printf("\nUnconnected\n");
        return NULL;
    }

    if(message->header.file_name_size > 0){
        message->file_name = (char*)calloc(message->header.file_name_size, sizeof(char));
        if (!message->file_name) {
            perror("Failed to allocate file name memory");
            return NULL;
        }
        valread = recv(client_socket, message->file_name, message->header.file_name_size, 0);
        if(valread < 0){
            perror("recv file name error");
            return NULL;
        }
    }

    if(message->header.content_size > 0){
        message->content = (char*)calloc(message->header.content_size, sizeof(char));
        if (!message->content) {
            perror("Failed to allocate content memory");
            return NULL;
        }
        valread = recv(client_socket, message->content, message->header.content_size, 0);
        if(valread < 0){
            perror("recv content error");
            return NULL;
        }
    }
    return message;
}

void process_job(Job* job){
    char* file_path = NULL;
    _Message* message = job->message;
    _Response* response = (_Response*)calloc(1, sizeof(_Response));
    if(!response){
        perror("response calloc error");
        return;
    }
    switch(message->header.type){
        case GETALL:
            command_getall(response);
            break;
        case DELETE:
            file_path = get_file_path(FILE_HOME, message->file_name, NULL);
            command_delete(message, file_path, response);
            free(file_path);
            break;
        case GET: case PUT:
            file_path = get_file_path(FILE_HOME, message->file_name, NULL);
            sync_file_io(message, file_path, response);
            free(file_path);
            break;
        default:
            log(log_file, LOG_LEVEL_ERROR, "%s\n", "Invalid command");
            make_response(response, ERROR, "Invalid command");
            break;
    }
    send_response(job, response);
}

void send_response(Job* job, _Response* response){
    send(job->client_socket, &response->header, sizeof(response->header), 0);
    if(response->header.data_size > 0){
        send(job->client_socket, response->data, response->header.data_size, 0);
    }
    free_response(response);
}

void sync_file_io(_Message* message, char* file_path, _Response* response){
	FILE* fp = open_and_seek_file(message, file_path);

	if(message->header.type == PUT){
        command_put(&fp, message, file_path, response);
    }else if(message->header.type == GET){
        command_get(fp, message->header.length, response);
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
    response->data = (char*)malloc(length * sizeof(char));

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
        strcat(response->data, ret);
        length_left -= strlen(ret);
        if(buffer[strlen(ret)-1]=='\n') length_left++;
    }
    strcat(response->data, "\0");
    response->header.data_size = strlen(response->data)+1;
}

void command_put(FILE** fp, _Message* message, char* file_path, _Response* response){
    if(!*fp){
        log_info(log_file, "Creating file...\n");
        *fp = fopen(file_path, "w+");
    }
    fputs(message->content, *fp);
    make_response(response, SUCCESS, "File written successfully.\n");
}

FILE* open_and_seek_file(_Message* message, char* file_path){
    FILE* fp = fopen(file_path, "r+");

    if(fp){
        log_info(log_file, "The file is opened\n");
        fseek(fp, message->header.offset, SEEK_SET);
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
    response->data = (char*)malloc(response->header.data_size * sizeof(char));
    if(!response->data){
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
        strcat(response->data, dir->d_name);
        strcat(response->data, "\n");
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

void* wthr_func(void* arg) {
    JobQueue* queue = (JobQueue*)arg;
    while (1) {
        Job* job = pop(queue);
        if (job != NULL) {
            process_job(job);
            if(job->message->file_name)
                free(job->message->file_name);
            if(job->message->content)
                free(job->message->content);
            if(job->message)
                free(job->message);
            free(job);
        }
    }
}