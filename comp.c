#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>



struct name {
    char source[256];                                                                                           // source file
    char old_source[256];                                                                                       // old source file (for repeat command)
    char bin[256];                                                                                              // binary file
} name;

struct pid {
    pid_t compile;                                                                                              // compile processes
    pid_t run;                                                                                                  // run processes
    pid_t self;                                                                                                 // self-compile process
    pid_t ls;                                                                                                   // list files process
} pid;



int col() {
    struct termios oldt, newt;                                                                                  // terminal settings
    char buf[32];                                                                                               // buffer for cursor position response
    int i = 0;                                                                                                  // just a counter

    tcgetattr(STDIN_FILENO, &oldt);                                                                             // get current terminal settings
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);                                                                           // disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);                                                                    // request cursor position report

    printf("\033[6n");                                                                                          // ANSI escape code to request cursor position
    fflush(stdout);

    while (i < sizeof(buf) - 1) {                                                                               // read the response: ESC [ rows ; cols R
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;                                                         // | read one character at a time
        if (buf[i] == 'R') break;                                                                               // | stop at 'R'
        i++;                                                                                                    // ^ ++
    }
    buf[i + 1] = '\0';                                                                                          // restore terminal settings

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);                                                                    // parse the response to get the current column number

    int row, col;                                                                                               // if column is 1, it's a new line
    if (sscanf(buf, "\033[%d;%dR", &row, &col) == 2) return (col == 1);                                         // ^ read

    return -1;                                                                                                  // else -1
}

// self-compile function

void self_comp() {
    pid.self = fork();                                                                                          // compile
    if (pid.self == 0) {                                                                                        // |
        execvp("gcc", (char *[]){"gcc", "comp.c", "-o", "comp.c.bin", NULL});                                   // |
        exit(1);                                                                                                // ^

    } else {
        int status;                                                                                             // run
        waitpid(pid.self, &status, 0);                                                                          // |    wait for compilation
        if (WIFEXITED(status)) {                                                                                // |
            int code = WEXITSTATUS(status);                                                                     // |
            if (!code) { execvp("./comp.c.bin", (char *[]){"./comp.c.bin", NULL}); }                            // |    run if compilation succeeded
            else { printf("Compilation failed with code %d.\n", code); }                                        // ^    print error if compilation failed
}}}

// run function

void run() {
    printf("Running %s...\n\\/-----------\\/\n", name.bin);                                                     // notification
    pid.run = fork();
    if (pid.run == 0) {
        signal(SIGINT, SIG_DFL);                                                                                // allow interruption of the child process
        execvp(name.bin, (char *[]){name.bin, NULL}); exit(1);                                                  // run
    } else {
        wait(NULL);
        if (!col()) printf("\n");
        printf("/\\-----------/\\\n");                                                                          // notification
    }
}



// ======= program ==================================================================================

int main() {
signal(SIGINT, SIG_IGN);                                                                                        // ignore interruption
while (1) {

    // greeting
    printf(" >> ");                                                                                             // greeting
    scanf("%s", name.source);                                                                                   // input

    // commands
    if (strcmp(name.source, "r") == 0) { strcpy(name.source, name.old_source); }                                // repeat

    if (strcmp(name.source, "q") == 0) { break; }                                                               // quit

    else if (strcmp(name.source, "s") == 0) { self_comp(); strcpy(name.old_source, name.source); continue; }    // self-compile

    else if (strcmp(name.source, "h") == 0) {                                                                   // help
        printf("Commands:\n  q - quit   r - repeat last command    s - self-compile    h - help\n");
        strcpy(name.old_source, name.source); continue; }
    else if (strcmp(name.source, "ls") == 0) {                                                                  // list files
        pid.ls = fork();
        if (pid.ls == 0) { execvp("ls", (char *[]){"ls", NULL}); exit(1); }
        wait(NULL);
        strcpy(name.old_source, name.source); continue; }

    // main

    strcpy(name.old_source, name.source);                                                                       // save input
    printf("Compiling %s...\n", name.source);                                                                   // notification

    sprintf(name.bin, "./%s.bin", name.source);                                                                 // compile
    pid.compile = fork();                                                                                       // |
    if (pid.compile == 0) { execvp("gcc", (char *[]){"gcc", name.source, "-o", name.bin, NULL}); exit(1); }         // ^

    else {                                                                                                      // run
        int status;                                                                                             // |
        waitpid(pid.compile, &status, 0);                                                                       // |    wait for compilation
        if (WIFEXITED(status)) {                                                                                // |    check if compilation succeeded
            int code = WEXITSTATUS(status);                                                                     // |
            if (!code) run();                                                                                   // |    run if compilation succeeded
            else printf("Compilation failed with code %d.\n", code);                                            // ^    print error if compilation failed
}}}}
