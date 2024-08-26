#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <string.h>
#include <stdlib.h>

#define MAX_FILE_NAME_SIZE 256
#define MAX_MESSAGE_SIZE 1024

typedef enum{
	GETALL,
	GET,
	DELETE,
	PUT,
	UNKNOWN
} MessageType;

typedef struct{
	MessageType type;
	char file_name[MAX_FILE_NAME_SIZE];
} MessageHeader;

typedef struct{
	long offset;
	int length;
	char content[MAX_MESSAGE_SIZE];
} MessageBody;

typedef struct{
	MessageHeader header;
	MessageBody body;
} _Message;

MessageType getMessageType(char* input);
#endif
