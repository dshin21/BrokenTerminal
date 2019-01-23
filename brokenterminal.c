/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: mainwindow.cpp - A windows program that performs TCP/IP lookup functions.
--
-- PROGRAM:     WinsockApp
--
-- FUNCTIONS:
--              void host_to_ip()
--              void ip_to_host()
--              void port_to_service()
--              void service_to_port()
--              void cleanup()
--              bool validate_user_input(QString)
--              void print_to_console(QString)
--
-- DATE:        Jan. 16, 2019
--
-- REVISIONS:   None
--
-- DESIGNER:    Daniel Shin
--
-- PROGRAMMER:  Daniel Shin
--
-- NOTES:
--              This program performs four TCP/IP lookup functions using the Winsock2 API:
--                  - take a user specified host name and resolve it into a IP address
--                  - take a user specified IP address and resolve it into host name(s)
--                  - take a user specified service name/protocol and resolve it into its port number
--                  - take a user specified port number/protocol and resolve it into its service name
--               IMPORTANT NOTE: Please add "WS2_32.Lib" to the project source list.
----------------------------------------------------------------------------------------------------------------------*/
#include "brokenterminal.h"

pid_t PID[2];

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
-- FUNCTION:    host_to_ip
--
-- DATE:        Jan. 16, 2019
--
-- DESIGNER:    Daniel Shin
--
-- PROGRAMMER:  Daniel Shin
--
-- INTERFACE:   void host_to_ip()
--
-- RETURNS:     void
--
-- NOTES:
--              This function is responsible for using the Winsock v2.2 session and searching
--              to see if the IP address exists, given an host name.
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

void translate_helper(char *in, char *out)
{
    int i, j;

    for (i = 0, j = 0; i < strlen(in); ++i)
    {
        switch (in[i])
        {
        case 'K':
            j = 0;
            break;
        case 'X':
            --j;
            break;
        case 'a':
            out[j++] = 'z';
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