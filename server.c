#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

// FIFO names:
#define serverIn "SERVER_INPUT"
#define serverOut "SERVER_OUTPUT"

// Users list file:
#define usersFile "USERS"

// Error messages:
#define fifoNotOppened "Could not open FIFO's!"
#define dataNotSent "Could not send data to client!"
#define dataNotReceived "Could not receive data from client!"
#define bufferSizeMismatch "The buffer size does not match the payload value!"
#define bufferOverflow "The request is too large!"
#define invalidRequest "The request is invalid!"

// Comands:
#define loginPrefix "login : "
#define loggedUsers "get-logged-users"
#define procInfoPrefix "get-proc-info : "
#define logout "logout"
#define quit "quit"

// Maximum buffer size:
#define MAX_BUFFER_SIZE 50

// File descriptors for FIFO's:
int serverInput, serverOutput;

// Buffer size:
int bufferSize;

// Buffers for reading and writing data:
char request[100], response[100];

// List of all logged users;
char loggedUsersList[100][100];

// List of logged users tokens:
int loggedUsersTokens[100], numberOfLoggedUsers;

void createFifos(){
	mkfifo(serverIn, 0666);
	mkfifo(serverOut, 0666);
}

int openServerChannels(){
	// Opening FIFO's:
	int serverInput = open(serverIn, O_RDONLY);
	int serverOutput = open(serverOut, O_WRONLY);

	// Error checking:
	if(serverInput == -1 || serverOutput == -1){
		printf(fifoNotOppened);
		return 0;
	}

	printf("New client connected!\n");

	return 1;
}

int readDataFromClient(){
	// Get request's buffer size:
	if(read(serverInput, &bufferSize, 4) == -1 || read(serverInput, request, sizeof(request)) == -1){
		printf(dataNotReceived);
		return 0;
	}
	return 1;
}

int isRequestValid(){
	if(bufferSize != strlen(request)){
		printf(bufferSizeMismatch);
		return 0;
	}
	else if(bufferSize > MAX_BUFFER_SIZE){
		printf(bufferOverflow);
		return 0;
	}
	else if(
		strstr(request, loginPrefix) != 0 ||
		strcmp(request, loggedUsers) ||
		strstr(request, procInfoPrefix) != 0 ||
		strcmp(request, logout) ||
		strcmp(request, quit))
			printf(invalidRequest);
			return 0;
	return 1;
}

int respondToCLient(){
	// Command "login : username":
	if(strstr(request, "login : ") == 0){
		// Getting username from request:
		char user[100];
		strcpy(user, request + strlen("login : 0"));


	}
}

void communicateWithClient(){
	//Infinite loop:
	while(1){
		// If there is any error:
		if(!readDataFromClient() || !isRequestValid() || !respondToCLient())
			break;

		if(mustQuit())
			break;
	}
}

int main(){
	createFifos();

	if(!openServerChannels())
		return 1;

	communicateWithClient();

	return 0;
}
