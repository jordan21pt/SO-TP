#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char const *argv[])
{
    char pe[50];
    char pl[50];
    for (int i = 0; i < 100000; i++)
    {
        sprintf(pe, "pipe%dLeitura", i);
        sprintf(pl, "pipe%dEscrita", i);
        unlink(pe);
        unlink(pl);
        // unlink("fifo");
    }
    return 0;
}
