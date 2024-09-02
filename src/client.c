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
_Message* parse_command(char* input_buffer);
int check_too_many_args(char* token);
void send_message(int client_fd, _Message* message);

int main(int argc, char const* argv[]){
    connect_server();
    return 0;
}

void connect_server(){
	int status, client_fd;
	struct sockaddr_in server_addr;

	if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("\n Can not create socket");
		return;
	}

    memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	if(inet_pton(AF_INET, LOCAL_HOST, &server_addr.sin_addr) <= 0){
		perror("\n Unsupported / Invalid address");
		return;
	}

	status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(status < 0){
		perror("\n Connection Failed");
		return;
	}

	exec_command(client_fd);
	shutdown(client_fd, SHUT_RDWR);
	close(client_fd);
}

void exec_command(int client_fd){
    ssize_t valread;
    _Message* message;
    _Response* response;
    char input_buffer[BUF_SIZE];

    while(1){
        printf("CLIENT> ");

        memset(input_buffer, 0, sizeof(input_buffer));
        rewind(stdin);
        scanf(" %[^;]",input_buffer);
        getchar();

        if(is_exit(input_buffer)){
            return;
        }

        message = parse_command(input_buffer);
        if(message){
            send_message(client_fd, message);

            response = (_Response*)calloc(1, sizeof(_Response));
            valread = recv(client_fd, &response->header, sizeof(response->header), 0);
            if(valread <= 0){
            // TODO
                continue;
            }

            valread = recv(client_fd, &response->body, sizeof(response->body), 0);
            if(valread <= 0){
            // TODO
                continue;
            }

            if(response->body.status == ERROR){
                printf("\n[ERROR]\n");
            }else{
                printf("\n[DATA]\n");
            }

            if(response->header.data_size > 0){
                response->body.data = (char*)calloc(response->header.data_size, sizeof(char));
                valread = recv(client_fd, response->body.data, response->header.data_size, 0);
                if(valread <= 0){
                // TODO
                    continue;
                }

                printf("\n%s\n\n", response->body.data);
            }

            if(message->header.file_name_size>0)
                free(message->body.file_name);
            if(message->header.content_size>0)
                free(message->body.content);
            if(message)
                free(message);

            if(response->header.data_size>0)
                free(response->body.data);
            if(response)
            free(response);
        }
    }
}

_Message* parse_command(char* input_buffer){
    _Message* message = (_Message*)malloc(sizeof(_Message));
    message->body.file_name = NULL;
    message->body.content = NULL;

    char* token = strtok(input_buffer,DELIM);
    if(!token){
        syntax_error("Empty command");
        return NULL;
    }
    message->body.type = get_message_type(to_upper_case(token, strlen(token)));

    if(message->body.type == UNKNOWN){
        syntax_error("Undefined Command");
        return NULL;
    }

    if(message->body.type != GETALL){
        token = strtok(NULL, DELIM);
        if(!token){
            syntax_error("Missing arguments");
            return NULL;
        }
        message->header.file_name_size = (strlen(token)+1) * sizeof(char);
        message->body.file_name = (char*)calloc(strlen(token)+1, sizeof(char));
        strncpy(message->body.file_name, token, strlen(token)+1);

        if(message->body.type != DELETE){
            token = strtok(NULL, DELIM);
            if(!token){
                syntax_error("Missing arguments");
                return NULL;
            }
            char* endptr;
            message->body.offset = strtol(token, &endptr, 10);

            if(message->body.type == GET){
                token = strtok(NULL, DELIM);
                if(!token){
                    syntax_error("Missing arguments");
                    return NULL;
                }
                message->body.length = atoi(token);
            }else if(message->body.type == PUT){
                token = strtok(NULL, "");
                if(!token){
                    syntax_error("Missing arguments");
                    return NULL;
                }
                message->body.content = (char*)calloc(strlen(token)+1, sizeof(char));
                strncpy(message->body.content, token, strlen(token)+1);
                message->header.content_size = (strlen(token)+1) * sizeof(char);
            }
        }
    }

    token = strtok(NULL, DELIM);
    if(token){
        syntax_error("Too many arguments");
        return NULL;
    }
    return message;
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
}

void send_message(int client_fd, _Message* message){
    send(client_fd, &message->header, sizeof(message->header), 0);
    send(client_fd, &message->body, sizeof(message->body), 0);

    if(message->header.file_name_size>0){
        if(send(client_fd, message->body.file_name, message->header.file_name_size, 0) == -1){
            perror("send file_name");
            return;
        }
    }

    if(message->header.content_size>0){
        if(send(client_fd, message->body.content, message->header.content_size, 0) == -1){
            perror("send content");
            return;
        }
    }
}