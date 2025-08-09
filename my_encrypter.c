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

#define ENCRYPTER_PIPE "/mnt/mta/server_pipe"
#define PIPE_DIR "/mnt/mta/"
#define CONF_FILE "/mnt/mta/mtacrypt.conf"
#define LOG_FILE "/var/log/mtacrypt.log"
#define MAX_DECRYPTERS 32
#define MAX_MSG 1024
#define MAX_PIPE_NAME 512

typedef struct {
    int id;
    int active;
    char pipe_name[MAX_PIPE_NAME];
} dec_client_t;

dec_client_t dec_clients[MAX_DECRYPTERS];
int dec_client_count = 0;
unsigned int pwd_size = 24;
FILE* log_output = NULL;

long get_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void print_str(FILE* out, const char* buf, unsigned int len) {
    for (unsigned int i = 0; i < len; ++i)
        fprintf(out, "%c", isprint((unsigned char)buf[i]) ? buf[i] : '.');
}

void log_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(log_output, format, args);
    fflush(log_output);
    va_end(args);
}

void generate_random_printable(char* buf, unsigned int len) {
    for (unsigned int i = 0; i < len; ++i) {
        char c;
        do {
            c = MTA_get_rand_char();
        } while (!isprint((unsigned char)c));
        buf[i] = c;
    }
}

void read_config() {
    log_printf("Loading configuration file...\n");
    FILE* f = fopen(CONF_FILE, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "PASSWORD_LENGTH=", 16) == 0) {
                pwd_size = atoi(line + 16);
                log_printf("Password length set to %u\n", pwd_size);
            }
        }
        fclose(f);
    } else {
        log_printf("[ENCRYPTION SERVER][ERROR] Could not open config file %s: %s\n", CONF_FILE, strerror(errno));
    }
}

int register_decrypter(const char* pipe_name) {
    for (int i = 0; i < dec_client_count; i++) {
        if (strcmp(dec_clients[i].pipe_name, pipe_name) == 0) {
            return dec_clients[i].id;
        }
    }
    if (dec_client_count >= MAX_DECRYPTERS) return -1;
    int new_id = dec_client_count + 1;
    snprintf(dec_clients[dec_client_count].pipe_name, sizeof(dec_clients[dec_client_count].pipe_name), "%s", pipe_name);
    dec_clients[dec_client_count].id = new_id;
    dec_clients[dec_client_count].active = 1;
    log_printf("%ld  [ENCRYPTION SERVER]  [INFO] Decrypter registration: id %d, fifo %s%s\n",
        get_timestamp(), new_id, PIPE_DIR, pipe_name);
    dec_client_count++;
    return new_id;
}

void send_password_to_decrypter(int idx, const char* cipher_buf, unsigned int cipher_len) {
    char path[1024];
    snprintf(path, sizeof(path), "%s%s", PIPE_DIR, dec_clients[idx].pipe_name);
    int fd = open(path, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        log_printf("%ld  [ENCRYPTION SERVER]  [ERROR] Failed to open %s for writing: %s\n", get_timestamp(), path, strerror(errno));
        return;
    }
    ssize_t written = write(fd, cipher_buf, cipher_len);
    if (written < 0) {
        log_printf("%ld  [ENCRYPTION SERVER]  [ERROR] Failed to write to %s: %s\n", get_timestamp(), path, strerror(errno));
    }
    close(fd);
}

void broadcast_password(const char* cipher_buf, unsigned int cipher_len) {
    for (int i = 0; i < dec_client_count; i++) {
        if (!dec_clients[i].active) continue;
        send_password_to_decrypter(i, cipher_buf, cipher_len);
    }
}

int main() {
    log_output = fopen(LOG_FILE, "w");
    if (!log_output) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    read_config();

    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        log_printf("[ENCRYPTION SERVER] Failed to initialize crypto library!\n");
        exit(EXIT_FAILURE);
    }

    umask(0);
    unlink(ENCRYPTER_PIPE);
    if (mkfifo(ENCRYPTER_PIPE, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    chmod(ENCRYPTER_PIPE, 0666);

    int reg_fd = open(ENCRYPTER_PIPE, O_RDONLY | O_NONBLOCK);
    if (reg_fd < 0) {
        perror("open server_pipe");
        exit(EXIT_FAILURE);
    }

    char* pwd_buf = NULL;
    char* cipher_buf = NULL;
    unsigned int cipher_len = 0;
    char* key_buf = NULL;
    unsigned int key_size = 0;
    int is_first = 1;

    while (1) {
        if (!pwd_buf) {
            key_size = pwd_size / 8;
            pwd_buf = malloc(pwd_size);
            key_buf = malloc(key_size);
            cipher_buf = malloc(pwd_size);

            generate_random_printable(pwd_buf, pwd_size);
            MTA_get_rand_data(key_buf, key_size);

            if (MTA_encrypt(key_buf, key_size, pwd_buf, pwd_size, cipher_buf, &cipher_len) != MTA_CRYPT_RET_OK) {
                log_printf("%ld  [ENCRYPTION SERVER]  [ERROR] Encryption failed\n", get_timestamp());
                free(pwd_buf); free(key_buf); free(cipher_buf);
                pwd_buf = NULL; cipher_buf = NULL; key_buf = NULL;
                continue;
            }

            if (is_first) {
                log_printf("%ld  [ENCRYPTION SERVER]  [INFO] New password generated: ");
                print_str(log_output, pwd_buf, pwd_size);
                log_printf(", key: ");
                print_str(log_output, key_buf, key_size);
                fprintf(log_output, ", After encryption: %.*s", cipher_len, cipher_buf);
                log_printf("\nListening on /mnt/mta/server_pipe\n");
                is_first = 0;
            } else {
                log_printf("%ld  [ENCRYPTION SERVER]  [INFO] New password: ");
                print_str(log_output, pwd_buf, pwd_size);
                log_printf(", key: ");
                print_str(log_output, key_buf, key_size);
                fprintf(log_output, ", Encrypted: %.*s", cipher_len, cipher_buf);
                log_printf("\n");
            }

            broadcast_password(cipher_buf, cipher_len);
        }

        char buf[2048];
        ssize_t n = read(reg_fd, buf, sizeof(buf) - 1);
        if (n == 0) {
            close(reg_fd);
            reg_fd = open(ENCRYPTER_PIPE, O_RDONLY | O_NONBLOCK);
        } else if (n > 0) {
            buf[n] = '\0';
            char* newline = strchr(buf, '\n');
            if (newline) *newline = '\0';

            if (strncmp(buf, "SUBSCRIBE:", 10) == 0) {
                char* pipe_name = buf + 10;
                int id = register_decrypter(pipe_name);
                if (id > 0 && cipher_buf) {
                    send_password_to_decrypter(id - 1, cipher_buf, cipher_len);
                }
            } else if (strncmp(buf, "SOLUTION:", 9) == 0) {
                char* solution_data = buf + 9;
                char* colon = strchr(solution_data, ':');
                if (colon) {
                    int client_id = atoi(solution_data);
                    char* guess = colon + 1;
                    if (strlen(guess) == pwd_size && pwd_buf &&
                        memcmp(guess, pwd_buf, pwd_size) == 0) {
                        log_printf("%ld  [ENCRYPTION SERVER]  [OK] Password decrypted successfully by decrypter #%d\n",
                            get_timestamp(), client_id);

                        free(pwd_buf); free(cipher_buf); free(key_buf);
                        pwd_buf = NULL; cipher_buf = NULL; key_buf = NULL;
                    }
                }
            }
        }

        usleep(100000);
    }

    fclose(log_output);
    return 0;
}
