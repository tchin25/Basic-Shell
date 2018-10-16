#include <iostream>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <sys/fcntl.h>
#include <sys/wait.h>

void parseCommand(char* input);
void specialCharacterCheck(char* input);
char** splitInput(char* input, int spChar);

char* shellDirectory;
char* currentDirectory;

int main(int argc, char** argv) {
    //printf("%s", argv[0]);
    char* buffer = (char*) calloc(200, sizeof(char));
    getcwd(buffer, 200);
    shellDirectory = buffer;
    currentDirectory = buffer;

    if(argc == 2 && strcmp(argv[1], "batchfile") == 0){//Run commands from file
        FILE* toRead = fopen("batchfile", "r");
        if (toRead == NULL){
            puts("Unable to read file");
            return 1;
        }
        char line [100];
        char junk [100];
        while(!feof(toRead)){
            fscanf(toRead,"%[^\n\t\r]s",line); //Get text
            //printf("%s\n", line);

            fscanf(toRead,"%[\n\t\r]",junk); //Remove any 'white space' characters
#ifdef DEBUG
            printf("Executing: %s\n", line);
#endif
            //Replaces \n with \0
            char *end = strchr(line, '\n');
            //printf("End: '%s'\n", end);
            if(end != NULL){
                strcpy(end, "\0");
            }
            printf("TUShell> %s\n", line);

            //Checks if run in background
            pid_t id = -1;
            //printf("Last character: '%c'\n", line[strlen(line)-1]);
            if(line[strlen(line)-1] == '&'){
                id = fork();
                if(id == 0){ //Is child
                    //Removes & from line
                    char* cache = (char*) malloc(strlen(line) * sizeof(char));
                    strncpy(cache, line, strlen(line)-1);
                    cache[strlen(line)-1] = '\0';
                    strcpy(line, cache);
#ifdef DEBUG
                    printf("Forked successfully with %s\n", line);
#endif
                }
            }

            //Checks to see if exits
            if(strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0){
                break;
            }else if(strcmp(line, "pause") == 0){
                puts("Press Enter to Continue");
                while(getchar() != '\n'){}
            } else{
                if (id <= 0) { //if no fork or is child. Prevents double execution
                    specialCharacterCheck(line);
                }
                //If child exit
                if (id == 0){
                    return 0;
                }
            }
        }
        fclose(toRead);
    }else{//Run user commands

        while (true){
            char *input = (char*) calloc(100, sizeof(char));
            printf("TUShell> ");
            fgets(input, 100, stdin);

            //Replaces \n with \0
            char *end = strstr(input, "\n");
            strcpy(end, "\0");
            //Checks if run in background
            pid_t id = -1;
            if(input[strlen(input)-1] == '&'){
                id = fork();
                if(id == 0){ //Is child
                    //Removes & from line
                    char* cache = (char*) malloc(strlen(input) * sizeof(char));
                    strncpy(cache, input, strlen(input)-1);
                    cache[strlen(input)-1] = '\0';
                    strcpy(input, cache);
#ifdef DEBUG
                    printf("Forked successfully with %s", input);
#endif
                }
            }

            //Checks to see if exits
            if(strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0){
                free(input);
                break;
            }else if(strcmp(input, "pause") == 0){
                puts("Press Enter to Continue");
                while(getchar() != '\n'){}
            } else{
                if (id <= 0) { //if no fork or is child. Prevents double execution
                    specialCharacterCheck(input);
                }
                //If child exit
                if (id == 0){
                    return 0;
                }
            }
        }
    }
    puts("Exiting");
    return 0;
}

