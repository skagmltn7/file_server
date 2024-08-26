#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/message.h"

#define PORT 8080
#define MAX_BUF_SIZE 1024

void connect_client();
void sync_file_io(_Message* message);
void exec_command(int new_socket);
void command_get(FILE* fp, int length);

int main(int argc, char const* argv[]){
	connect_client();
	return 0;
}

void connect_client(){
    int server_fd, new_socket;
	struct sockaddr_in addr;
	int opt = 1;
	socklen_t addrlen = sizeof(addr);

	if((server_fd = socket(AF_INET, SOCK_STREAM,0))<0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if(setsockopt(server_fd, SOL_SOCKET, 
				SO_REUSEADDR | SO_REUSEPORT,
				&opt, sizeof(opt))){
			perror("setsockopt");
			exit(EXIT_FAILURE);	
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		perror("bind failed");
		exit(EXIT_FAILURE);	
	}

	if(listen(server_fd, 1) <0){
		perror("listen");
		exit(EXIT_FAILURE);	
	}

	if((new_socket = accept(server_fd, (struct sockaddr*)&addr, &addrlen)) < 0 ){
		perror("accept");
		exit(EXIT_FAILURE);
	}

    printf("\nConnected to client: %s\n", inet_ntoa(addr.sin_addr));
    exec_command(new_socket);

    printf("\nCLOSE CONNECTION: %s\n\n", inet_ntoa(addr.sin_addr));
    close(new_socket);
    close(server_fd);
}

void exec_command(int new_socket){
    ssize_t valread;
    _Message* message = NULL;

	while(1){
		message = (_Message*)malloc(sizeof(_Message));
        if(message == NULL){
            printf("\nMemory allocation failed\n");
            return;
        }

        valread = recv(new_socket, message, sizeof(*message), 0);
        if(valread<=0){
            free(message);
            return;
        }
        sync_file_io(message);
        free(message);
	}
}

void sync_file_io(_Message* message){
	FILE* fp = NULL;

	if((fp = fopen(message->header.file_name, getMode(message->header.type)))==NULL){
		perror("\n can not open this file ");
		return ;
	}

	printf("\nThe file is opened\n\n");
	fseek(fp, message->body.offset, SEEK_SET);

	if(message->header.type == PUT){
	    if(strlen(message->body.content)>0){
		    fputs(message->body.content, fp);
        }
	}else if(message->header.type == GET){
        command_get(fp, message->body.length);
	}

	fclose(fp);
}

void command_get(FILE* fp, int length){
    char* ret = NULL;
    int length_left = length;

    while(length_left > 0 && !feof(fp)){
        char buffer[MAX_BUF_SIZE] = {0};
        int length_read = length > MAX_BUF_SIZE ? MAX_BUF_SIZE : length_left + 1;
        ret = fgets(buffer, length_read, fp);

        if(ret == NULL){
            if(feof(fp)) {
                break;
            }
            perror("fgets");
            return;
        }

        if(!strcmp(ret,"\n")) continue;
        printf("%s",ret);
        length_left -= strlen(ret);
        if(buffer[strlen(ret)-1]=='\n') length_left++;
    }
    printf("\n");
}
