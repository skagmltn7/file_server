#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080
#define BUFFER_SIZE 1024

void connect_client();

int main(int argc, char const* argv[]){
	connect_client();
	return 0;
}

void connect_client(){
	int server_fd, new_socket;
	ssize_t valread;
	struct sockaddr_in addr;
	int opt = 1;
	socklen_t addrlen = sizeof(addr);

	if((server_fd = socket(AF_INET, SOCK_STREAM,0))<0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if(setsockopt(server_fd, SOL_SOCKET, 
				SO_REUSEADDR | SO_REUSEPORT,
				&opt, sizeof(opt))){
			perror("setsockopt");
			exit(EXIT_FAILURE);	
	}
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		perror("bind failed");
		exit(EXIT_FAILURE);	
	}

	if(listen(server_fd, 3) <0){
		perror("listen");
		exit(EXIT_FAILURE);	
	}

	if((new_socket = accept(server_fd, (struct sockaddr*)&addr, &addrlen)) < 0 ){
		perror("accept");
		exit(EXIT_FAILURE);
	}

//	printf("successful connect with client: %d:%d\n",addr.sin_addr, PORT);
	for(;;){

		char buffer[BUFFER_SIZE]={0};
		valread = read(new_socket, buffer, BUFFER_SIZE-1);
		if(valread==0){
			close(new_socket);
			close(server_fd);	
			break;
		}
		printf("buffer: %s\n", buffer);
		printf("valread: %ld\n", valread);
	}
	
}
