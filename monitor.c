#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define BUFFSIZE 1024

int main()
{
    char buffer[BUFFSIZE];
    mkfifo("fifo", 0666);
    int pid;
    int leituraS = open("fifo", O_RDONLY);
    int escritaS = open("fifo", O_WRONLY);
    char infoOutput[3][20];
    int argumentos = 0;

    while (read(leituraS, &pid, sizeof(int)) > 0)
    {

        if (fork() == 0)
        {
            // printf("O pid deste cliente é %d\n", pid);

            // close(escritaS); //isto esta provocar que o servidor nao termine apos a leitura do primeiro pid... tentar corrigir dps
            // close(leituraS);

            // Criacao de dois pipes unicos para cada cliente
            //  pipeEscrita serve para escrever a partir do servidor
            //  pipeLeitura serve para ler a partir do servidor
            char pipeEscrita[25];
            char pipeLeitura[25];
            sprintf(pipeEscrita, "pipe%dLeitura", pid);
            sprintf(pipeLeitura, "pipe%dEscrita", pid);

            int lerString = open(pipeLeitura, O_RDONLY);

            if (lerString == -1)
            {
                perror("Não consegui abrir o pipe pessoal de leitura!");
            }

            int escreverString = open(pipeEscrita, O_WRONLY);
            if (escreverString == -1)
            {
                perror("Não consegui abrir o pipe pessoal de escrita!");
            }

            int bytesRead = read(lerString, &buffer, BUFFSIZE);
            // printf("%d\n", bytesRead);
            buffer[bytesRead] = '\0';

            printf("buffer: %s\n", buffer);

            char *token = strtok(buffer, " ");
            while (token != NULL)
            {
                strcpy(infoOutput[argumentos], token);
                argumentos++;
                token = strtok(NULL, " ");
            }
            if (strcmp(infoOutput[0], "status") == 0)
            {
                // printf("Calcular status!\n");
            }
            else
            {
                int tempo = atoi(infoOutput[2]);
                // printf("tempo : %d\n", tempo);

                if (write(escreverString, &tempo, sizeof(tempo)) == -1)
                {
                    perror("Não consegui escrever no pipe pessoal de escrita!");
                }
            }

            close(lerString);
            close(escreverString);
            _exit(0);
        }
    }

    return 0;
}