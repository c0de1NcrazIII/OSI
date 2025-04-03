#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <limits.h>   

enum errors {
    SUCCESS = 0,
    ERROR_OPEN_FILE = -1,
    ERROR_MALLOC = -2,
    ERROR_FORK = -3,
    ERROR_EMPTY_SEARCH_STRING = -4,
    ERROR_USAGE = -5,

};

int xorN(const char* filename, int N) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return ERROR_OPEN_FILE;
    }

    size_t bits_requested = 1 << N;
    size_t full_bytes = bits_requested / 8;
    size_t remaining_bits = bits_requested % 8;
    size_t total_bytes = full_bytes + (remaining_bits ? 1 : 0);

    uint8_t* block = (uint8_t*)calloc(total_bytes, sizeof(uint8_t));
    if (block == NULL) {
        fclose(file);
        return ERROR_MALLOC;
    }
    uint8_t* result = (uint8_t*)calloc(total_bytes, sizeof(uint8_t));
    if (result == NULL) {
        fclose(file);
        free(block);
        return ERROR_MALLOC;
    }

    size_t bytes_read;
    while ((bytes_read = fread(block, sizeof(uint8_t), total_bytes, file)) > 0) {
        if (bytes_read < total_bytes) {
            memset(block + bytes_read, 0, total_bytes - bytes_read);
        }
        
        for (size_t i = 0; i < full_bytes; i++) {
            result[i] ^= block[i];
        }
        
        if (remaining_bits > 0 && bytes_read >= full_bytes) {
            uint8_t mask = (1 << remaining_bits) - 1;
            result[full_bytes] ^= (block[full_bytes] & mask);
        }
    }

    printf("XOR result for file '%s' with N=%d (%zu bits):\n", filename, N, bits_requested);
    for (size_t i = 0; i < full_bytes; i++) {
        printf("%x ", result[i]);
    }
    if (remaining_bits > 0) {
        printf("%x (only %zu bits)", result[full_bytes], remaining_bits);
    }
    printf("\n");

    free(block);
    free(result);
    fclose(file);
    return SUCCESS;
}

int mask(const char *filename, uint32_t mask_value) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return ERROR_OPEN_FILE;
    }

    uint32_t value;
    size_t count = 0;

    while (fread(&value, sizeof(uint32_t), 1, file)) {
        if ((value & mask_value) == mask_value) {
            count++;
        }
    }

    fclose(file);

    printf("Mask count for %s: %zu\n", filename, count);
    return SUCCESS;
}

int copyN(const char *filename, int n) {
    for (int i = 1; i <= n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char new_filename[PATH_MAX];
            snprintf(new_filename, sizeof(new_filename), "%s_%d", filename, i);

            FILE *src_file = fopen(filename, "rb");
            if (!src_file) {
                exit(EXIT_FAILURE);
            }

            FILE *dst_file = fopen(new_filename, "wb");
            if (!dst_file) {
                fclose(src_file);
                exit(EXIT_FAILURE);
            }

            uint8_t buffer[BUFSIZ];
            size_t bytes_read;

            while ((bytes_read = fread(buffer, 1, BUFSIZ, src_file))) {
                fwrite(buffer, 1, bytes_read, dst_file);
            }

            fclose(src_file);
            fclose(dst_file);

            exit(EXIT_SUCCESS);
        } else if (pid < 0) {
            return ERROR_FORK;
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    return SUCCESS;
}

int find(const char *filename, const char *search_string) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return -1;
    }

    int search_len = strlen(search_string);
    if (search_len == 0) {
        fclose(file);
        return 0;
    }

    long file_size;
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    for (long i = 0; i <= file_size - search_len; i++) {
        int match = 1;
        
        for (int j = 0; j < search_len; j++) {
            char c;
            fseek(file, i + j, SEEK_SET);
            c = fgetc(file);
            
            if (c != search_string[j]) {
                match = 0;
                break;
            }
        }
        
        if (match) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <file1> <file2> ... <flag> [args]\n", argv[0]);
        return ERROR_USAGE;
    }

    char *flag = argv[argc - 1];
    int file_count = argc - 2;

    if (strncmp(flag, "xor", 3) == 0) {
        int n = atoi(flag + 3);
        if (n < 2 || n > 6) {
            printf("Invalid N value for xorN\n");
            return ERROR_USAGE;
        }

        for (int i = 1; i <= file_count; i++) {
            xorN(argv[i], n);
        }
    } else if (strncmp(flag, "mask", 4) == 0) {
        if (argc < 4) {
            printf("Usage: %s <file1> <file2> ... mask <hex>\n", argv[0]);
            return ERROR_USAGE;
        }

        uint32_t mask_value = strtoul(argv[argc - 1], NULL, 16);

        for (int i = 1; i <= file_count; i++) {
            mask(argv[i], mask_value);
        }
    } else if (strncmp(flag, "copy", 4) == 0) {
        int n = atoi(flag + 4);
        if (n <= 0) {
            printf("Invalid N value for copyN\n");
            return ERROR_USAGE;
        }

        for (int i = 1; i <= file_count; i++) {
            copyN(argv[i], n);
        }
    } else if (strncmp(flag, "find", 4) == 0) {
        if (argc < 4) {
            printf("Usage: %s <file1> <file2> ... find <string>\n", argv[0]);
            return ERROR_USAGE;
        }

        char *search_string = argv[argc - 2];

        for (int i = 1; i < file_count; i++) {
            pid_t pid = fork();
            if (pid == 0) {
                if (find(argv[i], search_string)) {
                    printf("Found string in: %s\n", argv[i]);
                } else {
                    printf("Did not find string in: %s\n", argv[i]);
                }
                exit(EXIT_SUCCESS);
            } else if (pid < 0) {
                printf("Failed to fork");
                return ERROR_FORK;
            }
        }

        for (int i = 0; i < file_count; i++) {
            wait(NULL);
        }
    } else {
        printf("Unknown flag: %s\n", flag);
        return ERROR_USAGE;
    }

    return SUCCESS;
}