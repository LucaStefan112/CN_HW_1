#include "./all_libs.h"

// File descriptors for FIFO's:
int serverInput, serverOutput;

// Client token:
int token;
bool a;
// Buffers for reading and writing data:
char terminalInput[MAX_BUFFER_SIZE], serverData[MAX_BUFFER_SIZE];

// Quit signal:
int quitting = 0;

int openServerChannels(){

	printf(clientStarted);
	
	// Opening FIFO's:
	serverInput = open(serverIn, O_WRONLY);
	serverOutput = open(serverOut, O_RDONLY);

	// Error checking:
	if(serverInput == -1 || serverOutput == -1){
		printf(fifoNotOppened);
		return 0;
	}

	printf(connectedToServer);

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
	if(write(serverInput, terminalInput, dataSize) == -1){
		printf(dataNotSent);
		return 0;
	}
	
	return 1;
}

int receiveDataFromServer(){
	strcpy(serverData, "");

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

	// Use first "bufferSize" characters:
	serverData[bufferSize] = 0;

	// Quit token:
	if(token == -1)
		quitting = 1;

	printf("%d %d %s\n", token, bufferSize, serverData);

	return 1;
}

void showData(){

	printf("%s", serverData);
}

void communicateWithServer(){
	// Infinite loop:
	while(!quitting){
		readDataFromTerminal();
		if(isInputValid() && sendDataToServer() && receiveDataFromServer())
			showData();
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
