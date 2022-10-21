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

// Max command size:
#define MAX_BUFFER_SIZE 50

// File descriptors for FIFO's:
int serverInput, serverOutput;

// Buffers for reading and writing data:
char terminalInput[100], serverData[100];

int openServerChannels(){
	// Opening FIFO's:
	int serverInput = open(serverIn, O_WRONLY);
	int serverOutput = open(serverOut, O_RDONLY);

	// Error checking:
	if(serverInput == -1 || serverOutput == -1){
		printf(fifoNotOppened);
		return 0;
	}

	printf("Connected to server!\n");

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
	if(strlen(terminalInput) > MAX_BUFFER_SIZE + 1){
		printf("Command too long!\n");
		return 0;
	}
	
	return 1;
}

int sendDataToServer(){
	// Send first buffer size:
	int dataSize = strlen(terminalInput);

	if(write(serverInput, &dataSize, 4) == -1){
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
	// Get response from server:
	if(read(serverOutput, serverData, sizeof(serverData)) == -1){
		printf(dataNotReceived);
		return 0;
	}
}

void showData(){
	printf("%s\n", serverData);
}

int mustQuit(){
	// Checks if the last command was to quit the app:
	return strcmp("quit", terminalInput);
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
		
		if(mustQuit())
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
