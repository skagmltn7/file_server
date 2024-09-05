#include "message.h"
#include <stdio.h>

MessageType get_message_type(char* input){
		if(strcmp(input,"GET")==0){
			return GET;
		}else if(strcmp(input, "GETALL")==0){
			return GETALL;
		}else if(strcmp(input, "DELETE")==0){
			return DELETE;
		}else if(strcmp(input, "PUT")==0){
			return PUT;
		}else return UNKNOWN;
}

void make_response(_Response* response, ResponseStatus status, char* data){
    response->header.status = status;
    response->header.data_size = strlen(data)+1;
    response->data = (char*)calloc(response->header.data_size, sizeof(char));
    strncpy(response->data, data, response->header.data_size);
}


void free_message(_Message* message){
    if(message){
        if(message->file_name){
            free(message->file_name);
            message->file_name = NULL;
        }
        if(message->content){
            free(message->content);
            message->content = NULL;
        }
        free(message);
        message = NULL;
    }
}

void free_response(_Response* response){
    if(response){
        if(response->data){
            free(response->data);
            response->data = NULL;
        }
        free(response);
        response = NULL;
    }
}

ssize_t recv_full(int sockfd, void *buffer, size_t length) {
    size_t total_received = 0;
    ssize_t bytes_received;

    while (total_received < length) {
        bytes_received = recv(sockfd, (char*)buffer + total_received, length - total_received, 0);
        if (bytes_received < 0) {
            perror("recv error");
            return -1;
        } else if (bytes_received == 0) {
            break;
        }

        total_received += bytes_received;
    }

    return total_received;
}

ssize_t send_full(int sockfd, const void *buffer, size_t length) {
    size_t total_sent = 0;
    ssize_t bytes_sent;

    while (total_sent < length) {
        bytes_sent = send(sockfd, (char*)buffer + total_sent, length - total_sent, 0);
        if (bytes_sent < 0) {
            perror("send error");
            return -1;
        }
        total_sent += bytes_sent;
    }
    return total_sent;
}