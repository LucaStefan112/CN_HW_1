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
#include <sys/wait.h>
#include <sys/socket.h>
#include <stdbool.h>

// FIFO names:
#define serverIn "SERVER_INPUT"
#define serverOut "SERVER_OUTPUT"

// Users list file:
#define usersFile "USERS"

// Maximum number of logged users:
#define maxLoggedUsers 100

// System messages:
#define serverStarted "Server started!\n"
#define clientConnected "New client connected!\n"
#define clientStarted "Client started!\n"
#define connectedToServer "Connected to server!\n"
#define comandExceedsLength "Command too long!\n"

// Error messages:
#define fifoNotOppened "Could not open FIFO's!\n"
#define dataNotSent "Could not send data!\n"
#define dataNotReceived "Could not receive data!\n"
#define bufferSizeMismatch "The buffer value does not match the payload size!\n"
#define bufferOverflow "The request is too large!\n"
#define pipeError "Pipe error!\n"
#define socketpairError "Socketpair error!\n"
#define forkError "Fork error!\n"

// Comands:
#define loginPrefix "login : *"
#define loggedUsers "get-logged-users"
#define processInfoPrefix "get-proc-info : *"
#define logout "logout"
#define quit "quit"

// Responses:
#define badRequest "Bad request!\n"
#define alreadyLogged "The user is already logged!\n"
#define loggedIn "Logged in!\n"
#define userNotFound "User not found!\n"
#define clientAlreadyLogged "You are already logged in!\n"
#define notLoggedIn "You are not logged in!\n"
#define invalidProcess "The process does not exist!\n"
#define loggedOut "Logged out!\n"
#define quitMessage "Good bye!\n"
#define unknownCommand "Command does not exist!\n"
#define commandMustBeAlphanum "Command must be use safe characters(a/A-z/Z, 0-9, '-', '_', ' ', ':')!\n"
#define numberOfLoggedUsersExceeded "The number of logged users is maxed out!\n"

// Maximum buffer size:
#define MAX_BUFFER_SIZE 500

// Safe characters list:
#define safeCharacters "_- :,.+=/?!@#$"

// Default token:
#define defaultToken 0