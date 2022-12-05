
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

struct lock {
    pthread_mutex_t lock;
    int padding[6];
};

pthread_mutex_t lock;
int main() {
    int i = 0;
    int j = 0;
    char *board1 = malloc(1024 * 1024);
    char *board2 = malloc(1024 * 1024);
    printf("%d\n", sizeof(struct lock));
    pthread_mutex_t *locks = malloc(1024 * 1024 * sizeof(pthread_mutex_t));
    for (i = 0; i < 1024; i++) {
        for (j = 0; j < 1024; j++) {
            pthread_mutex_init(&locks[i * 1024 + j], NULL);
            pthread_mutex_lock(&locks[i * 1024 + j]);
            board1[0] = 10;
            pthread_mutex_unlock(&locks[i * 1024 + j]);
        }
    }
    return 0;
}