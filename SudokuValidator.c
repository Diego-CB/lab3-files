#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sys/wait.h>
#include <omp.h>

int sudoku_array[9][9];

int check_column(int i)
{
    // Se recorre la columna 9 veces (buscando cada uno de los numeros)
    int n, j;
    for (n = 1; n < 10; n++)
    {
        // Se busca cada numero i en la columna
        int found_i = 0;
        omp_set_num_threads(9);
        #pragma parallel for private(found_i, sudoku_array, n) //schedule(dynamic)
        for (j = 0; j < 9; j++)
        {
            if (sudoku_array[j][i] == n)
            {
                found_i = 1;
            }
        }

        // Si no se encuentra i, se devuelve 0
        if (found_i == 0)
        {
            return 0;
        }
    }
    
    // Si se encuentran todos los numeros en las columnas, se devuelve 1
    return 1;
}

int check_row()
{
    // Se recorre cada fila 9 veces (buscando cada uno de los numeros)
    int n, i, j;
    for (i = 0; i < 9; i++)
    {
        for (n = 1; n < 10; n++)
        {
            // Se busca cada numero i en la columna
            int found_i = 0;
            for (j = 0; j < 9; j++)
            {
                if (sudoku_array[i][j] == n)
                {
                    found_i = 1;
                }
            }

            // Si no se encuentra i, se devuelve 0
            if (found_i == 0)
            {
                return 0;
            }
        }
    }
    
    // Si se encuentran todos los numeros en las columnas, se devuelve 1
    return 1;
}

int check_square(int start_i, int start_j)
{
    // Se recorre el subarreglo 9 veces (buscando cada uno de los numeros)
    int n, i, j;
    for (i = start_i; i < start_i + 3; i++)
    {
        // Se busca cada numero n en el subarreglo
        int found_i = 0;
        for (n = 1; n < 10; n++)
        {
            omp_set_num_threads(9);
            #pragma parallel for private(found_i, sudoku_array, n) //schedule(dynamic)
            for (j = start_j; j < start_j + 3; j++)
            {
                if (sudoku_array[i][j] == n)
                {
                    found_i = 1;
                }
            }
        }

        // Si no se encuentra n, se devuelve 0
        if (found_i == 0)
        {
            return 0;
        }
    }
    
    // Si se encuentran todos los numeros en el subarreglo, se devuelve 1
    return 1;
}

void* check_column_subrutine()
{
    int i, result_i;
    int global_result = 1;
    for (i = 0; i < 9; i++)
    {
        result_i = check_column(i);
        global_result = global_result * result_i;
    }
    
    pthread_exit(&global_result);
}

int main(int argc, char* argv[])
{
    omp_set_num_threads(1);
    // Task 1: Copiar archivo a amtriz
    FILE* stream;
    stream = fopen(argv[1], "r");

    int i, j, n = 0;
    for (n = 0; n < 81; n++)
    {
        sudoku_array[i][j] = (int) fgetc(stream);

        if (j == 9)
        {
            j = 0;
            i++;
        }
        else
        {
            j++;
        }
    }

    munmap(stream, sizeof(char)*81);
    fclose(stream);

    int result_squares = 1;

    omp_set_num_threads(9);
    #pragma omp parallel for private(result_squares)//schedule(dynamic)
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            result_squares = result_squares * check_square(i*3, j*3);
        }
    }
    
    // Task 2
    int pid_int = getpid();
    pid_t pid = fork();

    int* column_result;
    // Proceso Padre
    if (pid != 0)
    {
        pthread_t id[2];
        pthread_create(&id[0], NULL, check_column_subrutine, NULL);
        pthread_join(id[0], (void**) &column_result);
        printf("Thread en Revision de Columnas: %ld\n", syscall(SYS_gettid));
        wait(NULL);
    }
    else // proceso hijo
    {
        char pid_str[3];
        sprintf(pid_str, "%d", pid_int);
        
        execlp("ps", "ps", "-p", pid_str, "-lLf", NULL);
        exit(0);
    }

    int row_result = check_row();

    if (
        (row_result == 1)
        && ((long) column_result == 1)        
        && (result_squares == 1)
    ){
        printf("EL resultado del sudoku es valido\n");
    }
    else
    {
        printf("EL resultado del sudoku no es valido\n");
    }

    int pid_int1 = getpid();
    pid = fork();
    // Proceso hijo
    if (pid == 0)
    {
        char pid_str1[3];
        sprintf(pid_str1, "%d", pid_int1);
        printf("Estado del proceso antes de terminar\n");
        execlp("ps", "ps", "-p", pid_str1, "-lLf", NULL);
        exit(0);
    }
    else{
        wait(NULL);
    }

    return 0;
}