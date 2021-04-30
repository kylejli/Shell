//
//  wish.cpp
//  
//
//  Created by Kyle on 4/19/21.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

// Prints Error Message to stderr
void printError() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Splits String into Vector, Each Element
// Containing an argument or ">" or "&"
// Also Changes isParallel in Main if "&" found
vector<char*> splitArgs(char* text, bool &isParallel) {
    vector<char*> ret;              // return vector
    string s(text);                 // convert input char* to string to use string library
    int first = 0;                  // index of first char of next argument to push_back
    size_t j = 0;                   // index of first char of first arg after leading whitespace
    isParallel = false;
    // remove leading whitespace
    while (isspace(s[j])) {
        // If whole string is white space, return empty vector
        if (j == s.size() - 1) {
            return (ret);
        }
        j++;
        first++;
    }
    // remove trailing whitespace
    while(isspace(s[s.size()-1])) {
        s.pop_back();
    }
    s.shrink_to_fit();
    // Find whitespace, '&', '>'
    for (size_t i = j; i < s.size(); i++) {
        // If Whitespace
        if (isspace(s[i])) {
            // Puts argument before the space into return vector
            ret.push_back(strdup(s.substr(first,i - first).c_str()));
            // Skip following whitespace
            while (isspace(s[i + 1])) {
                i++;
            }
            // update first to first char of next arg
            first = i + 1;
        }
        // If '&'
        else if (s[i] == '&') {
            // If No Whitespace Before '&', then push_back previous argument
            if (!isspace(s[i - 1])) {
                ret.push_back(strdup(s.substr(first,i - first).c_str()));
            }
            // Push_back '&'
            ret.push_back(strdup("&"));
            // Set isParallel in main to true
            isParallel = true;
            // Skip white space following '&'
            while (isspace(s[i + 1])) {
                i++;
            }
            // update first to first char of next arg
            first = i + 1;
        }
        // If '>'
        else if (s[i] == '>') {
            // If '>' is not first arg and no whitespace Before '>', then push_back previous argument
            if (i > j && !isspace(s[i - 1]))
                ret.push_back(strdup(s.substr(first,i - first).c_str()));
            // Push_back '>'
            ret.push_back(strdup(">"));
            // Skip white space following '>'
            while (isspace(s[i + 1])) {
                i++;
            }
            // update first to first char of next arg
            first = i + 1;
        }
    }
    // If there is one last word, push it back
    if (s.substr(first).size() > 0)
        ret.push_back(strdup(s.substr(first).c_str()));
    return(ret);
}

// Checks vector of split args for redirection
// Returns 1 if valid redirection, 0 if no redirection, -1 if bad redirection
int isRedirection(vector<char*> args) {
    size_t index = 0;
    // '>' cant be first character of command
    if (!strcmp(args[0],">")) {
        printError();
        return (-1);
    }
    // check for multiple redirects
    for (size_t i = 1; i < args.size(); i++) {
        if (!strcmp(args[i],">")){
            if (index == 0) {
                index = i;
            }
            else {
                printError();
                return (-1);
            }
        }
    }
    // If no redirection
    if (index == 0)
        return (0);
    // Check if only one output file for redirection
    else if (index == args.size() - 2)
        return (1);
    else {
        printError();
        return (-1);
    }
}

