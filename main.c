#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "mta_crypt.h"
#include "mta_rand.h"

#define MAX_LEN 1024
#define MAX_WORKERS 64

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_new_task = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_result_ready = PTHREAD_COND_INITIALIZER;

char encrypted_buffer[MAX_LEN];
unsigned int encrypted_size;
char target_password[MAX_LEN];
char discovered_password[MAX_LEN];
int pwd_length = 0;
int key_length = 0;
int is_password_found = 0;
int wait_timeout = -1;
int current_round = 0;

long current_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec;
}

void generate_printable(char *buf, int len) {
    do {
        MTA_get_rand_data(buf, len);
    } while (!isprint(buf[0]));
    for (int i = 0; i < len; i++) {
        while (!isprint(buf[i])) {
            buf[i] = MTA_get_rand_char();
        }
    }
}

void log_password_info(const char *tag, char *data, int len) {
    printf("%ld\t[SERVER] [%s] Generated password: ", current_time(), tag);
    for (int i = 0; i < len; i++) putchar(isprint(data[i]) ? data[i] : '.');
    printf("\n");
}

void *password_producer(void *arg) {
    int *timeout_ptr = (int *)arg;
    wait_timeout = *timeout_ptr;

    while (1) {
        pthread_mutex_lock(&mutex);
        is_password_found = 0;
        current_round++;

        generate_printable(target_password, pwd_length);
        char key_buf[MAX_LEN];
        key_length = pwd_length / 8;
        MTA_get_rand_data(key_buf, key_length);
        MTA_encrypt(key_buf, key_length, target_password, pwd_length, encrypted_buffer, &encrypted_size);

        printf("%ld\t[SERVER] [INFO] Generated password: ", current_time());
        for (int i = 0; i < pwd_length; i++) putchar(isprint(target_password[i]) ? target_password[i] : '.');
        printf(", key: ");
        for (int i = 0; i < key_length; i++) putchar(isprint(key_buf[i]) ? key_buf[i] : '.');
        printf(", Encrypted: ");
        for (unsigned int i = 0; i < encrypted_size; i++) putchar(isprint(encrypted_buffer[i]) ? encrypted_buffer[i] : '.');
        printf("\n");

        pthread_cond_broadcast(&cond_new_task);

        if (wait_timeout > 0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += wait_timeout;
            int rc = pthread_cond_timedwait(&cond_result_ready, &mutex, &ts);
            if (!is_password_found) {
                printf("%ld\t[SERVER] [ERROR] No correct password received in %d seconds, regenerating...\n", current_time(), wait_timeout);
            }
        } else {
            while (!is_password_found) {
                pthread_cond_wait(&cond_result_ready, &mutex);
            }
        }

        if (is_password_found) {
            printf("%ld\t[SERVER] [OK] Password successfully decrypted: (", current_time());
            for (int i = 0; i < pwd_length; i++) putchar(isprint(discovered_password[i]) ? discovered_password[i] : '.');
            printf(")\n");
        }

        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

void *password_cracker(void *arg) {
    int thread_id = *(int *)arg;
    int local_round = 0;
    char local_cipher[MAX_LEN];
    unsigned int local_size;
    int trial_count;

    while (1) {
        pthread_mutex_lock(&mutex);
        while (local_round == current_round) {
            pthread_cond_wait(&cond_new_task, &mutex);
        }

        local_round = current_round;
        memcpy(local_cipher, encrypted_buffer, encrypted_size);
        local_size = encrypted_size;
        pthread_mutex_unlock(&mutex);

        trial_count = 0;
        while (local_round == current_round && !is_password_found) {
            char candidate_key[MAX_LEN];
            char candidate_result[MAX_LEN];
            unsigned int out_len;
            MTA_get_rand_data(candidate_key, key_length);
            trial_count++;

            if (MTA_decrypt(candidate_key, key_length, local_cipher, local_size, candidate_result, &out_len) == MTA_CRYPT_RET_OK) {
                int is_printable = 1;
                for (unsigned int i = 0; i < out_len; i++) {
                    if (!isprint(candidate_result[i])) {
                        is_printable = 0;
                        break;
                    }
                }

                if (is_printable) {
                    pthread_mutex_lock(&mutex);
                    printf("%ld\t[CLIENT #%d] [INFO] Decrypted result(", current_time(), thread_id);
                    for (unsigned int i = 0; i < out_len; i++) putchar(isprint(candidate_result[i]) ? candidate_result[i] : '.');
                    printf("), key tried(");
                    for (int i = 0; i < key_length; i++) putchar(isprint(candidate_key[i]) ? candidate_key[i] : '.');
                    printf("), after %d attempts\n", trial_count);

                    if (!is_password_found && memcmp(candidate_result, target_password, pwd_length) == 0) {
                        is_password_found = 1;
                        memcpy(discovered_password, candidate_result, pwd_length);
                        pthread_cond_signal(&cond_result_ready);
                    } else if (!is_password_found) {
                        printf("%ld\t[SERVER] [ERROR] Incorrect password from client #%d (", current_time(), thread_id);
                        for (unsigned int i = 0; i < out_len; i++) putchar(isprint(candidate_result[i]) ? candidate_result[i] : '.');
                        printf("), expected (");
                        for (int i = 0; i < pwd_length; i++) putchar(isprint(target_password[i]) ? target_password[i] : '.');
                        printf(")\n");
                    }
                    pthread_mutex_unlock(&mutex);
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int cracker_threads = 4;
    pwd_length = 16;
    wait_timeout = -1;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--num-of-decrypters") == 0) && i + 1 < argc) {
            cracker_threads = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--password-length") == 0) && i + 1 < argc) {
            pwd_length = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) && i + 1 < argc) {
            wait_timeout = atoi(argv[++i]);
        } else {
            fprintf(stderr, "Usage: %s [-t timeout seconds] <-n|--num-of-decrypters <number>> <-l|--password-length <length>>\n", argv[0]);
            exit(1);
        }
    }

    if (pwd_length % 8 != 0) {
        fprintf(stderr, "Password length must be a multiple of 8\n");
        exit(1);
    }

    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Failed to initialize crypto lib\n");
        exit(1);
    }

    pthread_t producer_thread;
    pthread_create(&producer_thread, NULL, password_producer, &wait_timeout);

    pthread_t worker_threads[MAX_WORKERS];
    int thread_ids[MAX_WORKERS];
    for (int i = 0; i < cracker_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&worker_threads[i], NULL, password_cracker, &thread_ids[i]);
    }

    pthread_join(producer_thread, NULL);
    for (int i = 0; i < cracker_threads; i++) {
        pthread_cancel(worker_threads[i]);
        pthread_join(worker_threads[i], NULL);
    }

    return 0;
}
