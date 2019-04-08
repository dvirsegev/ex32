/// dvir segev
/// 318651627
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>

#define ERROR "Error in system call\n"

#define SIZE 300
#define ARG 2
#define FINAL "final.txt"
#define EXE "a.out"
#define EXCELLENT ",100,GREAT_JOB\n"
#define TIMEOUT ", 40,TIMEOUT\n"
#define GOOD ",80,SIMILAR_OUTPUT\n"
#define BAD ",60,BAD_OUTPUT\n"
#define COMPILE ",20,COMPILATION_ERROR\n"
#define MISS " ,0,NoCFile\n"

static char firstLine[SIZE];
static char secondLine[SIZE];
static char thirdLine[SIZE];
static char theRealPath[SIZE];


/**
 * @param path the path of the file
 * @return the name of the file
 */
char *getLastPath(const char *path) {
    char name[SIZE];
    const char *del = &path[strlen(path)];
    unsigned int i = strlen(path);
    unsigned int j = 0;
    while (del > path && *del != '/') {
        del--;
        i--;
    }

    if (*del == '/') {
        del++;
        while (i <= strlen(path)) {
            name[j] = *del;
            i++, j++, del++;
        }
    }
    char *p = name;
    return p;
}


/**
 * @param status of file.
 * @param text1 the first line.
 * @param text2 the second line.
 * @param text3 the third line.
 */
void readText(int status, char text1[SIZE], char text2[SIZE], char text3[SIZE]) {
    unsigned int total_read = 0;
    char *token;
    char *temp = (char *) calloc(SIZE, sizeof(char));
    unsigned int n_read;
    unsigned int length = 0;
    while ((n_read = read(status, temp + total_read, 1)) > 0) {
        total_read += n_read;
    }
    length = 0;
    token = strtok(temp, "\n");
    if (token != NULL)
        strcpy(text1, token);
    ++length;
    while (token != NULL) {
        token = strtok(NULL, "\n");
        if (length == 1) strcpy(text2, token);
        else if (length == 2)strcpy(text3, token);
        ++length;
    }
    free(temp);
}

/**
 * write text to file with fd
 * @param fd file descriptor
 * @param text text
 */
void write_to_file(int fd, char *text) {
    if ((write(fd, text, strcspn(text, "\0")) == -1)) { // error writing
        write(2, ERROR, sizeof(ERROR) - 1);
        close(fd);
        _exit(1);
    }
}
/**
 * @param path and we just delete the last directory we been
 */
void removeEnd(char *path) {
    char *del = &path[strlen(path)];
    unsigned int i = strlen(path);
    while (del > path && *del != '/') {
        del--;
        path[i] = '\0';
    }

    if (*del == '/')
        *del = '\0';
    memcpy(path, path, i);
}
/**
 *
 * @param path of the file
 * @param typeError which error we write
 */
void writeErrorToFile(const char *path, const char *typeError) {
    // change directory to the path of our work
    chdir(theRealPath);
    // open the file
    int final = open(FINAL, O_RDWR | O_APPEND | O_CREAT, 0777);
    // print the path
    char *buffer = getLastPath(path);
    if (final == -1) {
        write(2, ERROR, sizeof(ERROR) - 1);
        _exit(1);
    }
    write_to_file(final, buffer);
    if (strcmp(typeError, "errorCompile") == 0)
        write_to_file(final,COMPILE);
    else if (strcmp(typeError, "NoCFile") == 0) {
      write_to_file(final,MISS);
    }
    close(final);
}

/**
 * @param argv
 * @param nameFile
 * compile the program.
 */
void compile(const char *argv, const char *nameFile) {
    pid_t pid;
    char *args[3] = {0};
    args[0] = "gcc";
    if (chdir(argv) < 0) {
        perror("lsh");
        printf("\n");
    }
    args[1] = nameFile;
    args[2] = NULL;
    if ((pid = fork()) < 0) {     /* fork a child process           */
        printf("*** ERROR: forking child process failed\n");
    } else if (pid == 0) {          /* for the child process:         */
        if (execvp(args[0], args) < 0) {     /* compile the command  */
            exit(1);
        }
    } else {                                  /* for the parent:      */
        wait(&pid);
        int exit_status = WEXITSTATUS(pid);
        // child exit was not good(doesn't compile).
        if (exit_status) {
            writeErrorToFile(argv, "errorCompile");
        }
    }
}

/**
 * @param name
 * @return 1 if the file is a type of C file.
 */
int fileIsC(char *name) {
    return strcmp(".c", &name[strlen(name) - 2]) == 0;
}

/**
 * @param dip
 * @param dp
 * @param path the path
 * @param count how many time we go deep.
 * get into each directory and compile the c files.
 */
void openFolders(DIR *dip, struct dirent *dp, char *path,int count) {
    int flagCfile = 0;
    DIR *temp = NULL;
    while ((dp = readdir(dip)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0
            || strcmp(dp->d_name, EXE) == 0)
            continue;
        if (fileIsC(dp->d_name)) {
            flagCfile = 1;
            compile(path, dp->d_name);
        } else {
            getcwd(path, sizeof(path));
            strcat(path, "/");
            strcat(path, dp->d_name);
            if ((temp = opendir(path)) != NULL) {
                openFolders(temp, dp, path,count+1);
            } else {
                removeEnd(path);

            }
        }
    }
    if (!flagCfile && count==1) {
        writeErrorToFile(path, "NoCFile");
    }
    removeEnd(path);
    closedir(temp);
}


