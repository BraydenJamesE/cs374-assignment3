#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_CHAR_LENGTH 2049
#define MAX_PATH_LENGTH 1024
#define MAX_NUM_ARGS 512
#define MAX_COMMAND_SIZE 100
#define NOT_ALPHA " \t\n\0"
#define EXIT_NAME "exit"
#define COMMENT_NAME "Comment"

pid_t backgroundProcesses[1000];
pid_t foregroundProcess = -1; // creating a forground process variable
int numberOfBackgroundProcesses = 0;
int lastExitStatus = 0;
int lastSignalStatus = 0;
char* home; // this variable will hold the home directory.

char* currentWorkingDirectory; // storing a variable that holds the value of the current working directory. This variable is set to the current working directory at the start of the program.

bool lastStatusWasSignal = false;

struct Command {
    char* name; // this holds the command
    char** args;
    char* inputFile;
    char* outputFile;
    char* cdFilePath;
    char* commandFilePath;
    int numberOfArgs;
    bool runInBackground;
    bool isCd;
    bool isComment;
};

void freeStructMemory(struct Command cmd) { // freeing all the memory associated with the struct.
    if (cmd.name != NULL) {
        free(cmd.name);
    }
    if (cmd.args != NULL) {
        for (int i = 0; cmd.args[i] != NULL; i++) {
            free(cmd.args[i]);
        }
        free(cmd.args);
    }
    if (cmd.inputFile != NULL) {
        free(cmd.inputFile);
    }
    if (cmd.outputFile != NULL) {
        free(cmd.outputFile);
    }
    if (cmd.cdFilePath != NULL) {
        free(cmd.cdFilePath);
    }
    if (cmd.commandFilePath != NULL) { // if it is null, then memory was never allocated to it.
        free(cmd.commandFilePath);
    }
} // end of "freeStructMemory" function



char* getCommandFilePath(char* command) {
    char* path = getenv("PATH");
    if (path == NULL) { // handling an error case before using path
        fprintf(stderr, "Path variable not available\n");
        fflush(stdout);
        return NULL;
    }
    char* delim = ":"; // separating each segment of the path by its delimiter.
    char* token = strtok(path, delim); // getting the first output of path
    char* fullPath = malloc(sizeof (char) * 200); // allocating memory to the file path

    while (token != NULL) {
        snprintf(fullPath, 200, "%s/%s", token, command);
        fflush(stdout);

        if (access(fullPath, X_OK) != -1) {
            return fullPath; // allocating memory to be returned.
        }
        token = strtok(NULL, delim);
    }

    fprintf(stderr, "Error: Command '%s' not found\n", command); // if file path is not available, output error message
    fflush(stdout);
    free(fullPath); // free allocated memory
    exit(EXIT_FAILURE); // send exit status of 1
} // end of "getCommandFilePath" function


void killBackgroundProcesses() {
    printf("Number of background processes: %d \n", numberOfBackgroundProcesses);
    fflush(stdout);
    for (int i = 0; i < numberOfBackgroundProcesses; i++) {
        kill(backgroundProcesses[i], SIGTERM); // terminating all background processes.
    }
    for (int i = 0; i < numberOfBackgroundProcesses; i++) { // waiting for all proccesses to close.
        waitpid(backgroundProcesses[i], NULL, 0);
        printf("Background process with PID %d has exited\n", backgroundProcesses[i]);
        fflush(stdout);
    }
    numberOfBackgroundProcesses = 0;
} // end of "killBackgroundProcesses" function


void killForegroundProcess(int signum) { // function for killing the foreground process
    if (foregroundProcess != -1) { // ensuring that the foreground process is running
        kill(foregroundProcess, SIGINT); // killing the foreground process with sigint
    }
    printf("terminated by signal %d\n", signum); // outputting to the user.
    fflush(stdout);
    lastStatusWasSignal = true;
    lastSignalStatus = signum;
    foregroundProcess = -1;
} // end of "killForegroundProcess" function