// runs given vector of args, path, and isParallel
void runCommand(vector<char*> args, vector<char*> &path, bool &isParallel) {
    pid_t ret;                  // fork id
    // If parallel, call runCommand on the separate commands
    if (isParallel) {
        isParallel = false;     // run each command without isParallel flag
        int numChildren = 1;    // numChildren = number of '&' + 1
        size_t start = 0;       // start is first argument of each command
        for (size_t i = 0; i < args.size(); i++) {
            // If vector element == "&", run the command before this "&"
            if (strcmp(args[i],"&") == 0) {
                // Create new process to run command
                ret = fork();
                // Check if fork succcessful
                if (ret < 0) {
                    printError();
                }
                // Child runs command
                else if (ret == 0) {
                    // push back commands between last "&" and current "&"
                    vector<char*> childArgs;
                    for (size_t j = start; j < i; j++) {
                        childArgs.push_back(args[j]);
                    }
                    // run command
                    runCommand(childArgs, path, isParallel);
                    // kill child
                    exit(0);
                }
                // increment numChildren and update start
                else {
                    numChildren++;
                    start = i + 1;
                }
            }
        }
        // Parent needs to create child to run last command
        if (ret > 0) {
            ret = fork();
            // Check if fork successful
            if (ret < 0) {
                printError();
                return;
            }
            // Child runs last command
            else if (ret == 0) {
                vector<char*> childArgs;
                // push back last command
                for (size_t j = start; j < args.size(); j++) {
                    childArgs.push_back(args[j]);
                }
                // run command
                runCommand(childArgs, path, isParallel);
                // kill child
                exit(0);
            }
            // wait for numChildren children
            else {
                for (int j = 0; j < numChildren; j++) {
                    wait(NULL);
                }
                return;
            }
        }
    }
    // call isRedirection to check for redirection
    int isRedirect = isRedirection(args);
    // return if bad redirection
    if (isRedirect == -1) {
        return;
    }
    // Check Built-In Commands
    // If "exit"
    else if(!strcmp(args[0],strdup("exit"))) {
        // Check that exit has no additional arguments
        if (args.size() == 1) {
            exit(0);
        }
        else {
            printError();
            return;
        }
    }
    // If "cd"
    else if(!strcmp(args[0],strdup("cd"))) {
        // check only one argument for cd
        if (args.size() == 2) {
            // call chdir with given argument as parameter
            chdir(args[1]);
        }
        else {
            printError();
        }
        return;
    }
    // If "path"
    else if(!strcmp(args[0],strdup("path"))) {
        // Create new vector with given arguments as elements
        vector<char*> newPath;
        for (size_t i = 1; i < args.size(); i++) {
            newPath.push_back(args[i]);
        }
        // Swap newPath with path in main (by reference)
        path.swap(newPath);
        return;
    }
    // Non-built in commands
    else {
        // Check each path in path vector for command
        for (size_t i = 0; i < path.size(); i++) {
            // fullPath is the full path of the command, path/command, e.g. bin/ls
            char *fullPath = strdup(path[i]);
            strcat(fullPath, strdup("/"));
            strcat(fullPath, args[0]);
            // Check if command executable
            if (access(fullPath, X_OK) == 0) {
                // Create child to run command
                pid_t ret = fork();
                // Check if fork successful
                if (ret < 0) {
                    printError();
                    return;
                }
                // Child runs command
                else if (ret == 0) {
                    char *newargs[args.size() + 1];             // newargs is NULL terminated array of args
                    size_t newargsSize = args.size() + 1;
                    for (size_t j = 0; j < args.size(); j++) {
                        newargs[j] = args[j];
                    }
                    newargs[newargsSize - 1] = NULL;
                    int fileDescriptor = -1;
                    // If Redirection
                    if (isRedirect > 0){
                        // Open redirection output file
                        fileDescriptor = open(newargs[newargsSize - 2], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
                        // Check if open successful
                        if (fileDescriptor < 0) {
                            printError();
                            exit(0);
                        }
                        // Set STDOUT and STDERR to output file
                        dup2(fileDescriptor, STDOUT_FILENO);
                        dup2(fileDescriptor, STDERR_FILENO);
                        char *redirectArgs[newargsSize-2];              // redirectArgs is NULL terminated array of args, without the redirection symbol '>' and the output file
                        for (size_t j = 0; j < newargsSize - 3; j++) {
                            redirectArgs[j] = newargs[j];
                        }
                        redirectArgs[newargsSize-3] = NULL;
                        // call execv with full path of command and arguments
                        execv(fullPath, redirectArgs);
                        // close file descriptor
                        close(fileDescriptor);
                    }
                    // If no redirection
                    else {
                        // call execv with full path of command and arguments
                        execv(fullPath, newargs);
                    }
                    exit(0);
                }
                // Parent waits for child
                else {
                    wait(NULL);
                    return;
                }
            }
        }
    }
    // If no command found, print error
    printError();
    return;
}

int main(int argc, char *argv[]) {
    vector<char*> path;                 // vector containing paths
    bool isParallel = false;            // bool indicating if arguments have '&'
    path.push_back(strdup("/bin"));     // initial path "/bin"
    // No more than 2 arguments (.wish and input batch file)
    if (argc > 2) {
        printError();
        exit(1);
    }
    // Batch Mode
    if (argc == 2) {
        char tempChar;                  // temp char to read to
        string buffer;                  // string to hold most recent line
        size_t bytesRead;               // number of bytes read
        int fileDescriptor;             // file descriptor of given batch file
        fileDescriptor = open(argv[1], O_RDONLY);
        // Check if Open Successful
        if (fileDescriptor < 0) {
            printError();
            exit(1);
        }
        vector<char*> myargs;
        // Attempt to Read at Least Once, Don't Repeat if No Bytes Read
        do {
            bytesRead = read(fileDescriptor, &tempChar, 1);
            // Check if end of line
            if (tempChar == '\n') {
                // If buffer not empty
                if (buffer.size() > 0) {
                    myargs = splitArgs(strdup(buffer.c_str()), isParallel);     // split buffer into arguments
                    buffer.clear();                                             // clear buffer for next line
                    buffer.shrink_to_fit();
                    // call runCommand if myargs not empty
                    if (myargs.size() > 0)
                        runCommand(myargs, path, isParallel);
                }
            }
            // Push back chars to end of buffer if not '\n'
            else {
                buffer.push_back(tempChar);
            }
        }
        while (bytesRead > 0);
        // Check for final line
        if (buffer.size() > 0) {
            myargs = splitArgs(strdup(buffer.c_str()), isParallel); // split buffer into arguments
            // call runCommand if myargs not empty
            if (myargs.size() > 0)
                runCommand(myargs, path, isParallel);
        }
        // close file descriptor
        close(fileDescriptor);
    }
    // Interactive mode
    else {
        char *buffer = NULL;    // buffer containing line of user input
        size_t len = 0;         // buffer size (automatically resized by getline)
        ssize_t nread;          // number of characters read, -1 if fail
        
        // Repeat until user inputs "exit"
        bool done = false;
        while (!done) {
            // Get user input
            cout << "wish> ";
            nread = getline(&buffer, &len, stdin);
            // Check if getline successful
            if (nread == -1)
                printError();
            else {
                // split string into vector of args
                vector<char*> myargs = splitArgs(buffer, isParallel);
                // call runCommand if myargs not empty
                if (myargs.size() > 0)
                    runCommand(myargs, path, isParallel);
            }
        }
    }
    return(0);
}
