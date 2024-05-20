#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_CHAR_LENGTH 2049
#define MAX_PATH_LENGTH 1024
#define MAX_COMMAND_SIZE 100
#define NOT_ALPHA " \t\n\0"
#define EXIT_NAME "exit"
#define COMMENT_NAME "Comment"

pid_t backgroundProcesses[1000];
int numberOfBackgroundProcesses = 0;
int lastStatus = 0;
char* home; // this variable will hold the home directory.

char currentWorkingDirectory[MAX_PATH_LENGTH]; // storing a variable that holds the value of the current working directory. This variable is set to the current working directory at the start of the program.


struct Command {
    char* name;
    char** args;
    char* inputFile;
    char* outputFile;
    bool runInBackground;
    char* cdFilePath;
    char* commentContents;
    bool isCd;
    bool isComment;
};


void freeStructMemory(struct Command *cmd) { // freeing all the memory associated with the struct.
    free(cmd->name);
} // end of "freeStructMemory" function


void killBackgroundProcesses() {
    printf("Number of background processes: %d \n", numberOfBackgroundProcesses);
    for (int i = 0; i < numberOfBackgroundProcesses; i++) {
        kill(backgroundProcesses[i], SIGTERM); // terminating all background processes.
    }
    for (int i = 0; i < numberOfBackgroundProcesses; i++) { // waiting for all proccesses to close.
        waitpid(backgroundProcesses[i], NULL, 0);
        printf("Background process with PID %d has exited\n", backgroundProcesses[i]);
    }
    numberOfBackgroundProcesses = 0;
} // end of "killBackgroundProcesses" function


bool checkForComment(char firstLetterOfUserInput) {
    return firstLetterOfUserInput == '#';
} // end of "checkForComment" function


bool checkForCD(const char* userInput) { // this function reads the command and checks if it is 'cd'
    if (userInput[0] == 'c' && userInput[1] == 'd') {
        return true;
    }
    else {
        return false;
    }
} // end of "checkForCD" function


bool checkForExit(char* userInput) { // this function reads the command and checks if it is 'exit'
    char* token = strtok(userInput, NOT_ALPHA);
    return strcmp(token, "exit") == 0;
} // end of "checkForExit" function


bool checkForStatus(char* userInput) { // this function reads the command and checks if it is 'status'
    return strcmp(userInput, "status") == 0;
} // end of "checkForStatus" function


void changeDirectory(char* path) {
    if (chdir(path) != 0) {
        fprintf(stderr, "%s\n", strerror(errno));
    }
    else {
        strcpy(currentWorkingDirectory, path);
    }
} // end of "changeDirectory" function


struct Command getUserInput() {
    struct Command cmd = {0};
    char buffer[MAX_CHAR_LENGTH];
    char* token;
    printf(": ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // scan buffer until it finds the newline char and replace it with null terminator.wh
    cmd.name = malloc(sizeof(char) * 15); // allocating 15 characters to the name.

    if (checkForCD(buffer)) { // checking if the cd command was in the buffer
        strcpy(cmd.name, "cd");
        token = strtok(buffer + 3, NOT_ALPHA); // getting the file path.
        cmd.isCd = true;
        cmd.cdFilePath = token;
    }
    else if (checkForComment(buffer[0])) { // checking if the user inputted a comment
        cmd.isComment = true;
        strcpy(cmd.name, COMMENT_NAME); // setting the name of the command struct to "Comment"
    }
    else if (checkForExit(buffer)) {
        strcpy(cmd.name, EXIT_NAME);
    }
    return cmd;
} // end of "getUserInput" function

void handleUserInput(struct Command cmd) {
    if (cmd.isCd) {
        if (cmd.cdFilePath == NULL) { // handling the case where the user didn't pass a filepath with cd command.
            changeDirectory(home);
        }
        else {
            changeDirectory(cmd.cdFilePath);
        }
    }
    else if (strcmp(cmd.name, EXIT_NAME) == 0) {
        if (numberOfBackgroundProcesses > 0) {
            killBackgroundProcesses();
        }
        exit(EXIT_SUCCESS); // exiting the program
    }
    else if (checkForStatus(cmd.name)) {
    }
    else if(!cmd.isComment) { // handle all other scenarios that are not comments.
        printf("Non Standard arg\n");
        printf("cmd.name: %s\n", cmd.name);
        printf("cmd.isCd: %d\n", cmd.isCd);
    }
} // end of "handleUserInput" function


int main() {
    home = getenv("HOME");
    getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)); // setting the working directory to the initial directory tha the file is stored in.

    while (true) {
        struct Command cmd = getUserInput();
        if (!cmd.isComment) { // only handle the command if it's not a comment. If it is a comment, ignore it.
            handleUserInput(cmd);
        }
        freeStructMemory(&cmd);
    } // end of while loop
}