void checkOnBackgroundProcesses() {
    int numOfBackgroundProcessesTerminated = 0;
    for (int i = 0; i < numberOfBackgroundProcesses; i++) {
        int status;
        pid_t result = waitpid(backgroundProcesses[i], &status, WNOHANG); // getting the result of the child process
        if (result == 0) { // the process is still running and not complete
            continue;
        }
        else if (result == -1) { // there was an error.
            perror("waitpid");
            backgroundProcesses[i] = -1; // doing this so that it can be removed from the array
            numOfBackgroundProcessesTerminated += 1; // if this value is greater than 0, we must reorder the array.
        }
        else {
            if (WIFEXITED(status)) {
                int exitStatus = WEXITSTATUS(status);
                if (exitStatus == EXIT_FAILURE) {
                    lastExitStatus = 1;
                }
                else {
                    lastExitStatus = 0;
                }
            }
            else {
                lastExitStatus = 1;
            }
            printf("Background pid %d is done: exit value %d\n", backgroundProcesses[i], lastExitStatus);
            fflush(stdout);
            backgroundProcesses[i] = -1; // doing this so that it can be removed from the array
            numOfBackgroundProcessesTerminated += 1; // if this value is greater than 0, we must reorder the array.
        }
    } // end of for loop
    if (numOfBackgroundProcessesTerminated > 0) { // replacing the
        int j = 0;
        for (int i = 0; i < numberOfBackgroundProcesses; i++) { // running a loop that reorders the array removing pid's that have been marked for termination.
            if (backgroundProcesses[i] != -1) { // if the pid was not terminated, move it up in the array
                backgroundProcesses[j] = backgroundProcesses[i];
                j++;
            }
        } // end of for loop
        numberOfBackgroundProcesses = j; // updating the amount of background processes that are active in the array.
    }
} // end of "checkOnBackgroundProcesses" function


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
    char* userInputCopy = strdup(userInput); // creating a copy to avoid editing the original value
    char* token = strtok(userInputCopy, NOT_ALPHA);
    free(userInputCopy);
    return strcmp(token, "exit") == 0;
} // end of "checkForExit" function


bool checkForStatus(char* userInput) { // this function reads the command and checks if it is 'status'
    char* userInputCopy = strdup(userInput); // creating a copy to avoid editing the original value
    char* token = strtok(userInputCopy, NOT_ALPHA);
    free(userInputCopy);
    return strcmp(token, "status") == 0;
} // end of "checkForStatus" function


void changeDirectory(char* path) {
    char* pathCopy = strdup(path); // creating a copy to avoid editing the original value
    if (pathCopy != NULL) {
        if (chdir(pathCopy) != 0) {
            perror("chdir");
            free(pathCopy);
            fflush(stdout);
        } else {
            strcpy(currentWorkingDirectory, pathCopy);
            free(pathCopy);
        }
    }
} // end of "changeDirectory" function

