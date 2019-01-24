/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: brokenterminal.c - a program that re-programs a "raw mode" terminal
--
-- PROGRAM:     brokenterminal
--
-- FUNCTIONS:
--              int main(void)
--              void input(int *p_translate, int *p_output)
--              void output(int *p_input, int *p_translate)
--              void translate(int *p_input, int *p_output)
--              void translate_helper(char *in, char *out)
--
-- DATE:        Jan. 18, 2019
--
-- REVISIONS:   None
--
-- DESIGNER:    Daniel Shin
--
-- PROGRAMMER:  Daniel Shin
--
-- NOTES:
--              This program re-programs the terminal by putting it into "raw mode" and rewrites the commands:
--                  - a -> z
--                  - E -> enter
--                  - T -> normal termination
--                  - ctrl + k -> abnormal termination
--                  - X -> backspace
--                  - K -> line kill
----------------------------------------------------------------------------------------------------------------------*/
#include "brokenterminal.h"

pid_t PID[2];

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    main
--
-- DATE:        Jan. 18, 2019
--
-- DESIGNER:    Daniel Shin
--
-- PROGRAMMER:  Daniel Shin
--
-- INTERFACE:   int main(void)
--
-- RETURNS:     int; 0 for successful termination, 1 for otherwise.
--
-- NOTES:
--              Entry point to the program. Initializes all pipes and processes needed for the program.
--          	Creates 3 pipes: input -> output, input -> translate, translate -> output.
--              Input -> output and translate -> output are set to non-blocking mode.
--              This function also disables and re-enables the "raw mode".
----------------------------------------------------------------------------------------------------------------------*/
int main(void)
{
    int pipes[3][2], i;

    for (i = 0; i < 3; ++i)
    {
        if (pipe(pipes[i]) < 0)
        {
            printf("\r\nOpening pipe failed\r\n");
            return 1;
        }
    }

    if (fcntl(pipes[1][0], F_SETFL, O_NDELAY) < 0 || fcntl(pipes[2][0], F_SETFL, O_NDELAY) < 0)
    {
        fprintf(stderr, "\r\nError setting flag on pipe\r\n");
        return 1;
    }

    system("stty raw igncr -echo");

    for (i = 0; i < 2; ++i)
    {
        PID[i] = fork();
        if (PID[i] == 0)
            break;
    }

    switch (i)
    {
    case 0:
        output(pipes[1], pipes[2]);
        break;
    case 1:
        translate(pipes[0], pipes[2]);
        break;
    case 2:
        input(pipes[0], pipes[1]);

        kill(PID[0], SIGKILL);
        kill(PID[1], SIGKILL);

        for (i = 0; i < 3; i++)
        {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        break;
    default:
        printf("\r\nError during forking\n");
        break;
    }

    system("stty -raw -igncr echo");

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    input
--
-- DATE:        Jan. 18, 2019
--
-- DESIGNER:    Daniel Shin
--
-- PROGRAMMER:  Daniel Shin
--
-- INTERFACE:   void input(int *p_translate, int *p_output)
--                  int *p_translate, a pointer to the translate pipe
--                  int *p_output, a pointer to the output pipe
--
-- RETURNS:     void
--
-- NOTES:
--              This function handles the input of the program. Receives the input from the user and stores
--              the inputted characters to the buffer to be translated and displayed to the console.
----------------------------------------------------------------------------------------------------------------------*/
void input(int *p_translate, int *p_output)
{
    const char R = '\r', N = '\n';
    char c, buffer[BUFFER_SIZE];
    int i = 0;

    fflush(stdout);

    while ((c = getchar()))
    {
        switch (c)
        {
        case CTRL_K:
            kill(0, SIGQUIT);
            break;
        case 'E':
            write(p_output[1], &R, 1);
            write(p_output[1], &N, 1);
            write(p_translate[1], buffer, strlen(buffer));
            memset(buffer, 0, i);
            i = 0;
            break;
        case 'T':
            write(p_output[1], &R, 1);
            write(p_output[1], &N, 1);
            write(p_translate[1], buffer, strlen(buffer));
            kill(getpid(), SIGTERM);
            break;
        default:
            write(p_output[1], &c, 1);
            if (i < BUFFER_SIZE - 1)
            {
                buffer[i++] = c;
            }
            else
            {
                printf("\r\nPlease keep it under 1024!\r\n");
                break;
            }
            break;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    output
--
-- DATE:        Jan. 18, 2019
--
-- DESIGNER:    Daniel Shin
--
-- PROGRAMMER:  Daniel Shin
--
-- INTERFACE:   void output(int *p_input, int *p_translate)
--                  int *p_input, a pointer to the input pipe
--                  int *p_translate, a pointer to the translate pipe
--
-- RETURNS:     void
--
-- NOTES:
--              This function handles the outputs of the program. Constantly checks the input and translate pipe
--              for data and displays the data from the pipes.
----------------------------------------------------------------------------------------------------------------------*/
void output(int *p_input, int *p_translate)
{
    char input_buffer, translate_buffer[BUFFER_SIZE];

    while (1)
    {
        if (read(p_input[0], &input_buffer, 1) > 0)
        {
            printf("%c", input_buffer);
            input_buffer = '\0';
        }
        if (read(p_translate[0], translate_buffer, BUFFER_SIZE) > 0)
        {
            printf("%s", translate_buffer);
            memset(translate_buffer, 0, BUFFER_SIZE);
        }
        fflush(stdout);
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    translate
--
-- DATE:        Jan. 18, 2019
--
-- DESIGNER:    Daniel Shin
--
-- PROGRAMMER:  Daniel Shin
--
-- INTERFACE:   void translate(int *p_input, int *p_output)
--                  int *p_input, a pointer to the input pipe
--                  int *p_output, a pointer to the output pipe
--
-- RETURNS:     void
--
-- NOTES:
--              This function handles the translation of the inputted characters.
--              Constantly reads the input pipe for data, translates and then clears the buffer.
----------------------------------------------------------------------------------------------------------------------*/
void translate(int *p_input, int *p_output)
{
    char buffer[BUFFER_SIZE], result[BUFFER_SIZE];

    while (read(p_input[0], buffer, BUFFER_SIZE))
    {
        translate_helper(buffer, result);
        memset(buffer, 0, BUFFER_SIZE);
        write(p_output[1], result, strlen(result));
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    translate_helper
--
-- DATE:        Jan. 18, 2019
--
-- DESIGNER:    Daniel Shin
--
-- PROGRAMMER:  Daniel Shin
--
-- INTERFACE:   void translate_helper(char *in, char *out)
--                  int *in, a pointer to the input pipe
--                  int *out, a pointer to the output pipe
--
-- RETURNS:     void
--
-- NOTES:
--              This function handles the translation of the inputted characters:
--                  if 'a' is found, translates into 'z'
--                  if 'K' is found, clears the in buffer
--                  if 'X' is found, deletes the previous character
--                  otherwise, stores it into the output buffer
----------------------------------------------------------------------------------------------------------------------*/
void translate_helper(char *in, char *out)
{
    int i, j;

    for (i = 0, j = 0; i < strlen(in); ++i)
    {
        switch (in[i])
        {
        case 'a':
            out[j++] = 'z';
            break;
        case 'K':
            j = 0;
            break;
        case 'X':
            --j;
            break;
        default:
            out[j] = in[i];
            ++j;
            break;
        }
    }

    out[j] = '\r';
    out[j + 1] = '\n';
    out[j + 2] = '\0';
}