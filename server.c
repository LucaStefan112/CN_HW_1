#include "./all_libs.h"

// File descriptors for FIFO's:
int serverInput, serverOutput;

// Client token:
int clientToken;

// Buffers for reading and writing data:
char request[MAX_BUFFER_SIZE], response[MAX_BUFFER_SIZE];

// List of all logged users;
char loggedUsersList[maxLoggedUsers][100];

// Quitting signal:
bool quitting;

// List of logged users tokens:
int loggedUsersTokens[maxLoggedUsers], numberOfLoggedUsers;

void createFifos(){
	mkfifo(serverIn, 0666);
	mkfifo(serverOut, 0666);
}

bool openServerChannels(){
	printf(serverStarted);

	// Opening FIFO's:
	serverInput = open(serverIn, O_RDONLY);
	serverOutput = open(serverOut, O_WRONLY);

	// Error checking:
	if(serverInput == -1 || serverOutput == -1){
		printf(fifoNotOppened);
		return false;
	}

	printf(clientConnected);

	return true;
}

bool readDataFromClient(){
	int bufferSize = 0;
	strcpy(request, "");

	// Get client's authentification token:
	if(read(serverInput, &clientToken, sizeof(int)) == -1){
		printf(dataNotReceived);
		return false;
	}

	// Get request buffer size:
	if(read(serverInput, &bufferSize, sizeof(int)) == -1){
		printf(dataNotReceived);
		return false;
	}

	// Get request data:
	if(read(serverInput, request, bufferSize) == -1){
		printf(dataNotReceived);
		return false;
	}

	// Use first "bufferSize" characters:
	request[bufferSize] = 0;

	return true;
}

bool sendDataToClient(){
	int bufferSize = strlen(response);
	response[bufferSize] = 0;
	if(
		write(serverOutput, &clientToken, sizeof(int)) == -1 ||
		write(serverOutput, &bufferSize, sizeof(int)) == -1 ||
		write(serverOutput, response, bufferSize) == -1){
			printf(dataNotSent);
			return false;
		}
	printf("%d %d %s\n", clientToken, bufferSize, response);
	return true;
}

bool isRequestValid(){
	// Generate prefix regex expressions for "login" and "process informations":
	regex_t loginPrefixRegex, processInfoPrefixRegex;
    regcomp(&loginPrefixRegex, loginPrefix, 0);
	regcomp(&processInfoPrefixRegex, processInfoPrefix, 0);

	// Check buffer size:
	if(strlen(response) > MAX_BUFFER_SIZE){
		strcpy(response, badRequest);
		sendDataToClient();
		return false;
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
			return false;
		}
	
	// Check maximum number of logged users:
	if(numberOfLoggedUsers == maxLoggedUsers){
		strcpy(response, numberOfLoggedUsersExceeded);
		return false;
	}

	// Check is token is active:
	else if(clientToken){
		for(int i = 0; i < numberOfLoggedUsers; i++)
			if(clientToken == loggedUsersTokens[i])
				return true;
		
		sendDataToClient();
		return false;
	}
	return true;
}

bool child_parentCommunication(int pid, int channel, bool loginCommand){
	// Parent code:
	if(pid){
		int bufferSize = 0;
		// Reading buffer size:
		if(read(channel, &bufferSize, sizeof(bufferSize)) == -1){
			printf(dataNotReceived);
			return false;
		}
		// Reading buffer:
		if(read(channel, response, bufferSize) == -1){
			printf(dataNotReceived);
			return false;
		}
		response[bufferSize] = 0;

		// Login custom communication:
		if(loginCommand){
			// Reading client token:
			if(read(channel, &clientToken, sizeof(clientToken)) == -1){
				printf(dataNotReceived);
				return false;
			}
			// New logged user:
			if(clientToken){
				// Saving the name of the logged user:
				if(read(channel, &bufferSize, sizeof(bufferSize)) == -1){
					printf(dataNotReceived);
					return false;
				}
				if(read(channel, loggedUsersList[numberOfLoggedUsers], sizeof(loggedUsersList[numberOfLoggedUsers])) == -1){
					printf(dataNotReceived);
					return false;
				}

				loggedUsersTokens[numberOfLoggedUsers++] = clientToken;
			}
		}
	}
	// Child code:
	else{
		int bufferSize = strlen(response);
		if(write(channel, &bufferSize, sizeof(bufferSize)) == -1 || write(channel, response, bufferSize) == -1){
			printf(dataNotSent);
			return false;
		}
		response[bufferSize] = 0;

		// Login custom communication:
		if(loginCommand){
			// Writing client token:
			if(write(channel, &clientToken, sizeof(clientToken)) == -1){
				printf(dataNotSent);
				return false;
			}
			// New user logged:
			if(clientToken){
				// Sending the name of the new user:
				bufferSize = strlen(loggedUsersList[numberOfLoggedUsers - 1]);
				if(write(channel, &bufferSize, sizeof(bufferSize)) == -1){
					printf(dataNotSent);
					return false;
				}
				if(write(channel, loggedUsersList[numberOfLoggedUsers - 1], sizeof(loggedUsersList[numberOfLoggedUsers - 1])) == -1){
					printf(dataNotSent);
					return false;
				}
			}
		}
	}
	return true;
}

