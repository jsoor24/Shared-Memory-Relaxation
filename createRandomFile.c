#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    FILE *fw = fopen("numbers.txt", "w");

    srand(time(0));

    int n = 1000000;
    if(fprintf(fw, "%d\n", n) == EOF) {return -1;}
    for (int i = 0; i < n; i++) {
        int number = rand() % 1000;
        if(fprintf(fw, "%d\n", number) == EOF) {return -1;}
    }

    fclose(fw);
    return 0;
}