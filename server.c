#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <utmp.h>

// FIFO names:
#define serverIn "SERVER_INPUT"
#define serverOut "SERVER_OUTPUT"

// Users list file:
#define usersFile "USERS"

// Maximum number of logged users:
#define maxLoggedUsers 100

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

// Responses:
#define alreadyLogged "The user is already logged!\n"
#define loggedIn "Loggen in!\n"
#define userNotFound "User not found!\n"
#define clientAlreadyLogged "You are already logged in!\n"
#define notLoggedIn "You are not logged in!\n"

// Maximum buffer size:
#define MAX_BUFFER_SIZE 50

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
	return 1;
}

int sendDataToClient(){
	int bufferSize = strlen(response);

	if(
		write(serverOutput, &clientToken, sizeof(int)) == -1 ||
		write(serverOutput, &bufferSize, sizeof(int)) == -1 ||
		write(serverOutput, response, bufferSize) == -1){
			printf(dataNotSent);
			return 0;
	}
	return 1;
}

int isRequestValid(){
	// Check data length:
	if(bufferSize != strlen(request)){
		printf(bufferSizeMismatch);
		return 0;
	}
	// Check buffer size:
	else if(bufferSize > MAX_BUFFER_SIZE){
		printf(bufferOverflow);
		return 0;
	}
	// Request validation:
	else if(
		strstr(request, loginPrefix) != 0 &&
		strcmp(request, loggedUsers) &&
		strstr(request, procInfoPrefix) != 0 &&
		strcmp(request, logout) &&
		strcmp(request, quit)){
			printf(invalidRequest);
			return 0;
		}
	// Check is token is active:
	else if(clientToken){
		for(int i = 0; i < numberOfLoggedUsers; i++)
			if(clientToken == loggedUsersTokens[i])
				return 1;
		printf(invalidRequest);
		return 0;
	}
	return 1;
}

int respondToCLient(){
	// Command "login : username":
	if(strstr(request, loginPrefix) == 0){
		// Getting username from request:
		char user[100];
		strcpy(user, request + strlen(loginPrefix));
		
		// Client is not logged in:
		if(clientToken == 0){
			for(int i = 0; i < numberOfLoggedUsers; i++)
				// Account is already in use at the moment:
				if(!strcmp(user, loggedUsersList[i])){
					strcpy(response, alreadyLogged);
					return sendDataToClient();
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
					return sendDataToClient();
				}
			}
			
			strcpy(response, userNotFound);
			return sendDataToClient();
		}
		// Client logged in another account:
		strcpy(response, clientAlreadyLogged);
		return sendDataToClient();
	}

	// Command "get-logged-users":
	else if(!strcmp(request, loggedUsers)){
		//Check if client is authentificated:
		if(!clientToken){
			strcpy(response, notLoggedIn);
			return sendDataToClient();
		}
		
		// Create utmp pointer:
		strcpy(response, "\0");
		struct utmp* utmpPointer = getutent();
		
		// Concatenate data:
		while(utmpPointer){
			strcat(response, utmpPointer->ut_user);
			strcat(response, " \0");
			strcat(response, utmpPointer->ut_host);
			strcat(response, " \0");
			strcat(response, utmpPointer->ut_tv.tv_sec);
			strcat(response, "\n\0");

			utmpPointer = getutent();
		}

		return respondToCLient();
	}

	else if(1){
		
	}
}

void communicateWithClient(){
	//Infinite loop:
	while(1){
		// If there is any error:
		if(!readDataFromClient() || !isRequestValid() || !respondToCLient())
			break;
	}
}

int main(){
	srand(time(NULL));

	createFifos();

	if(!openServerChannels())
		return 1;

	communicateWithClient();

	return 0;
}
