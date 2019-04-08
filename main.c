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

static char firstLine[SIZE];
static char secondLine[SIZE];
static char thirdLine[SIZE];
static char theRealPath[SIZE];

void removeEnd(char *path);

void execute(char *path);

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

void writeErorToFile(const char *path, const char *typeError) {
    chdir(theRealPath);
    int final = open("final.txt", O_RDWR | O_APPEND | O_CREAT, 0777);
    char *buffer = getLastPath(path);
    if (final == -1) {
        write(2, ERROR, sizeof(ERROR) - 1);
        _exit(1);
    }
    char *set;
    write(final, buffer, strlen(buffer));
    if (strcmp(typeError, "errorCompile") == 0)
        set = ",20,COMPILATION_ERROR\n";
    else if (strcmp(typeError, "NoCFile") == 0) {
        set = " ,0,NoCFile\n";
    }
    write(final, set, strlen(set));
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
            writeErorToFile(argv, "errorCompile");
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
 * get into each directory and compile the c files.
 */
void openFolder(DIR *dip, struct dirent *dp, char path[SIZE]) {
    int flagCfile = 0;
    int countFolder = 0;
    DIR *temp = NULL;
    while ((dp = readdir(dip)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0
            || strcmp(dp->d_name, "a.out") == 0)
            continue;
        if (fileIsC(dp->d_name)) {
            flagCfile = 1;
            compile(path, dp->d_name);
        } else {
            getcwd(path, sizeof(path));
            strcat(path, "/");
            strcat(path, dp->d_name);
            countFolder++;
            if ((temp = opendir(path)) != NULL) {
                --countFolder;
                openFolder(temp, dp, path);
            } else removeEnd(path);
        }
    }
    if (!flagCfile && countFolder > 0) {
        writeErorToFile(path, "NoCFile");
    }
    removeEnd(path);
    closedir(temp);
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
        if (strcmp(dp->d_name, "a.out") == 0) {
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


void compareResult(int time, char *path) {
    pid_t pid;
    int exit_status;
    char *buffer = getLastPath(path);
    char *r = theRealPath;
    chdir(theRealPath);
    strcat(path, "/out.txt");
    char *args[] = {"./comp.out", thirdLine, path, NULL};
    int final = open("final.txt", O_RDWR | O_APPEND | O_CREAT, 0777);
    char *set;
    if (final == -1) {
        write(2, ERROR, sizeof(ERROR) - 1);
        _exit(1);
    }
    if (time >= 51) {
        write(final, buffer, strlen(buffer));
        set = ", 40,TIMEOUT\n";
        write(final, set, strlen(set));
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
            waitpid(pid, &exit_status, WCONTINUED);
            exit_status = WEXITSTATUS(exit_status);
            write(final, buffer, strlen(buffer));
            switch (exit_status) {
                case 1:
                    set = ",100,GREAT_JOB\n";

                    write_to_file(final, set);
                    break;
                case 2:
                    set = ",60,BAD_OUTPUT\n";
                    write_to_file(final, set);
                    break;
                case 3:
                    set = ",80,SIMILAR_OUTPUT\n";
                    write_to_file(final, set);
                    break;

            }
            close(final);
        }

    }

}

void execute(char *path) {
    if (chdir(path) < 0) {
        perror("lsh");
        printf("\n");
    }
    int in, out, status;
    pid_t pid;
    stdin = dup(0);
    stdout = dup(1);
    char *grep_args[] = {"./a.out", NULL};

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

    // close unused file descriptors

    // execute grep

    if ((pid = fork()) < 0) {     /* fork a child process           */
        printf("*** ERROR: forking child process failed\n");
    } else if (pid == 0) {
        /* for the child process:         */
        if (execvp(grep_args[0], grep_args) < 0) {     /* compile the command  */
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
    // get the path of the project and save it.
    getcwd(theRealPath, sizeof(theRealPath));
    int file = 0;
    // open the file
    if ((file = open(argv[1], O_RDWR | O_APPEND)) < -1) return 1;
    // close file
    /// get the lines of the file.
    readText(file, firstLine, secondLine, thirdLine);
    close(file);
    DIR *dip = openDirectory();
    // change directory to the first line
    chdir(firstLine);
    struct direct *sd;
    char path[SIZE];
    getcwd(path, sizeof(path));
    char savePath[SIZE];
    strcpy(savePath, path);
    /// open the folders
    openFolder(dip, sd, path);
    dip = openDirectory();
    /// run and compare all the program
    runProg(dip, sd, savePath);
    // close DIR
    closedir(dip);
    return 0;
}