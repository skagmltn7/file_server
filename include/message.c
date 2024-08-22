#include "message.h"
#include <stdio.h>

char* getMode(MessageType type){
	switch(type){
		case GETALL:
			return"ra";
		case GET:
			return "r";
		case PUT:	
			return "a+";
		case DELETE:
			return "d";
	}
	return NULL;
}

MessageType getMessageType(char* input){
		if(strcmp(input,"GET")==0){
			return GET;
		}else if(strcmp(input, "GETALL")==0){
			return GETALL;
		}else if(strcmp(input, "DELETE")==0){
			return DELETE;
		}else if(strcmp(input, "PUT")==0){
			return PUT;
		}
	return UNKNOWN;
}

