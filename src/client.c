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

_Bool is_exit(char* str);
char* to_upper_case(char* str, int len);
void syntax_error(char* cause);

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

	while(1){
		printf("CLIENT> ");

		char input_buffer[BUF_SIZE];
		scanf(" %[^;]",input_buffer);
		getchar();

		if(is_exit(input_buffer)){
			close(client_fd);
			break;
		}

        char* token = strtok(input_buffer,DELIM);
        _Message* message = (_Message*)malloc(sizeof(_Message));
        if(message == NULL){
            printf("Memory allocation failed\n");
            return -1;
        }
		message->header.type = getMessageType(to_upper_case(token, strlen(token)));

		if(message->header.type == UNKNOWN){
		    syntax_error("Undefined Command");
		    continue;
		}

        token = strtok(NULL, DELIM);

        if(message->header.type == GETALL){
            if(token != NULL){
                syntax_error("Too many arguments");
                continue;
            }
        }else{ // GET, DELETE, PUT
            if(token == NULL){
                syntax_error("Missing arguments");
                continue;
            }
            strncpy(message->header.file_name, token, strlen(token));

            token = strtok(NULL, DELIM);
            if(message->header.type == DELETE){
                if(token != NULL){
                    syntax_error("Too many arguments");
                    continue;
                }
            }else{
                if(token == NULL){
                    syntax_error("Missing arguments");
                    continue;
                }

                char* endptr;
                message->body.offset = strtol(token, &endptr, 10);

                if(message->header.type == GET){
                    token = strtok(NULL, DELIM);
                    if(token == NULL){
                        syntax_error("Missing arguments");
                        continue;
                    }
                    message->body.length = atoi(token);

                    if((token = strtok(NULL, DELIM))!=NULL){
                        syntax_error("Too many arguments");
                        continue;
                    }
                }else if(message->header.type == PUT){
                    token = strtok(NULL, "");
                    strncpy(message->body.content, token, strlen(token));
                }
            }
        }

		send(client_fd, message, sizeof(*message), 0);
		free(message);
		printf("\n successful send to server \n\n");
	}

	return 0;	
}

_Bool is_exit(char* str){
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