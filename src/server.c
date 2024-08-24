#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/message.h"

#define PORT 8080
#define BUFFER_SIZE 1024

void connect_client();
void sync_file_io(char* file_name, const char* content, const char* access_mode);

int main(int argc, char const* argv[]){
	connect_client();
	return 0;
}

void connect_client(){
	int server_fd, new_socket;
	ssize_t valread;
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
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		perror("bind failed");
		exit(EXIT_FAILURE);	
	}

	if(listen(server_fd, 3) <0){
		perror("listen");
		exit(EXIT_FAILURE);	
	}

	if((new_socket = accept(server_fd, (struct sockaddr*)&addr, &addrlen)) < 0 ){
		perror("accept");
		exit(EXIT_FAILURE);
	}

    printf("\nConnected to client: %s\n", inet_ntoa(addr.sin_addr));

	while(1){
		_Message* message = (_Message*)malloc(sizeof(_Message));
        if(message == NULL){
            printf("\nMemory allocation failed\n");
            break;
        }

		valread = recv(new_socket, message, sizeof(*message), 0);
		if(valread==0){
			close(new_socket);
			close(server_fd);
			printf("\nCLOSE CONNECTION: %s\n\n", inet_ntoa(addr.sin_addr));
			break;
		}

		//sync_file_io("test.txt", buffer, getMode(message->header.type));
        free(message);
	}	
}

void sync_file_io(char* file_name, const char* content, const char* access_mode){
	FILE* fp;
	
	if((fp = fopen(file_name, access_mode))==NULL){
		printf("\n can not open this file \n\n");
		return;
	}

	printf("The file is opened\n");
	
	if(strlen(content)>0){
		fputs(content, fp);
		fputs("\n",fp);
	}

	fclose(fp);
}