void parseCommand(char *input) {
    size_t lengthOfCommand = strcspn(input, " ");
    char* command = (char*) calloc(lengthOfCommand+1, sizeof(char)); //Calloc command plus space for \0
    strncpy(command, input, lengthOfCommand); //Copies input up to length of command into command
    char* arguments = strchr(input, ' '); //Sets arguments to be after command
    if(arguments != NULL){
        arguments++; //Moves pointer past space after command
    }

#ifdef DEBUG
    printf("Length of Command: %li | Command: %s | Arguments: %s\n", lengthOfCommand, command, arguments);
#endif
    if(strcmp(command, "cd") == 0){
        if(arguments == NULL){
            printf("%s\n", currentDirectory);
        }else{
            if(chdir(arguments) < 0){
                puts("Error changing directory");
            }else{
                char* buffer = (char*) calloc(200, sizeof(char));
                getcwd(buffer, 200);
                currentDirectory = buffer; //Changes current working directory
            }
        }
    }else if(strcmp(command, "clr") == 0){
        system("clear");
    }else if(strcmp(command, "dir") == 0){
        //If no argument, dir ./
        DIR* dir;
        if (arguments == NULL){
            dir = opendir("./");
        }else{
            dir = opendir(arguments);
        }

        if(dir == NULL) {
            puts("Cannot open directory");
        }else {
            dirent *drnt = readdir(dir);
            while (drnt != NULL) { //Runs through every dirent struct and prints its name
                printf("%s\n", drnt->d_name);
                drnt = readdir(dir);
            }
            closedir(dir);
        }
    }else if(strcmp(command, "environ") == 0){
        system("printenv");
        printf("Shell Directory: %s\n", shellDirectory);
        printf("Current Directory: %s\n", currentDirectory);
    }else if(strcmp(command, "echo") == 0){
        printf("%s\n", arguments);
    }else if(strcmp(command, "help") == 0){
        FILE* fp = fopen("readme", "r");
        char line[100];
        size_t length = 100;

        if(fp == NULL){
            printf("File not found.\n");
        }else{
            while(fgets(line, length, fp)){ //Prints each line
                printf("%s", line);
            }
        }
    }else{
        pid_t id = fork();
        if(id == 0){ //If fork is child
            char* ptr = strtok(input, " ");
            char** argv = NULL;
            int i = 0;
            while(ptr != NULL){ //Splits input into array
                i++;
                argv = (char**) realloc(argv, sizeof(char*) * i);
                argv[i-1] = ptr;
                ptr = strtok(NULL, " ");
            }

            argv = (char**) realloc(argv, sizeof (char*) * (i+1));
            argv[i] = 0;

#ifdef DEBUG
            puts("Arguments for execvp:");
            for (int j = 0; j < (i+1); ++j){
                printf ("res[%d] = %s\n", j, argv[j]);
            }
#endif

            if(execvp(command, argv) == -1) {
                puts("Command not found");
                exit(-1);
            }
        }else{
            waitpid(id, NULL, 0);
        }
    }
}

