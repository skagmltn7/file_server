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