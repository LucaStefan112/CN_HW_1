#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// FIFO names:
#define serverIn "SERVER_INPUT"
#define serverOut "SERVER_OUTPUT"

// Error messages:
#define fifoNotOppened "Could not open FIFO's!\n"
#define dataNotSent "Could not send data to server!\n"
#define dataNotReceived "Could not receive data from server!\n"

// System messages:
#define started "Client started!\n"
#define connected "Connected to server!\n"
#define comandExceedsLength "Command too long!\n"
#define commandMustBeAlphanum "Command must be use safe characters(a/A-z/Z, 0-9, '-', '_', ' ', ':')!\n"

// Comands:
#define quit "quit"

// Max command size:
#define MAX_BUFFER_SIZE 50

// Safe characters list:
#define safeCharacters "_- :,.+=/?!@#$"

// File descriptors for FIFO's:
int serverInput, serverOutput;

// Client token:
int token;

// Buffers for reading and writing data:
char terminalInput[100], serverData[100];

int openServerChannels(){

	printf(started);
	
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
	// Check empty command:
	if(strlen(terminalInput) == 0)
		return 0;

	// Check input length:
	if(strlen(terminalInput) > MAX_BUFFER_SIZE){
		printf(comandExceedsLength);
		
		// Empty stdin buffer:
		int c;
		while ((c = getchar()) != '\n' && c != EOF);

		return 0;
	}

	// Check if input is alfa-num or safe character:
	for(int i = 0; terminalInput[i]; i++)
		if(!(
			'a' <= terminalInput[i] && terminalInput[i] <= 'z' ||
			'A' <= terminalInput[i] && terminalInput[i] <= 'Z' ||
			'0' <= terminalInput[i] && terminalInput[i] <= '9' ||
			strchr(safeCharacters, terminalInput[i])
		)){
			printf(commandMustBeAlphanum);
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