struct Command getUserInput() {
    if (numberOfBackgroundProcesses > 0) { // checking if there were any processes that finished since the last input.
        checkOnBackgroundProcesses();
    }
    struct Command cmd = {0};
    char buffer[MAX_CHAR_LENGTH];
    char* token;
    printf(": ");
    fflush(stdout);
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) { // if there was an error with fgets, just return an new output to the user
        return cmd;
    }
    buffer[strcspn(buffer, "\n")] = '\0'; // scan buffer until it finds the newline char and replace it with null terminator.
    if (buffer[0] == '\0') { // if the user input nothing, return cmd empty and give the user a new output
        return cmd;
    }
    cmd.name = malloc(sizeof(char) * 15); // allocating 15 characters to the name.
    if (checkForCD(buffer)) { // checking if the cd command was in the buffer
        strcpy(cmd.name, "cd");
        token = strtok(buffer + 3, NOT_ALPHA); // getting the file path.
        cmd.isCd = true;
        if (token != NULL) {
            cmd.cdFilePath = malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(cmd.cdFilePath, token);
        }
    }
    else if (checkForComment(buffer[0])) { // checking if the user inputted a comment
        cmd.isComment = true;
        strcpy(cmd.name, COMMENT_NAME); // setting the name of the command struct to "Comment"
    }
    else if (checkForExit(buffer)) {
        strcpy(cmd.name, EXIT_NAME);
    }
    else if (checkForStatus(buffer)) {
        token = strtok(buffer, NOT_ALPHA);
        strcpy(cmd.name, token);
    }
    else { // commands other than standard (exit, cd, & status) and comment.
        token = strtok(buffer, NOT_ALPHA);
        if (token == NULL) {
            fprintf(stderr, "No command found\n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        strcpy(cmd.name, token); // getting the command
        int avoidInfLoopIndex = 0;
        cmd.numberOfArgs = 0; // setting the number of commands to 0 for use in loop.
        cmd.args = malloc(sizeof(char*) * (MAX_NUM_ARGS + 1)); // allocating the proper memory amount to args.
        while (true) {
            if (cmd.numberOfArgs == 0) { // adding the command to the args
                cmd.numberOfArgs += 1;
                cmd.args[0] = malloc(sizeof(char) * (strlen(cmd.name) + 1));
                strcpy(cmd.args[0], cmd.name);
                cmd.args[1] = NULL;
                continue;
            }
            token = strtok(NULL, NOT_ALPHA);
            if (token == NULL) {
                break; // no more arguments
            }
            if (token[0] != '<' && token[0] != '>' && token[0] != '&') {
                cmd.numberOfArgs += 1;
                cmd.args[cmd.numberOfArgs - 1] = malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(cmd.args[cmd.numberOfArgs - 1], token);
                cmd.args[cmd.numberOfArgs] = NULL;

            }
            else if (token[0] == '<' && strlen(token) == 1) {
                token = strtok(NULL, NOT_ALPHA);
                cmd.inputFile = malloc(sizeof (char) * (strlen(token) + 1));
                strcpy(cmd.inputFile, token);
            }
            else if (token[0] == '>' && strlen(token) == 1) {
                token = strtok(NULL, NOT_ALPHA);
                cmd.outputFile = malloc(sizeof (char) * (strlen(token) + 1));
                strcpy(cmd.outputFile, token);
            }
            else if (token[0] == '&') {
                token = strtok(NULL, NOT_ALPHA);
                if (token == NULL)
                    cmd.runInBackground = true;
            }
            else {
                break;
            }
        } // end of while loop

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
        free(currentWorkingDirectory);
        exit(EXIT_SUCCESS); // exiting the program
    }
    else if (checkForStatus(cmd.name)) {
        if (lastStatusWasSignal) {
            printf("terminated by signal %d\n", lastSignalStatus);
            fflush(stdout);
        }
        else {
            printf("exit value %d\n", lastExitStatus);
            fflush(stdout);
        }
    }
    else if(!cmd.isComment) { // handle all other scenarios that are not comments.
        lastStatusWasSignal = false;
        pid_t pid = fork();
        if (pid == -1) { // checking if the fork failed before using
            perror("fork");
            return;
        }
        else if (pid == 0) { // child proccess
            char* filePathToCommand = getCommandFilePath(cmd.name);
            cmd.commandFilePath = filePathToCommand;
            if (cmd.commandFilePath == NULL) {
                printf("Error: Command file path is NULL\n");
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            if (cmd.inputFile != NULL) { // redirecting the input to a file.
                int inputFile = open(cmd.inputFile, O_RDONLY); // opening file as read only
                if (inputFile == -1) { // error handling for input file
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                if (dup2(inputFile, STDIN_FILENO) == -1) { // redirecting input as the file and error handling
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                if (close(inputFile) == -1) { // error handling close
                    perror("close");
                    exit(EXIT_FAILURE);
                }
            }

            if (cmd.outputFile != NULL) { // redirecting the output to a file
                int outputFile = open(cmd.outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // creating file if it doesn't exist and truncating if it does. also assigning read and write permissions to owner.
                if (outputFile == -1) { // checking that the open is successful before using it. Exiting it not.
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                if (dup2(outputFile, STDOUT_FILENO) == -1) { // calling dup2 and error handling
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                if (close(outputFile) == -1) {
                    perror("close");
                    exit(EXIT_FAILURE);
                }
            }

            if (cmd.runInBackground) {
                if (cmd.inputFile == NULL) {
                    int devNull = open("/dev/null", O_RDONLY);
                    if (devNull == -1) { // throwing an error if unable to open the file.
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    if (dup2(devNull, STDIN_FILENO) == -1) { // redirecting the standard out to /dev/null and checking that there is no erro thrown
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    if (close(devNull) == -1) { // closing the devNull and handling the error if there is one.
                        perror("close");
                        exit(EXIT_FAILURE);
                    }
                }
                if (cmd.outputFile == NULL) {
                    int devNull = open("/dev/null", O_WRONLY); // opening /dev/null in read only
                    if (devNull == -1) { // checking that open worked properly
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    if (dup2(devNull, STDOUT_FILENO) == -1) { // redirecting the standard out to the /dev/null file and handling the error if there is one.
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    if (close(devNull) == -1) { // closing the devNull and handling the error if there is one.
                        perror("close");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            execv(cmd.commandFilePath, cmd.args);
            printf("Error in execv\n");
            fflush(stdout);
            perror("execv\n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        else { // parent process
            int status;
            if (!cmd.runInBackground) {
                foregroundProcess = pid;
                if (waitpid(foregroundProcess, &status, 0) == -1) { // outputting the error of waitpid before using it
                    perror("waitpid");
                } // wait for the child process to finish
                if (WIFEXITED(status)) { // ensuring that the child exited normally
                    if (WEXITSTATUS(status) == EXIT_FAILURE) {
                        lastExitStatus = 1; // setting the last exit status to the child exit status.
                    }
                    else {
                        lastExitStatus = 0;
                    }
                }
                else { // if the child didn't exit normally, set exit status to 1
                    lastExitStatus = 1;
                }
            }
            else {
                backgroundProcesses[numberOfBackgroundProcesses] = pid;
                printf("background pid is %d\n", pid);
                fflush(stdout);
                numberOfBackgroundProcesses += 1;
            }
        }
    }
} // end of "handleUserInput" function


void printCommandStructContents(struct Command cmd) {
    printf("name: %s\n", cmd.name);
    fflush(stdout);
    printf("args: ");
    fflush(stdout);
    for (int i = 0; cmd.args[i] != NULL; i++) {
        printf("%s  ", cmd.args[i]);
        fflush(stdout);
    }
    printf("\nnumberOfArgs: %d\n", cmd.numberOfArgs);
    fflush(stdout);
    printf("inputFile: %s\n", cmd.inputFile);
    fflush(stdout);
    printf("outputFile: %s\n", cmd.outputFile);
    fflush(stdout);
    printf("runInBackground: %d\n", cmd.runInBackground);
    fflush(stdout);
    printf("cdFilePath: %s\n", cmd.cdFilePath);
    fflush(stdout);
    printf("isCd: %d\n", cmd.isCd);
    fflush(stdout);
    printf("isComment: %d\n", cmd.isComment);
    fflush(stdout);
    printf("commandFilePath: %s\n", cmd.commandFilePath);
    fflush(stdout);
}

int main() {
    home = getenv("HOME");
    currentWorkingDirectory = malloc(sizeof(char) * (MAX_PATH_LENGTH + 1));
    getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)); // setting the working directory to the initial directory tha the file is stored in.

    signal(SIGINT, killForegroundProcess); // Set up SIGINT signal handler

    while (true) {
        struct Command cmd = getUserInput();
        if (cmd.name != NULL && !cmd.isComment) { // only handle the command if it's not a comment and the cmd was not null indicating that nothing was entered by the user. If it is a comment, ignore it.
            handleUserInput(cmd);
        }
        freeStructMemory(cmd);
    } // end of while loop
}