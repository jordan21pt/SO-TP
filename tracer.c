#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

int criaPipes(int fd[][2], int n)
{
    for (int i = 0; i < n; i++)
    {
        if (pipe(fd[i]) == -1)
            return -1;
    }
    return 0;
}

void closePipes(int pipes[][2], int n)
{
    for (int i = 0; i < n; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

int splitString(char *str, char **result)
{
    int i = 0;
    char *token = strtok(str, " ");
    while (token != NULL)
    {
        result[i++] = token;
        token = strtok(NULL, " ");
    }
    result[i] = NULL;
    return i;
}css

int main(int argc, char *argv[])
{
    struct timeval tempo_inicial, tempo_final, duracao, tempo_status;
    int tempo_inicial_server;

    char pipeEscrita[50];
    char pipeLeitura[50];
    char error[50];
    char tempo[50];
    int tamanho = 0;
    for (int i = 1; i < argc; i++)
    {
        tamanho += strlen(argv[i]) + 1;
    }

    int server_cliente = open("fifo", O_WRONLY);
    if (server_cliente == -1)
    {
        perror("O cliente não consegui abrir o fifo\n");
    }

    int pid = getpid();

    if (write(server_cliente, &pid, sizeof(int)) == -1)
    {
        perror("O cliente não conseguiu enviar o pid para o servidor.\n");
    }

    // criar os dois pipes de leitura e escrita do lado do cliente
    sprintf(pipeLeitura, "pipe%dLeitura", pid);
    sprintf(pipeEscrita, "pipe%dEscrita", pid);

    mkfifo(pipeEscrita, 0666);
    mkfifo(pipeLeitura, 0666);

    int escrever = open(pipeEscrita, O_WRONLY);
    if (escrever == -1)
    {
        perror("O cliente não consegui criar o pipe pessoal de escrita!");
    }
    int leitura = open(pipeLeitura, O_RDONLY);
    if (leitura == -1)
    {
        perror("O cliente não consegui criar o pipe pessoal de leitura!");
    }

    if (strcmp(argv[1], "execute") == 0)
    {
        if (strcmp(argv[2], "-u") == 0)
        {
            int id = fork();
            if (id == 0)
            { // processo filho
                int pidFilho = getpid();
                char pidString[20];
                char info[50];
                sprintf(pidString, "Running PID %d\n", pidFilho);
                if (write(1, pidString, strlen(pidString)) == -1)
                {
                    perror("Não consegui escrever o pid do processo!\n");
                }
                int num_args = argc - 3;
                char **new_argv = (char **)malloc(sizeof(char *) * (num_args + 1));
                for (int i = 3; i < argc; i++)
                {
                    new_argv[i - 3] = argv[i];
                }
                new_argv[num_args] = NULL;
                gettimeofday(&tempo_inicial, NULL);
                sprintf(info, "%d %s %d", pidFilho, argv[3], tempo_inicial.tv_usec);
                write(escrever, info, sizeof(info));
                execvp(argv[3], new_argv);
                perror("execvp falhou!");
                return 1;
            }

            wait(NULL);
            gettimeofday(&tempo_final, NULL);

            if (read(leitura, &tempo_inicial_server, sizeof(int)) == -1)
            {
                perror("Não consegui ler do servidor!");
            }
            tempo_inicial.tv_usec = tempo_inicial_server;
            timersub(&tempo_final, &tempo_inicial, &duracao);
            sprintf(tempo, "\nEnded in %d ms\n", duracao.tv_usec / 1000);
            write(1, tempo, sizeof(tempo));
        }
        else if (strcmp(argv[2], "-p") == 0)
        {
            char pidString[20];
            char info[250];
            int tamanho = strlen(argv[3]);
            char comando[tamanho];
            strcpy(comando, argv[3]);

            int nPipes = 0;
            for (int i = 0; i < strlen(comando); i++)
            {
                if (comando[i] == '|')
                    nPipes++;
            }

            int nComandos = nPipes + 1;

            char variosComandos[nComandos][50];
            char *token = strtok(comando, "|");
            int i = 0;
            while (token != NULL && i < nComandos)
            {
                strncpy(variosComandos[i], token, 50);
                variosComandos[i][50 - 1] = '\0';
                token = strtok(NULL, "|");
                i++;
            }

            if (nComandos == 2)
            {

                int fd[1][2];
                if (criaPipes(fd, 1) == -1)
                {
                    return 1;
                }
                if (fork() == 0)
                {
                    int pidFilho = getpid();
                    sprintf(pidString, "Running PID %d\n", pidFilho);
                    if (write(1, pidString, strlen(pidString)) == -1)
                    {
                        perror("Não consegui escrever o pid do processo!\n");
                    }
                    char *result[20];
                    int n = splitString(variosComandos[0], result);
                    dup2(fd[0][1], STDOUT_FILENO);
                    closePipes(fd, nComandos - 1);
                    gettimeofday(&tempo_inicial, NULL);
                    sprintf(info, "%d %s %d", pidFilho, argv[3], tempo_inicial.tv_usec);
                    write(escrever, info, sizeof(info));
                    execvp(result[0], result); // execlp("cat", "cat", "lusiadas", NULL);
                    perror("falhou primeiro!");
                }
                if (fork() == 0)
                {
                    char *result[20];
                    int n = splitString(variosComandos[1], result);
                    dup2(fd[0][0], STDIN_FILENO);
                    closePipes(fd, nComandos - 1);
                    execvp(result[0], result);
                    perror("falhou ultimo!");
                }

                closePipes(fd, nComandos - 1);
                wait(NULL);
                gettimeofday(&tempo_final, NULL);

                if (read(leitura, &tempo_inicial_server, sizeof(int)) == -1)
                {
                    perror("Não consegui ler do servidor!");
                }
                tempo_inicial.tv_usec = tempo_inicial_server;
                timersub(&tempo_final, &tempo_inicial, &duracao);
                sprintf(tempo, "\nEnded in %d ms\n", duracao.tv_usec / 1000);
                write(1, tempo, sizeof(tempo));
            }

            else // N programas
            {

                int fd[nComandos - 1][2];
                if (criaPipes(fd, nComandos - 1) == -1)
                {
                    return 1;
                }

                if (fork() == 0)
                {
                    int pidFilho = getpid();
                    sprintf(pidString, "Running PID %d\n", pidFilho);
                    if (write(1, pidString, strlen(pidString)) == -1)
                    {
                        perror("Não consegui escrever o pid do processo!\n");
                    }
                    char *result[20];
                    int n = splitString(variosComandos[0], result);
                    dup2(fd[0][1], STDOUT_FILENO);
                    closePipes(fd, nComandos - 1);
                    gettimeofday(&tempo_inicial, NULL);
                    sprintf(info, "%d %s %d", pidFilho, argv[3], tempo_inicial.tv_usec);
                    write(escrever, info, sizeof(info));
                    execvp(result[0], result);
                    perror("falhou primeiro!");
                }

                for (int i = 1; i < nComandos - 1; i++)
                {
                    char *result[20];
                    int n = splitString(variosComandos[i], result);
                    if (fork() == 0)
                    {
                        char perro[20];
                        sprintf(perro, "falhou %d", i);
                        dup2(fd[i - 1][0], STDIN_FILENO);
                        dup2(fd[i][1], STDOUT_FILENO);
                        closePipes(fd, nComandos - 1);
                        execvp(result[0], result);
                        perror(perro);
                    }
                }

                if (fork() == 0)
                {
                    char *result[20];
                    int n = splitString(variosComandos[nComandos - 1], result);
                    dup2(fd[nComandos - 2][0], STDIN_FILENO);
                    closePipes(fd, nComandos - 1);
                    execvp(result[0], result);
                    perror("falhou ultimo!");
                }

                closePipes(fd, nComandos - 1);
                for (int i = 0; i < nComandos; i++)
                {
                    wait(NULL);
                }
                gettimeofday(&tempo_final, NULL);

                if (read(leitura, &tempo_inicial_server, sizeof(int)) == -1)
                {
                    perror("Não consegui ler do servidor!");
                }
                tempo_inicial.tv_usec = tempo_inicial_server;
                timersub(&tempo_final, &tempo_inicial, &duracao);
                sprintf(tempo, "\nEnded in %d ms\n", duracao.tv_usec / 1000);
                write(1, tempo, sizeof(tempo));
            }
        }

        else
        {
            sprintf(error, "'%s': is not a valid flag for '%s' command!\n", argv[2], argv[1]);
            perror(error);
        }
    }
    /*
    else if (strcmp(argv[1], "status") == 0)
    {
        printf("A implementar...!\n");
        gettimeofday(&tempo_status, NULL);
        char info[50];
        sprintf(info, "%s %d", argv[1], tempo_status.tv_usec);
        write(escrever, info, sizeof(info));
    }
    */
    else
    {
        sprintf(error, "'%s': is not a command!\n", argv[1]);
        perror(error);
    }

    while (wait(NULL) != -1 || errno != ECHILD)
        ;

    close(escrever);
    close(leitura);
    unlink(pipeEscrita);
    unlink(pipeLeitura);
    return 0;
}