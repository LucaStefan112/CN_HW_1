#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// FIFO names:
#define serverIn "SERVER_INPUT"
#define serverOut "SERVER_OUTPUT"

// Error messages:
#define fifoNotOppened "Could not open FIFO's!"
#define dataNotSent "Could not send data to server!"
#define dataNotReceived "Could not receive data from server!"

// System messages:
#define connected "Connected to server!\n"
#define comandExceedsLength "Command too long!\n"
#define unknownCommand "Command not found or invalid!\n"

// Comands:
#define loginPrefix "login : "
#define loggedUsers "get-logged-users"
#define procInfoPrefix "get-proc-info : "
#define logout "logout"
#define quit "quit"

// Max command size:
#define MAX_BUFFER_SIZE 50

// File descriptors for FIFO's:
int serverInput, serverOutput;

// Client token:
int token;

// Buffers for reading and writing data:
char terminalInput[100], serverData[100];

int openServerChannels(){
	// Opening FIFO's:
	serverInput = open(serverIn, O_WRONLY);
	serverOutput = open(serverOut, O_RDONLY);

	// Error checking:
	if(serverInput == -1 || serverOutput == -1){
		printf(fifoNotOppened);
		return 0;
	}

	printf(connected);

	return 1;
}

void readDataFromTerminal(){
	printf(">");
	fgets(terminalInput, sizeof(terminalInput), stdin);
	// Erase '\n' character at the end:
	terminalInput[strlen(terminalInput) - 1] = 0;
}

int isInputValid(){
	// Check input length:
	if(strlen(terminalInput) > MAX_BUFFER_SIZE){
		printf(comandExceedsLength);
		return 0;
	}
	// Validate command:
	else if(
		strstr(terminalInput, loginPrefix) != 0 &&
		strcmp(terminalInput, loggedUsers) &&
		strstr(terminalInput, procInfoPrefix) != 0 &&
		strcmp(terminalInput, logout) &&
		strcmp(terminalInput, quit)){
			printf(unknownCommand);
			return 0;
		}
	
	return 1;
}

int sendDataToServer(){
	// Send authentification token:
	if(write(serverInput, &token, sizeof(int)) == -1){
		printf(dataNotSent);
		return 0;
	}

	// Send buffer size:
	int dataSize = strlen(terminalInput);
	if(write(serverInput, &dataSize, sizeof(int)) == -1){
		printf(dataNotSent);
		return 0;
	}

	// Send actual data:
	if(write(serverInput, terminalInput, strlen(terminalInput)) == -1){
		printf(dataNotSent);
		return 0;
	}

	return 1;
}

int receiveDataFromServer(){
	// Get token from server:
	if(read(serverOutput, &token, sizeof(int)) == -1){
		printf(dataNotReceived);
		return 0;
	}

	// Get response buffer size:
	int bufferSize = 0;
	if(read(serverOutput, &bufferSize, sizeof(int)) == -1){
		printf(dataNotReceived);
		return 0;
	}

	// Get response:
	if(read(serverOutput, serverData, bufferSize) == -1){
		printf(dataNotReceived);
		return 0;
	}
}

void showData(){
	printf("%s\n", serverData);
}

void communicateWithServer(){
	// Infinite loop:
	while(1){
		readDataFromTerminal();
		
		if(isInputValid() && sendDataToServer() && receiveDataFromServer())
			showData();
		// If there is any error, quit:
		else 
			break;
		
		// Quit token:
		if(token == -1)
			break;
	}
}

void closeServerChannels(){
	// Close FIFO's:
	close(serverInput);
	close(serverOutput);
}

int main(){
	if(!openServerChannels())
		return 1;

	communicateWithServer();

	closeServerChannels();

	return 0;
}
