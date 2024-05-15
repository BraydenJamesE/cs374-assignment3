#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define MAX_CHAR_LENGTH 2049
#define MAX_PATH_LENGTH 1024
#define MAX_COMMAND_SIZE 100
#define COMMENT_NAME "Comment"

pid_t backgroundProcesses[1000];
int numberOfBackgroundProcesses = 0;


char currentWorkingDirectory[MAX_PATH_LENGTH]; // storing a variable that holds the value of the current working directory. This variable is set to the current working directory at the start of the program.


struct Command {
    char* name;
    char** args;
    char* inputFile;
    char* outputFile;
    bool runInBackground;
    char* cdFilePath;
    bool isCd;
    bool isComment;
};


void killBackgroundProcesses() {
    for (int i = 0; i < numberOfBackgroundProcesses; i++) {
        kill(backgroundProcesses[i], SIGTERM); // terminating all background processes.
    }
    for (int i = 0; i < numberOfBackgroundProcesses; i++) {

    }
}


bool checkForComment(char firstLetterOfUserInput) {
    return firstLetterOfUserInput == '#';
} // end of "checkForComment" function


bool checkForCD(char* userInput) { // this function reads the command and checks if it is 'cd'
    return strcmp(userInput, "cd") == 0;
} // end of "checkForCD" function


bool checkForExit(char* userInput) { // this function reads the command and checks if it is 'exit'
    return strcmp(userInput, "exit") == 0;
} // end of "checkForExit" function


bool checkForStatus(char* userInput) { // this function reads the command and checks if it is 'status'
    return strcmp(userInput, "status") == 0;
} // end of "checkForStatus" function


void changeDirectory(char* path) {
    chdir(path);
} // end of "changeDirectory" function


struct Command getUserInput() {
    struct Command cmd = {0};
    char buffer[MAX_CHAR_LENGTH];
    printf(": ");
    fgets(buffer, sizeof(buffer), stdin);
    char* token;
    token = strtok(buffer, " ");
//    printf("token 1: %s \n", token);

    cmd.name = buffer; // setting the name of the command struct.

    if (checkForCD(buffer)) { // checking if the cd command was in the buffer
        token = strtok(NULL, "\n"); // getting the file path.
        cmd.isCd = true;
        cmd.cdFilePath = token;
    }
    else if (checkForExit(buffer)) { // checking if the exit command was in the buffer
        // run stuff for exit
    }
    else if (checkForStatus(buffer)) { // checking if the status command was in the buffer
        // run stuff for status
    }
    else if (checkForComment(buffer[0])) { // checking if the user inputted a comment
        cmd.isComment = true;
        strcpy(cmd.name, COMMENT_NAME); // setting the name of the command struct to "Comment"
    }
    return cmd;
} // end of "getUserInput" function


void handleUserInput(struct Command cmd) {
    if (cmd.isCd) {
        changeDirectory(cmd.cdFilePath);
    }
    else if (checkForExit(cmd.name)) {
        // handle exit
    }
    else if (checkForStatus(cmd.name)) {
        // handle status
    }
    else if (checkForStatus(cmd.name)) {
        // run stuff for status
    }


    printf("%s\n", cmd.name);
} // end of "handleUserInput" function


int main(int argc, char **argv) {
    getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)); // setting the working directory to the initial directory tha the file is stored in.

    // implementing cd functionality

    while (true) {
        struct Command cmd = {0};
        cmd = getUserInput();
        if (!cmd.isComment) { // only handle the command if it's not a comment. If it is a comment, ignore it.
            handleUserInput(cmd);
        }
    }

    return 0;
}