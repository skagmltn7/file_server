#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <string.h>
#include <stdlib.h>

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
	// FD 매핑정보
} MessageHeader;

typedef struct{
    MessageType type;
	char* file_name;
    long offset;
	int length;
	char* content;
} MessageBody;

typedef struct{
	MessageHeader header;
	MessageBody body;
} _Message;

typedef enum{
    SUCCESS,
    ERROR
} ResponseStatus;

typedef struct{
    size_t data_size;
} ResponseHeader;

typedef struct{
    ResponseStatus status;
	char* data;
} ResponseBody;

typedef struct{
	ResponseHeader header;
	ResponseBody body;
} _Response;

MessageType get_message_type(char* input);
void make_response(_Response* response, ResponseStatus status, char* data);
#endif
