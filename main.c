#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    for (int i = 0; i < numberOfBackgroundProcesses; i++) { // waiting for all proccesses to close.
        waitpid(backgroundProcesses[i], NULL, 0);
        printf("Background process with PID %d has exited\n", backgroundProcesses[i]);
    }
    numberOfBackgroundProcesses -= 1; // decrementing the number of open processes.
} // end of "killBackgroundProcesses" function


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
    char* token;

    printf(": ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // scan buffer until it finds the newline char and replace it with null terminator.
    cmd.name = buffer; // setting the name of the command struct.

    if (checkForCD(buffer)) { // checking if the cd command was in the buffer
        token = strtok(NULL, "\n"); // getting the file path.
        cmd.isCd = true;
        cmd.cdFilePath = token;
    }
    else if (checkForComment(buffer[0])) { // checking if the user inputted a comment
        cmd.isComment = true;
        strcpy(cmd.name, COMMENT_NAME); // setting the name of the command struct to "Comment"
    }
    return cmd;
} // end of "getUserInput" function


void handleUserInput(struct Command cmd) {
    if (cmd.isCd) {
        pid_t pid = fork();
        if (pid == 0) {
            changeDirectory(cmd.cdFilePath);
            exit(0);
        }
        else {
            backgroundProcesses[numberOfBackgroundProcesses] = fork();
            numberOfBackgroundProcesses++;
            changeDirectory(cmd.cdFilePath);
        }
    }
    else if (checkForExit(cmd.name)) {
        printf("Exit called");
        killBackgroundProcesses();
        exit(0);
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
        printf("|%s|\n", cmd.name);
        printf("Exit hit? %d\n", checkForExit(cmd.name));
        if (!cmd.isComment) { // only handle the command if it's not a comment. If it is a comment, ignore it.
            handleUserInput(cmd);
        }
    } // end of while loop
}