bool respondToCLient(){
	// Generate prefix regex expressions for "login" and "process informations":
	regex_t loginPrefixRegex, processInfoPrefixRegex;
    regcomp(&loginPrefixRegex, loginPrefix, 0);
	regcomp(&processInfoPrefixRegex, processInfoPrefix, 0);

	// Command "login : username":
	if(!regexec(&loginPrefixRegex, request, 0, NULL, 0)){
		// Check if client is already logged in:
		if(clientToken){
			strcpy(response, clientAlreadyLogged);
			sendDataToClient();
			return true;
		}
		
		int thisPipe[2], pid;
		
		// Check for pipe error:
		if(pipe(thisPipe) == -1){
			printf(pipeError);
			return false;
		}
		// Check for fork error:
		if((pid = fork()) == -1){
			printf(forkError);
			return false;
		}
		
		// Parent code:
		if(pid){
			close(thisPipe[1]);

			bool communicationStatus;
			if((communicationStatus = child_parentCommunication(pid, thisPipe[0], true)))
				sendDataToClient();

			close(thisPipe[0]);
			return communicationStatus;
		}

		// Child code:
		close(thisPipe[0]);

		// Getting username from request:
		char requestedUser[MAX_BUFFER_SIZE];
		strcpy(requestedUser, request + strlen(loginPrefix) - 1);
		
		// Check if account is already in use at the moment:
		for(int i = 0; i < numberOfLoggedUsers; i++)
			if(!strcmp(requestedUser, loggedUsersList[i])){
				strcpy(response, alreadyLogged);
				child_parentCommunication(pid, thisPipe[1], true);
				close(thisPipe[1]);
				exit(0);
			}

		// Check if user exists:
		FILE* usersFilePointer = fopen(usersFile, "r");
		char username[MAX_BUFFER_SIZE];

		while(fgets(username, sizeof(username), usersFilePointer)){

			// Remove '\n' character:
			username[strlen(username) - 1] = 0;

			// If username exists:
			if(!strcmp(requestedUser, username)){

				// Create new token for client:
				for(int i = 1; i <= maxLoggedUsers; i++){
					bool tokenUsed = false;
					for(int j = 0; j < numberOfLoggedUsers && !tokenUsed; j++)
						if(loggedUsersTokens[j] == i)
							tokenUsed = true;
					
					if(!tokenUsed){
						clientToken = i;
						break;
					}
				}

				// Add user to logged users lists:
				strcpy(loggedUsersList[numberOfLoggedUsers], username);
				numberOfLoggedUsers++;

				strcpy(response, loggedIn);
				child_parentCommunication(pid, thisPipe[1], true);
				close(thisPipe[1]);
				exit(0);
			}
		}
		// User not found:
		strcpy(response, userNotFound);
		child_parentCommunication(pid, thisPipe[1], true);
		close(thisPipe[1]);
		exit(0);
	}

	// Command "get-logged-users":
	else if(!strcmp(request, loggedUsers)){
		int thisPipe[2], pid;
		
		if(pipe(thisPipe) == -1){
			printf(pipeError);
			return 1;
		}

		if((pid = fork()) == -1){
			printf(forkError);
			return 1;
		}
		
		// Parent code:
		if(pid){
			close(thisPipe[1]);
			int thisBufferSize;
			
			read(thisPipe[0], &clientToken, sizeof(int));
			read(thisPipe[0], &thisBufferSize, sizeof(int));
			read(thisPipe[0], response, thisBufferSize);
			response[thisBufferSize] = 0;
			wait(NULL);

			sendDataToClient();
			close(thisPipe[0]);
			return 0;
		}

		// Child code:
		close(thisPipe[0]);
		
		//Check if client is authentificated:
		if(!clientToken){
			strcpy(response, notLoggedIn);
			int thisBufferSize = strlen(response);
			write(thisPipe[1], &clientToken, sizeof(int));
			write(thisPipe[1], &thisBufferSize, sizeof(int));
			write(thisPipe[1], response, strlen(response));
			quitting = 1;
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

			if(strlen(utmpPointer->ut_user) && strlen(utmpPointer->ut_host) && strlen(secondsString)){
			 	strcat(response, "User: \0");
			 	strcat(response, utmpPointer->ut_user);
			 	strcat(response, " Host: \0");
			 	strcat(response, utmpPointer->ut_host);
			 	strcat(response, " Seconds active: \0");
			 	strcat(response, secondsString);
			 	strcat(response, "\n\0");
			}
			utmpPointer = getutent();
		}

		int thisBufferSize = strlen(response);
		write(thisPipe[1], &clientToken, sizeof(int));
		write(thisPipe[1], &thisBufferSize, sizeof(int));
		write(thisPipe[1], response, strlen(response));
		quitting = 1;
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

		int thisSocketpair[2], pid;
		
		if(socketpair(AF_UNIX, SOCK_STREAM, 0, thisSocketpair) == -1){
			printf(socketpairError);
			return 1;
		}

		if((pid = fork()) == -1){
			printf(forkError);
			return 1;
		}
		
		// Parent code:
		if(pid){
			close(thisSocketpair[1]);
			int thisBufferSize;
			
			read(thisSocketpair[0], &thisBufferSize, sizeof(int));
			read(thisSocketpair[0], response, thisBufferSize);

			for(int i = 0; i < numberOfLoggedUsers; i++)
				read(thisSocketpair[0], &loggedUsersTokens[i], sizeof(int));

			response[thisBufferSize] = 0;
			wait(NULL);

			sendDataToClient();
			close(thisSocketpair[0]);
			return 0;
		}

		// Child code:
		close(thisSocketpair[0]);

		char pidProcess[20], processFilePath[50] = "\0";
		strcpy(pidProcess, request + strlen(processInfoPrefix) - 1);

		// Creating path to process file:
		strcat(processFilePath, "/proc/");
		strcat(processFilePath, pidProcess);
		strcat(processFilePath, "/status");

		FILE* filePointer = fopen(processFilePath, "r");

		if(!filePointer){
			strcpy(response, invalidProcess);
			int thisBufferSize = strlen(response);
			write(thisSocketpair[1], &thisBufferSize, sizeof(int));
			write(thisSocketpair[1], response, strlen(response));
			quitting = 1;
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

		int thisBufferSize = strlen(response);
		write(thisSocketpair[1], &thisBufferSize, sizeof(int));
		write(thisSocketpair[1], response, strlen(response));
		quitting = 1;
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
		clientToken = -1;
		strcpy(response, quitMessage);
		sendDataToClient();

		// Closing app signal:
		quitting = 1;
		return 0;
	}

	// Unknown command:
	else{
		strcpy(response, unknownCommand);
		sendDataToClient();
		return 0;
	}
}

void closeServerChannels(){
	// Close FIFO's:
	close(serverInput);
	close(serverOutput);
}

void communicateWithClient(){
	//Infinite loop:
	while(!quitting)
		// Execute stages sequentially:
		if(!readDataFromClient() || !isRequestValid() || !respondToCLient());
}

int main(){
	srand(time(NULL));

	createFifos();

	if(!openServerChannels())
		return 1;

	communicateWithClient();

	closeServerChannels();

	return 0;
}