/**
 * @param time how much time our program pass
 * @param path our path of the file
 */
void compareResult(int time, char *path) {
    pid_t pid;
    int exit_status;
    char *buffer = getLastPath(path);
    chdir(theRealPath);
    strcat(path, "/out.txt");
    char *args[] = {"./comp.out", thirdLine, path, NULL};
    int final = open(FINAL, O_RDWR | O_APPEND | O_CREAT, 0777);
    if (final == -1) {
        write(2, ERROR, sizeof(ERROR) - 1);
        _exit(1);
    }
    // if the file.c was longer then 51 second
    if (time >= 51) {
        write_to_file(final, buffer);
        write_to_file(final,TIMEOUT);
    } else {
        if ((pid = fork()) < 0) {     /* fork a child process           */
            printf("*** ERROR: forking child process failed\n");
            exit(-1);
        }
        if (pid == 0)
            /* for the child process:         */
            execvp(args[0], args);
            /* run the command  */
        else {
            // wait for the child process.
            waitpid(pid, &exit_status, WCONTINUED);
            // how the comp.out was finish
            exit_status = WEXITSTATUS(exit_status);
            write(final, buffer, strlen(buffer));
            switch (exit_status) {
                case 1:
                    write_to_file(final, EXCELLENT);
                    break;
                case 2:
                    write_to_file(final, BAD);
                    break;
                case 3:
                    write_to_file(final, GOOD);
                    break;
                default:
                    break;
            }
            close(final);
        }

    }

}
/**
 * @param path run the a.out
 */
void execute(char *path) {
    if (chdir(path) < 0) {
        perror("lsh");
        printf("\n");
    }
    int in, out, status;
    pid_t pid;
    stdin = dup(0);
    stdout = dup(1);
    char *mission[] = {"./a.out", NULL};

    // open input and output files

    in = open(secondLine, O_RDONLY);
    out = open("out.txt", O_WRONLY | O_CREAT, 0777);
    if (in == -1 || out == -1) {
        write(2, ERROR, sizeof(ERROR) - 1);
        _exit(1);
    }

    // replace standard input with input file
    dup2(in, 0);
    // replace standard output with output file
    dup2(out, 1);

    // execute mission

    if ((pid = fork()) < 0) {     /* fork a child process           */
        printf("*** ERROR: forking child process failed\n");
    } else if (pid == 0) {
        /* for the child process:         */
        if (execvp(mission[0], mission) < 0) {     /* compile the command  */
            exit(1);
        }
    } else {                                  /* for the parent:      */
        int time = 0;
        while (waitpid(pid, &status, WNOHANG) == 0 && time <= 50) {
            usleep(100000); // sleep 0.1 seconds
            ++time;
        }
        close(in);
        dup2(stdin, 0); // restore stdin
        dup2(stdout, 1); // restore stdout
        close(out);
        compareResult(time, path);
    }

}



/**
 *
 * @param dip the DIR
 * @param dp direct struct
 * @param path the path
 * the function is a recusion, just find each directory the file a.out and send it
 * to runAout
 */
void runProg(DIR *dip, struct dirent *dp, char path[SIZE]) {
    DIR *temp;
    while ((dp = readdir(dip)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;
        if (strcmp(dp->d_name, EXE) == 0) {
            execute(path);
            removeEnd(path);
            break;
        } else {
            getcwd(path, sizeof(path));
            strcat(path, "/");
            strcat(path, dp->d_name);
            if ((temp = opendir(path)) != NULL) {
                runProg(temp, dp, path);
            } else removeEnd(path);
        }
    }
    removeEnd(path);
}


/**
 *
 * @param firstLine the first line in the file
 * @return the DIR
 */
DIR *openDirectory() {
    DIR *dip;
    if ((dip = opendir(firstLine)) == NULL) {
        perror("opendir");
        return NULL;
    }
    return dip;
}


int main(int argc, char **argv) {
    if (argc != ARG) return 1;
    /// get the path of the project and save it.
    getcwd(theRealPath, sizeof(theRealPath));
    int file = 0;
    /// open the file
    if ((file = open(argv[1], O_RDWR | O_APPEND)) < -1) return 1;
    // close file
    /// get the lines of the file.
    readText(file, firstLine, secondLine, thirdLine);
    close(file);
    DIR *dip = openDirectory();
    /// change directory to the first line
    chdir(firstLine);
    struct direct *sd;
    char path[SIZE];
    getcwd(path, sizeof(path));
    char savePath[SIZE];
    strcpy(savePath, path);
    /// open the folders
    openFolders(dip, sd, path,0);
    dip = openDirectory();
    /// run and compare all the program
    runProg(dip, sd, savePath);
    // close DIR
    closedir(dip);
    return 0;
}