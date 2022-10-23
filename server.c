#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <utmp.h>
#include <regex.h>

// FIFO names:
#define serverIn "SERVER_INPUT"
#define serverOut "SERVER_OUTPUT"

// Users list file:
#define usersFile "USERS"

// Maximum number of logged users:
#define maxLoggedUsers 100

// System messages:
#define started "Server started!\n"
#define connected "New client connected!\n"

// Error messages:
#define fifoNotOppened "Could not open FIFO's!"
#define dataNotSent "Could not send data to client!"
#define dataNotReceived "Could not receive data from client!"
#define bufferSizeMismatch "The buffer size does not match the payload value!"
#define bufferOverflow "The request is too large!"

// Comands:
#define loginPrefix "login : *"
#define loggedUsers "get-logged-users"
#define processInfoPrefix "get-proc-info : *"
#define logout "logout"
#define quit "quit"

// Responses:
#define badRequest "Bad request!\n"
#define alreadyLogged "The user is already logged!\n"
#define loggedIn "Loggen in!\n"
#define userNotFound "User not found!\n"
#define clientAlreadyLogged "You are already logged in!\n"
#define notLoggedIn "You are not logged in!\n"
#define invalidProcess "The process does not exist!\n"
#define loggedOut "Logged out!\n"
#define quitMessage "Goof Bye!\n"
#define unknownCommand "Command does not exist!\n"

// Maximum buffer size:
#define MAX_BUFFER_SIZE 50

// Safe characters list:
#define safeCharacters "_- :,.+=/?!@#$"

// Default token:
#define defaultToken 0

// File descriptors for FIFO's:
int serverInput, serverOutput;

// Buffer size:
int bufferSize;

// Client token:
int clientToken;

// Buffers for reading and writing data:
char request[100], response[100];

// List of all logged users;
char loggedUsersList[maxLoggedUsers][100];

// List of logged users tokens:
int loggedUsersTokens[maxLoggedUsers], numberOfLoggedUsers;

void createFifos(){
	mkfifo(serverIn, 0666);
	mkfifo(serverOut, 0666);
}

int openServerChannels(){
	printf(started);

	// Opening FIFO's:
	serverInput = open(serverIn, O_RDONLY);
	serverOutput = open(serverOut, O_WRONLY);

	// Error checking:
	if(serverInput == -1 || serverOutput == -1){
		printf(fifoNotOppened);
		return 0;
	}

	printf(connected);

	return 1;
}

int readDataFromClient(){
	// Get client's authentification token:
	if(read(serverInput, &clientToken, sizeof(int)) == -1){
		printf(dataNotReceived);
		return 0;
	}

	// Get request buffer size:
	if(read(serverInput, &bufferSize, sizeof(int)) == -1){
		printf(dataNotReceived);
		return 0;
	}

	// Get request data:
	if(read(serverInput, request, bufferSize) == -1){
		printf(dataNotReceived);
		return 0;
	}
	printf("Token: %d; Size: %d; Data: %s;", clientToken, bufferSize, request);

	return 1;
}

void sendDataToClient(){
	int bufferSize = strlen(response);
	if(
		write(serverOutput, &clientToken, sizeof(int)) == -1 ||
		write(serverOutput, &bufferSize, sizeof(int)) == -1 ||
		write(serverOutput, response, bufferSize) == -1)
			printf(dataNotSent);
}

int isRequestValid(){
	// Generate prefix regex expressions for "login" and "process informations":
	regex_t loginPrefixRegex, processInfoPrefixRegex;
    regcomp(&loginPrefixRegex, loginPrefix, 0);
	regcomp(&processInfoPrefixRegex, processInfoPrefix, 0);

	// Check data length:
	if(bufferSize != strlen(request)){
		strcpy(response, badRequest);
		sendDataToClient();
		return 0;
	}

	// Check buffer size:
	else if(bufferSize > MAX_BUFFER_SIZE){
		strcpy(response, badRequest);
		sendDataToClient();
		return 0;
	}

	// Check if input is alfa-num or safe character:
	for(int i = 0; request[i]; i++)
		if(!(
			'a' <= request[i] && request[i] <= 'z' ||
			'A' <= request[i] && request[i] <= 'Z' ||
			'0' <= request[i] && request[i] <= '9' ||
			strchr(safeCharacters, request[i])
		)){
			strcpy(response, badRequest);
			sendDataToClient();
			return 0;
		}

	// Check is token is active:
	else if(clientToken){
		for(int i = 0; i < numberOfLoggedUsers; i++)
			if(clientToken == loggedUsersTokens[i])
				return 1;
		
		strcpy(response, badRequest);
		sendDataToClient();
		return 0;
	}
	return 1;
}

