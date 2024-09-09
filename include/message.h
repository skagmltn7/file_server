#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

typedef enum{
	GETALL,
	GET,
	DELETE,
	PUT,
	UNKNOWN
} MessageType;

typedef struct{
    size_t file_name_size;
    size_t content_size;
    MessageType type;
    long offset;
	int length;
} MessageHeader;

typedef struct{
	char* content;
	char* file_name;
    MessageHeader header;
} _Message;

typedef enum{
    SUCCESS,
    ERROR
} ResponseStatus;

typedef struct{
    size_t data_size;
    ResponseStatus status;
} ResponseHeader;

typedef struct{
	char* data;
	ResponseHeader header;
} _Response;

MessageType get_message_type(char* input);
void make_response(_Response* response, ResponseStatus status, char* data);
void free_message(_Message* message);
void free_response(_Response* response);
ssize_t recv_full(int sockfd, void *buffer, size_t length);
#endif