void specialCharacterCheck(char *input) {
    if (strchr(input, '|') != NULL){ //has pipe
#ifdef DEBUG
        puts("Detected '|'");
#endif
        //Tested using ps | wc
        char** inputs = splitInput(input, 1);
        int pfsd[2];
        if(pipe(pfsd) == 0){
            if(fork() == 0){ //Child - left side
                close(1); //Close stdout
                dup2(pfsd[1], 1); //Wires stdout into write end of pipe
                close(pfsd[0]); //Closes read end of pile
                parseCommand(inputs[0]); //Parses left side of pipe
                exit(1);
            }else { //Parent - right side
                int kbm = dup(0);
                close(0); //Close stdin
                dup2(pfsd[0], 0); //Wires stdin into read end of pipe
                close(pfsd[1]); //Closes write end of pipe
                parseCommand(inputs[1]); //Parses right side of pipe
                close(pfsd[0]); //Closes pipe
                dup2(kbm, 0); //Reopens stdin
                wait(NULL);
            }
        }
    }else if (strstr(input, ">>") != NULL){ //Output redirect, if file exist, append
#ifdef DEBUG
        puts("Detected '>>'");
#endif
        //Tested
        char** inputs = splitInput(input, 5);
        int fd = open(inputs[1], O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
        int sout = dup(1); //Stores sout file descriptor
        dup2(fd, 1); //Makes file stdout
        parseCommand(inputs[0]); //Parses command specified on left side of >>
        dup2(sout, 1); //Restores sout into stout
        close(fd);
    }else if (strchr(input, '>') != NULL){ //Output redirect, if file exist, overwrite
        if(strchr(input, '<') != NULL){ //Has both input and output redirect
#ifdef DEBUG
            puts("Detected '>' and  '<'");
#endif
            //Tested wc < input > output and wc > output < input
            char** inputs = splitInput(input, 4);
            remove(inputs[2]);
            int fd = open(inputs[2], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU); //Opens file specified on right side of >
            int sout = dup(1); //Stores stdout file descriptor
            dup2(fd, 1); //Makes file stdout
            int fd1 = open(inputs[1], O_RDONLY); //Opens file specified on right side of <
            int kbm = dup(0); //Stores keyboard file descriptor
            dup2(fd1, 0); //Makes file stdin
            parseCommand(inputs[0]); //Parses command specified on left side of >
            dup2(sout, 1); //Restores sout into stdout
            dup2(kbm, 0); //Restores keyboard as stdin
            close(fd);
            close(fd1);
        }else{ //Just output
#ifdef DEBUG
            puts("Detected '>'");
#endif
            //Tested ls > file
            char** inputs = splitInput(input, 3);
            remove(inputs[1]);
            int fd = open(inputs[1], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU); //Opens file specified on right side of >
            int sout = dup(1); //Stores stdout file descriptor
            dup2(fd, 1); //Makes file stdout
            parseCommand(inputs[0]); //Parses command specified on left side of >
            dup2(sout, 1); //Restores sout into stdout
            close(fd);
        }
    }else if (strchr(input, '<') != NULL){ //has input redirect
#ifdef DEBUG
        puts("Detected '<'");
#endif
        //Tested wc < ls
        char** inputs = splitInput(input, 2);
        int fd = open(inputs[1], O_RDONLY); //Opens file specified on right side of <
        int kbm = dup(0); //Stores keyboard file descriptor
        dup2(fd, 0); //Makes file stdin
        parseCommand(inputs[0]); //Parses command specified on left side of <
        dup2(kbm, 0); //Restores keyboard as stdin
        close(fd);
    }else{ //Normal
#ifdef DEBUG
        puts("Normal execution");
#endif
        parseCommand(input);
    }
}

//Normal = 0
//Pipe = 1
//< = 2
//> = 3
//Input and Output = 4
//>> = 5
char **splitInput(char *input, int spChar) {
    static char** inputs = new char*[3];
    char* input1 = NULL;
    char* input2 = NULL;
    char* input3 = NULL;

    if (spChar == 4){
        //input1 = inputs[0] = Command
        //input2 = inputs[1] = Input
        //input3 = inputs[2] = Output
        //Tested
        char* out = strchr(input, '>'); //Pointer to output
        char* in = strchr(input, '<'); //Pointer to input
        if (out > in){ //> is after <. i.e: command < input > output
            //puts("command < input > output");
            size_t lengthOfInput1 = strcspn(input, "<"); //Gets length up until special character
            input1 = (char*) calloc(lengthOfInput1, sizeof(char)); //Assuming space before special character
            strncpy(input1, input, lengthOfInput1-1); //Copy up until space, replace space with \0
            in += 2;
            char** secondSplit = splitInput(in, 3); //Splits substring
            input2 = secondSplit[0];
            input3 = secondSplit[1];
        }else{ //< is after >. i.e: command > output < input
            //puts("command > output < input");
            size_t lengthOfInput1 = strcspn(input, ">"); //Gets length up until special character
            input1 = (char*) calloc(lengthOfInput1, sizeof(char)); //Assuming space before special character
            strncpy(input1, input, lengthOfInput1-1); //Copy up until space, replace space with \0
            out += 2;
            char** secondSplit = splitInput(out, 2); //Splits substring
            input2 = secondSplit[1];
            input3 = secondSplit[0];
        }

    }else if (spChar == 5){
        //Tested
        //char sc[2] = {'>', '\0'};
        size_t lengthOfInput1 = strcspn(input, ">"); //Gets length up until special character
        input1 = (char*) calloc(lengthOfInput1, sizeof(char)); //Assuming space before special character
        strncpy(input1, input, lengthOfInput1-1); //Copy up until space, replace space with \0
        input2 = (char*) calloc(strlen(strchr(input, '>')+2), sizeof(char)); //Assuming space after special character, moves pointer past it and copies
        strcpy(input2, strchr(input, '>')+3);
    }else{
        char specialCharacter = 0;
        switch (spChar){
            case 1:
                specialCharacter = '|';
                break;
            case 2:
                specialCharacter = '<';
                break;
            case 3:
                specialCharacter = '>';
                break;
            default:
                return NULL;
        }
        char sc[2] = {specialCharacter, '\0'};
        size_t lengthOfInput1 = strcspn(input, sc); //Gets length up until special character
        input1 = (char*) calloc(lengthOfInput1, sizeof(char)); //Assuming space before special character
        strncpy(input1, input, lengthOfInput1-1); //Copy up until space, replace space with \0
        input2 = (char*) calloc(strlen(strchr(input, specialCharacter)+1), sizeof(char)); //Space for right side of special character
        strcpy(input2, strchr(input, specialCharacter)+2); //Assuming space after special character, moves pointer past it and copies
    }

#ifdef DEBUG
    printf("Input 1: %s | Input 2: %s | Input 3: %s\n", input1, input2, input3);
#endif

    inputs[0] = input1;
    inputs[1] = input2;
    inputs[2] = input3;
    return inputs;
}
