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
int lastExitStatus = 0;
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
    char* commandFilePath;
};

void freeStructMemory(struct Command *cmd) { // freeing all the memory associated with the struct.
    free(cmd->name);
    if (cmd->commandFilePath != NULL) { // if it is null, then memory was never allocated to it.
        free(cmd->commandFilePath);
    }
} // end of "freeStructMemory" function


char* getCommandFilePath(char* command) {
    char* path = getenv("PATH");
    if (path == NULL) { // handling an error case before using path
        fprintf(stderr, "Path variable not available\n");
        return NULL;
    }
    char* delim = ":"; // separating each segment of the path by its delimiter.
    char* token = strtok(path, delim); // getting the first output of path
    char* fullPath = malloc(sizeof (char) * 200); // allocating memory to the file path

    while (token != NULL) {
        snprintf(fullPath, 200, "%s/%s", token, command);

        if (access(fullPath, X_OK) != -1) {
            return fullPath; // allocating memory to be returned.
        }
        token = strtok(NULL, delim);
    }

    fprintf(stderr, "Error: Command '%s' not found\n", command); // if file path is not available, output error message
    free(fullPath); // free allocated memory
    exit(EXIT_FAILURE); // send exit status of 1
} // end of "getCommandFilePath" function


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
    else {
        token = strtok(buffer, NOT_ALPHA);
        strcpy(cmd.name, token);
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
        pid_t pid = fork();
        if (pid == -1) { // checking if the fork failed before using
            perror("fork");
            return;
        }
        else if (pid == 0) { // child proccess
            char* filePathToCommand = getCommandFilePath(cmd.name);
            cmd.commandFilePath = filePathToCommand;
            printf("filePathToCommand: %s\n", filePathToCommand);
            cmd.args = malloc(sizeof(char*) * (3));
            cmd.args[0] = malloc(sizeof(char) * strlen(cmd.commandFilePath) + 1);
            strcpy(cmd.args[0], cmd.commandFilePath);
            if (execv(cmd.commandFilePath, cmd.args) == -1) {
                if (errno == ENOENT) { // checking if errno equals "no such directory entry"
                    printf("Error: Command not found: %s\n", cmd.name);
                    exit(EXIT_FAILURE);
                }
                printf("Error in execv\n");
                perror("execv\n"); // printing the error.
            }
        }
        else { // parent process
            int status;
            if (waitpid(pid, &status, 0) == -1) { // outputting the error of waitpid before using it
                perror("waitpid");
            } // wait for the child process to finish
            lastExitStatus = WEXITSTATUS(status); // setting the last exit status to the child exit status.
        }
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