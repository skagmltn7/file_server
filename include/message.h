#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <stddef.h>
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
	MessageType type;
	char file_name[256];
} MessageHeader;

typedef struct{
	long offset;
	size_t length;
	char *content;
} MessageBody;

typedef struct{
	MessageHeader header;
	MessageBody body;
} _Message;

char* getMode(MessageType type);
MessageType getMessageType(char* input);
#endif
