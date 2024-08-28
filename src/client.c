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
#define EXIT "EXIT"
#define DELIM " \n"

int is_exit(char* str);
char* to_upper_case(char* str, int len);
void syntax_error(char* cause);
void connect_server();
void exec_command(int client_fd);
int parse_command(_Message* message, char* input_buffer);
int parse_file_name(_Message* message, char* token);
int parse_offset(_Message* message, char* token);
int parse_length(_Message* message, char* token);
int parse_content(_Message* message, char* token);
int check_too_many_args(char* token);

int main(int argc, char const* argv[]){
    connect_server();
    return 0;
}

void connect_server(){
	int status, client_fd;
	struct sockaddr_in server_addr;

	if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("\n Can not create socket \n");
		return;
	}

    bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	if(inet_pton(AF_INET, LOCAL_HOST, &server_addr.sin_addr) <= 0){
		perror("\n Unsupported / Invalid address \n");
		return;
	}

	status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(status < 0){
		perror("\n Connection Failed \n");
		return;
	}

	exec_command(client_fd);
	close(client_fd);
}

void exec_command(int client_fd){
    _Message* message = (_Message*)malloc(sizeof(_Message));
    if (message == NULL) {
        perror("Memory allocation failed\n");
        return;
    }

    while(1){
        printf("CLIENT> ");

        char input_buffer[BUF_SIZE];
        scanf(" %[^;]",input_buffer);
        getchar();

        if(is_exit(input_buffer)){
            break;
        }

        memset(message, 0, sizeof(_Message));
        if(parse_command(message, input_buffer)){
            send(client_fd, message, sizeof(*message), 0);
            printf("\n successful send to server \n\n");
        }
    }
    free(message);
}

int parse_command(_Message* message, char* input_buffer){
    char* token = strtok(input_buffer,DELIM);
    message->header.type = getMessageType(to_upper_case(token, strlen(token)));

    switch(message->header.type){
        case GETALL:
            return check_too_many_args(token);
        case DELETE:
            if(!parse_file_name(message, token))return 0;
            return check_too_many_args(token);
        case GET:
            if(!parse_file_name(message, token))return 0;
            if(!parse_offset(message, token))return 0;
            if(!parse_length(message, token))return 0;
            return check_too_many_args(token);
        case PUT:
            if(!parse_file_name(message, token)) return 0;
            if(!parse_offset(message, token)) return 0;
            if(!parse_content(message, token)) return 0;
            return 1;
        default:
            syntax_error("Undefined Command");
            return 0;
    }
}

int parse_file_name(_Message* message, char* token){
    token = strtok(NULL, DELIM);

    if(token == NULL){
        syntax_error("Missing arguments");
        return 0;
    }

    strncpy(message->header.file_name, token, strlen(token));
    return 1;
}

int parse_offset(_Message* message, char* token){
    token = strtok(NULL, DELIM);
    if(token == NULL){
        syntax_error("Missing arguments");
        return 0;
    }

    char* endptr;
    message->body.offset = strtol(token, &endptr, 10);
    return 1;
}

int parse_length(_Message* message, char* token){
    token = strtok(NULL, DELIM);
    if(token == NULL){
        syntax_error("Missing arguments");
        return 0;
    }
    message->body.length = atoi(token);
    return 1;
}

int parse_content(_Message* message, char* token){
    token = strtok(NULL, "");
    strncpy(message->body.content, token, strlen(token));
    return 1;
}

int check_too_many_args(char* token){
    token = strtok(NULL, DELIM);
    if(token){
        syntax_error("Too many arguments");
        return 0;
    }
    return 1;
}

int is_exit(char* str){
    return strlen(str)==strlen(EXIT)&& strcmp(EXIT, to_upper_case(str, strlen(str))) == 0;
}

char* to_upper_case(char* str, int len){
	char* res = (char*)malloc(len * sizeof(char));

	for(int i=0; i<len; i++){
		res[i] = str[i];
		if(str[i]>='a' && str[i]<='z'){
			res[i] -= 'a' - 'A';
		}
	}
	return res;
}

void syntax_error(char* cause){
    printf("\n syntax error: %s \n\n", cause);
    rewind(stdin);
}