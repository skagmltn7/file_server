#include "message.h"

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
    strcpy(response->body.data,data);
}

