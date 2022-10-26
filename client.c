#include "./all_libs.h"

// File descriptors for FIFO's:
int serverInput, serverOutput;

// Client token:
int clientToken;

// Buffers for reading and writing data:
char terminalInput[MAX_BUFFER_SIZE], serverData[MAX_BUFFER_SIZE];

bool openServerChannels(){

	printf(clientStarted);
	
	// Opening FIFO's:
	serverInput = open(serverIn, O_WRONLY);
	serverOutput = open(serverOut, O_RDONLY);

	// Error checking:
	if(serverInput == -1 || serverOutput == -1){
		printf(fifoNotOppened);
		return false;
	}

	printf(connectedToServer);

	return true;
}

void readDataFromTerminal(){
	printf(">");
	fgets(terminalInput, sizeof(terminalInput), stdin);
	// Erase '\n' character at the end:
	terminalInput[strlen(terminalInput) - 1] = 0;
}

bool isInputValid(){
	// Check empty command:
	if(strlen(terminalInput) == 0)
		return false;

	// Check input length:
	if(strlen(terminalInput) > MAX_BUFFER_SIZE){
		printf(comandExceedsLength);
		
		// Empty stdin buffer:
		int c;
		while ((c = getchar()) != '\n' && c != EOF);

		return false;
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
			return false;
		}

	return true;
}

bool sendDataToServer(){
	// Send authentification token:
	if(write(serverInput, &clientToken, sizeof(int)) == -1){
		printf(dataNotSent);
		return false;
	}

	// Send buffer size:
	int dataSize = strlen(terminalInput);
	if(write(serverInput, &dataSize, sizeof(int)) == -1){
		printf(dataNotSent);
		return false;
	}

	// Send buffer:
	if(write(serverInput, terminalInput, dataSize) == -1){
		printf(dataNotSent);
		return false;
	}
	
	return true;
}

int receiveDataFromServer(){
	strcpy(serverData, "");

	// Get token from server:
	if(read(serverOutput, &clientToken, sizeof(int)) == -1){
		printf(dataNotReceived);
		return false;
	}

	// Get response buffer size:
	int bufferSize = 0;
	if(read(serverOutput, &bufferSize, sizeof(int)) == -1){
		printf(dataNotReceived);
		return false;
	}

	// Get response:
	if(read(serverOutput, serverData, bufferSize) == -1){
		printf(dataNotReceived);
		return false;
	}

	// Use first "bufferSize" characters:
	serverData[bufferSize] = 0;

	// Quit token:
	if(clientToken == -1){
		printf(quitMessage);
		exit(0);
	}

	return true;
}

void showData(){
	printf("%s", serverData);
}

void communicateWithServer(){
	// Infinite loop:
	while(true){
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
