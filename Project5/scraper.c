#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_URLS 10

// Structure to pass data to threads
typedef struct {
    int id;
    char url[256];
} ThreadData;

// Thread function
void* fetch_url(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    char command[512];
    char filename[50];

    // Create output file name
    sprintf(filename, "output_%d.txt", data->id);

    printf("Thread %d fetching: %s\n", data->id, data->url);

    // Use curl to fetch webpage
    sprintf(command, "curl -s %s -o %s", data->url, filename);

    int result = system(command);

    if (result == 0) {
        printf("Thread %d: Downloaded successfully -> %s\n", data->id, filename);
    } else {
        printf("Thread %d: Error fetching URL\n", data->id);
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[MAX_URLS];
    ThreadData data[MAX_URLS];

    int num_urls;

    printf("Enter number of URLs (max %d): ", MAX_URLS);
    scanf("%d", &num_urls);

    if (num_urls <= 0 || num_urls > MAX_URLS) {
        printf("Invalid number of URLs.\n");
        return 1;
    }

    // Input URLs
    for (int i = 0; i < num_urls; i++) {
        printf("Enter URL %d: ", i + 1);
        scanf("%s", data[i].url);
        data[i].id = i + 1;
    }

    // Create threads
    for (int i = 0; i < num_urls; i++) {
        if (pthread_create(&threads[i], NULL, fetch_url, &data[i]) != 0) {
            printf("Error creating thread %d\n", i + 1);
            return 1;
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < num_urls; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nAll downloads completed.\n");

    return 0;
}