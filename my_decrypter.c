#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include "mta_crypt.h"
#include "mta_rand.h"

#define PIPE_DIR "/mnt/mta/"
#define ENCRYPTER_PIPE "/mnt/mta/server_pipe"
#define LOG_FILE "/var/log/mtacrypt.log"
#define MAX_MSG 1024
#define MAX_PIPE_NAME 256

int client_id = 1;
FILE* log_out = NULL;

long get_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void print_str(FILE* out, const char* buf, unsigned int len) {
    for (unsigned int i = 0; i < len; ++i)
        fprintf(out, "%c", isprint((unsigned char)buf[i]) ? buf[i] : '.');
}

int is_printable_str(const char* buf, unsigned int len) {
    for (unsigned int i = 0; i < len; ++i)
        if (!isprint((unsigned char)buf[i]))
            return 0;
    return 1;
}

void log_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(log_out, format, args);
    fflush(log_out);
    va_end(args);
}

int find_next_available_id() {
    for (int id = 1; id <= 32; id++) {
        char test_path[MAX_PIPE_NAME];
        snprintf(test_path, sizeof(test_path), "%sdecrypter_pipe_%d", PIPE_DIR, id);
        if (access(test_path, F_OK) != 0) {
            return id;
        }
    }
    return 1;
}

int main() {
    log_out = fopen(LOG_FILE, "w");
    if (!log_out) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        log_printf("[DECRYPTER] Failed to initialize crypto library!\n");
        exit(EXIT_FAILURE);
    }

    client_id = find_next_available_id();

    char fifo_name[MAX_PIPE_NAME], fifo_path[MAX_PIPE_NAME * 2];
    snprintf(fifo_name, sizeof(fifo_name), "decrypter_pipe_%d", client_id);
    snprintf(fifo_path, sizeof(fifo_path), "%s%s", PIPE_DIR, fifo_name);

    if (mkfifo(fifo_path, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo fifo_path");
        exit(EXIT_FAILURE);
    }

    // Register to server
    while (1) {
        int reg_fd = open(ENCRYPTER_PIPE, O_WRONLY | O_NONBLOCK);
        if (reg_fd >= 0) {
            char reg_msg[MAX_PIPE_NAME + 16];
            snprintf(reg_msg, sizeof(reg_msg), "SUBSCRIBE:%s\n", fifo_name);
            if (write(reg_fd, reg_msg, strlen(reg_msg)) > 0) {
                close(reg_fd);
                log_printf("%ld  [DECRYPTER #%d]  [INFO] Subscribed to encryption server\n", get_timestamp(), client_id);
                break;
            }
            close(reg_fd);
        }
        usleep(100000);
    }

    int fd = open(fifo_path, O_RDONLY);
    if (fd < 0) {
        log_printf("%ld  [DECRYPTER #%d]  [ERROR] Failed to open %s for reading: %s\n", get_timestamp(), client_id, fifo_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    char encrypted_data[MAX_MSG];
    unsigned int pwd_size = 0;
    unsigned int key_size = 0;
    unsigned long brute_attempts = 0;
    int ready_to_decrypt = 0;
    int first_entry = 1;

    while (1) {
        if (!ready_to_decrypt) {
            ssize_t n = read(fd, encrypted_data, sizeof(encrypted_data));
            if (n <= 0) {
                usleep(100000);
                continue;
            }
            pwd_size = n;
            key_size = pwd_size / 8;
            brute_attempts = 0;
            ready_to_decrypt = 1;

            if (first_entry) {
                log_printf("%ld  [DECRYPTER #%d]  [INFO] Received encrypted password %.*s\n", get_timestamp(), client_id, pwd_size, encrypted_data);
                first_entry = 0;
            } else {
                log_printf("%ld  [DECRYPTER #%d]  [INFO] Received new encrypted password %.*s\n", get_timestamp(), client_id, pwd_size, encrypted_data);
            }
        }

        while (ready_to_decrypt) {
            brute_attempts++;
            char* attempt_key = malloc(key_size);
            char* trial_result = malloc(pwd_size);
            unsigned int decrypted_len = 0;

            MTA_get_rand_data(attempt_key, key_size);

            if (MTA_decrypt(attempt_key, key_size, encrypted_data, pwd_size, trial_result, &decrypted_len) == MTA_CRYPT_RET_OK) {
                if (decrypted_len == pwd_size && is_printable_str(trial_result, decrypted_len)) {
                    log_printf("%ld  [DECRYPTER #%d]  [INFO] Decrypted password: ", get_timestamp(), client_id);
                    print_str(log_out, trial_result, decrypted_len);
                    log_printf(", Key: ");
                    print_str(log_out, attempt_key, key_size);
                    log_printf(" (in %lu attempts)\n", brute_attempts);

                    int sol_fd = open(ENCRYPTER_PIPE, O_WRONLY | O_NONBLOCK);
                    if (sol_fd >= 0) {
                        char solution_msg[MAX_MSG + 32];
                        int len = snprintf(solution_msg, sizeof(solution_msg), "SOLUTION:%d:", client_id);
                        memcpy(solution_msg + strlen(solution_msg), trial_result, decrypted_len);
                        len = strlen(solution_msg) + decrypted_len;
                        solution_msg[len++] = '\n';
                        write(sol_fd, solution_msg, len);
                        close(sol_fd);
                    }
                    ready_to_decrypt = 0;
                    break;
                }
            }
            free(attempt_key);
            free(trial_result);

            if (brute_attempts % 1000 == 0) {
                char fresh_data[MAX_MSG];
                ssize_t new_n = read(fd, fresh_data, sizeof(fresh_data));
                if (new_n > 0 && (new_n != pwd_size || memcmp(fresh_data, encrypted_data, pwd_size) != 0)) {
                    memcpy(encrypted_data, fresh_data, new_n);
                    pwd_size = new_n;
                    key_size = pwd_size / 8;
                    brute_attempts = 0;
                    log_printf("%ld  [DECRYPTER #%d]  [INFO] Received new encrypted password %.*s\n", get_timestamp(), client_id, pwd_size, encrypted_data);
                    break;
                }
            }
        }
    }

    close(fd);
    fclose(log_out);
    return 0;
}
