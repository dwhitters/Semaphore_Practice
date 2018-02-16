/*******************************************************************
 * @authors David Whitters and Jonah Bukowsky
 * @date 2/22/18
 *
 * CIS 452
 * Dr. Dulimarta
 *******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <sys/sem.h>

#define SIZE 16

int main (int argc, char * argv[]) {
    int status;
    long int i, loop, temp, *shmPtr;
    int shmId;
    pid_t pid;

    /* Create a new semaphore set for use by this (and other) processes. */
    int sem_id = semget(IPC_PRIVATE, 1, 00600);
    printf("Semaphore ID: %d\n", sem_id);
    struct sembuf sem_buff_wait = { /* Uses the semaphore when it's free. */
        .sem_num = 0, /* The semaphore to operate on in the set. */
        .sem_op = -1, /* Decrement semval. */
        .sem_flg = 0  /* No flags. */
    };
    struct sembuf sem_buff_signal = { /* Free up the semaphore. */
        .sem_num = 0, /* The semaphore to operate on in the set. */
        .sem_op = 1,  /* Amount of currently running processes allowed without queuing. */
        .sem_flg = 0  /* No flags. */
    };

    /* Initialize the semaphore set referenced by the previously obtained sem_id handle. */
    if(semctl(sem_id, 0, SETVAL, 1) == -1)
    {
        perror("semctl error\n");
        exit(EXIT_FAILURE);
    }

    /* Set loop to the value specified by user if there is only one cmdline arg. */
    loop = (argc == 2) ? atoi(argv[1]) : 0;
    if(0 == loop)
    {
        printf("\nNo loop value specified! Defaulted to zero.\n\n");
    }

    if ((shmId = shmget (IPC_PRIVATE, SIZE,
                         IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror ("i can't get no..\n");
        exit (1);
    }
    if ((shmPtr = shmat (shmId, 0, 0)) == (void *) -1) {
        perror ("can't attach\n");
        exit (1);
    }

    shmPtr[0] = 0;
    shmPtr[1] = 1;

    if (!(pid = fork ())) {
        for (i = 0; i < loop; i++) {
            if(semop(sem_id, &sem_buff_wait, 1) == -1) /* Wait for the semaphore to be freed. */
            {
                perror("Semaphore wait operation in the child\n");
                exit(EXIT_FAILURE);
            }
            /* Swap the contents of shmPtr[0] and  shmPtr[1] */
            temp = shmPtr[0];
            shmPtr[0] = shmPtr[1];
            shmPtr[1] = temp;
            if(semop(sem_id, &sem_buff_signal, 1) == -1) /* Free the semaphore. */
            {
                perror("Semaphore signal operation in the child\n");
                exit(EXIT_FAILURE);
            }
        }
        if (shmdt (shmPtr) < 0) {
            perror ("just can 't let go\n");
            exit (1);
        }
        exit (0);
    }
    else {
        for (i = 0; i < loop; i++) {
            if(semop(sem_id, &sem_buff_wait, 1) == -1) /* Wait for the semaphore to be freed. */
            {
                perror("Semaphore wait operation in the parent\n");
                exit(EXIT_FAILURE);
            }
            /* Swap the contents of shmPtr[1] and shmPtr[0] */
            temp = shmPtr[1];
            shmPtr[1] = shmPtr[0];
            shmPtr[0] = temp;
            if(semop(sem_id, &sem_buff_signal, 1) == -1) /* Free the semaphore. */
            {
                perror("Semaphore signal operation in the parent\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    wait (&status);
    printf ("values: %li\t%li\n", shmPtr[0], shmPtr[1]);

    if (shmdt (shmPtr) < 0) {
        perror ("just can't let go\n");
        exit (1);
    }
    if (shmctl (shmId, IPC_RMID, 0) < 0) {
        perror ("can't deallocate\n");
        exit (1);
    }

    /* Remove the semaphore referenced by sem_id. */
    if(semctl(sem_id, 0, IPC_RMID) == -1)
    {
        perror("Error removing the semaphore.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
