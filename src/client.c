#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#include "../include/message.h"

#define PORT 8080
#define BUF_SIZE 1024
#define LOCAL_HOST "127.0.0.1"
#define EXIT "exit"

_Bool start_with(char* pre, char* str, int strlen);
char* tolowercase(char* str, int len);

int main(int argc, char const* argv[]){
	int status, client_fd;
	struct sockaddr_in server_addr;

	if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("\n Can not create socket \n");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	if(inet_pton(AF_INET, LOCAL_HOST, &server_addr.sin_addr) <= 0){
		printf("\n Unsupported / Invalid address \n");
		return -1;
	}

	status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(status < 0){
		printf("\n Connection Failed \n");
		return -1;
	}

	for(;;){
		printf("CLIENT> ");

		char input_buffer[BUF_SIZE];
		scanf(" %[^;]",input_buffer);
		getchar();
		
		if(start_with(EXIT, tolowercase(input_buffer,strlen(EXIT)), strlen(EXIT))){
			close(client_fd);
			break;			
		}
		
		_Message message;
		
		message.header.type = getMessageType(input_buffer);
		printf("%s\n",getMode(message.header.type));	
		send(client_fd, input_buffer, strlen(input_buffer), 0);
		
		printf("\n successful send to server \n\n");
	}

	return 0;	
}

char* tolowercase(char* str, int len){
	char* res = (char*)malloc(len * sizeof(char));

	for(int i=0; i<len; i++){
		res[i] = str[i];
		if(str[i]>='A' && str[i]<='Z'){
			res[i] += 'a' - 'A';
		}
	}

	return res;
}

_Bool start_with(char* pre, char* str, int strlen){
	return strncmp(pre, str, strlen) == 0;
}
