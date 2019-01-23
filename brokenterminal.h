#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#define CTRL_K 11
#define BUFFER_SIZE 1024

void input(int *translatePipe, int *outputPipe);

void output(int *inputPipe, int *translatePipe);

void translate(int *inputPipe, int *outputPipe);

void translate_helper(char *in, char *out);