int respondToCLient(){
	// Generate prefix regex expressions for "login" and "process informations":
	regex_t loginPrefixRegex, processInfoPrefixRegex;
    regcomp(&loginPrefixRegex, loginPrefix, 0);
	regcomp(&processInfoPrefixRegex, processInfoPrefix, 0);

	// Command "login : username":
	if(!regexec(&loginPrefixRegex, request, 0, NULL, 0)){
		// Getting username from request:
		char user[100];
		strcpy(user, request + strlen(loginPrefix) - 1);
		
		// Client is not logged in:
		if(clientToken == 0){
			for(int i = 0; i < numberOfLoggedUsers; i++)
				// Account is already in use at the moment:
				if(!strcmp(user, loggedUsersList[i])){
					strcpy(response, alreadyLogged);
					sendDataToClient();
					return 0;
				}

			// Check if user exists:
			FILE* usersFilePointer = fopen(usersFile, "r");
			char username[100];
			while(fgets(username, sizeof(username), usersFilePointer)){
				// Remove '\n' character:
				username[strlen(username) - 1] = 0;

				// If username exists:
				if(!strcmp(user, username)){
					// Create new token for client:
					int tokenIsNew;
					do{
						tokenIsNew = 1;
						clientToken = rand() % maxLoggedUsers;

						for(int i = 0; i < numberOfLoggedUsers && tokenIsNew; i++)
							if(clientToken == loggedUsersTokens[i])
								tokenIsNew = 0;
					}while(!tokenIsNew);

					// Add user to logged users lists:
					strcpy(loggedUsersList[numberOfLoggedUsers], user);
					loggedUsersTokens[numberOfLoggedUsers] = clientToken;
					numberOfLoggedUsers++;

					strcpy(response, loggedIn);
					sendDataToClient();
					return 0;
				}
			}
			
			strcpy(response, userNotFound);
			sendDataToClient();
			return 0;
		}
		// Client logged in another account:
		strcpy(response, clientAlreadyLogged);
		sendDataToClient();
		return 0;
	}

	// Command "get-logged-users":
	else if(!strcmp(request, loggedUsers)){
		//Check if client is authentificated:
		if(!clientToken){
			strcpy(response, notLoggedIn);
			sendDataToClient();
			return 0;
		}
		
		// Create utmp pointer:
		strcpy(response, "\0");
		struct utmp* utmpPointer = getutent();
		
		// Concatenate data:
		while(utmpPointer){
			// Transforming seconds from int32_t to char*:
			int32_t seconds = utmpPointer->ut_tv.tv_sec, stringLength = 0;
			char secondsString[50];
			do{
				secondsString[stringLength++] = '0' + seconds % 10;
				seconds /= 10;
			}while(seconds);
			secondsString[stringLength] = 0;

			strcat(response, utmpPointer->ut_user);
			strcat(response, " \0");
			strcat(response, utmpPointer->ut_host);
			strcat(response, " \0");
			strcat(response, secondsString);
			strcat(response, "\n\0");

			utmpPointer = getutent();
		}

		sendDataToClient();
		return 0;
	}

	// Command "get-proc-info : pid":
	else if(!regexec(&processInfoPrefixRegex, request, 0, NULL, 0)){
		//Check if client is authentificated:
		if(!clientToken){
			strcpy(response, notLoggedIn);
			sendDataToClient();
			return 0;
		}

		char pid[20], processFilePath[50] = "\0";
		strcpy(pid, request + strlen(processInfoPrefix) - 1);

		strcat(processFilePath, "/proc/");
		strcat(processFilePath, pid);
		strcat(processFilePath, "/status");

		FILE* filePointer = fopen(processFilePath, "r");

		if(!filePointer){
			strcpy(response, invalidProcess);
			sendDataToClient();
			return 0;
		}

    	char field[100];
		strcpy(response, "\0");

		while(fgets(field, sizeof(field), filePointer))
			if(
				strstr(field, "Name:") ||
				strstr(field, "State:") ||
				strstr(field, "Ppid:") ||
				strstr(field, "Uid:") ||
				strstr(field, "VmSize:")
			)
				strcat(response, field);

		sendDataToClient();
		return 0;
	}

	// Command "logout":
	else if(!strcmp(request, logout)){
		//Check if client is authentificated:
		if(!clientToken){
			strcpy(response, notLoggedIn);
			sendDataToClient();
			return 0;
		}

		// Removing user from logged users:
		for(int i = 0; i < numberOfLoggedUsers; i++)
			if(clientToken == loggedUsersTokens[i]){
				loggedUsersTokens[i] = loggedUsersTokens[numberOfLoggedUsers - 1];
				strcpy(loggedUsersList[i], loggedUsersList[numberOfLoggedUsers - 1]);
				numberOfLoggedUsers--;
				break;
			}

		clientToken = 0;
		strcpy(response, loggedOut);
		sendDataToClient();
		return 0;
	}

	// Command "quit":
	else if(!strcmp(request, quit)){
		// If client is logged in:
		if(clientToken)
			// Removing user from logged users:
			for(int i = 0; i < numberOfLoggedUsers; i++)
				if(clientToken == loggedUsersTokens[i]){
					loggedUsersTokens[i] = loggedUsersTokens[numberOfLoggedUsers - 1];
					strcpy(loggedUsersList[i], loggedUsersList[numberOfLoggedUsers - 1]);
					numberOfLoggedUsers--;
					break;
				}
		
		clientToken = 0;
		strcpy(response, quitMessage);
		sendDataToClient();

		// Closing app signal:
		return 0;
	}

	// Unknown command:
	else{
		strcpy(response, unknownCommand);
		sendDataToClient();
		return 0;
	}
}

void communicateWithClient(){
	//Infinite loop:
	while(1)
		// Execute stages sequentially:
		if(!readDataFromClient() || !isRequestValid() || !respondToCLient());
}

int main(){
	srand(time(NULL));

	createFifos();

	if(!openServerChannels())
		return 1;

	communicateWithClient();

	return 0;
}
