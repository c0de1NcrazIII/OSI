#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_LOGIN_LENGTH 6
#define INITIAL_COMMAND_SIZE 10
#define DATABASE_FILE "bd.txt"

enum errors {
    SUCCESS = 0,
    ERROR_NO_USER = -1,
    ERROR_USERS_LIMIT = -2,
    ERROR_USER_ALREADY_EXIST = -3,
    ERROR_INVALID_PIN = -4,
    ERROR_INVALID_FLAG = -5,
    ERROR_TIME_PARSING = -6,
    ERROR_INPUT_CHOICE = -7,
    ERROR_TRASH_IN_LOGIN = -8,
    ERROR_FILE_OPEN = -9,
    ERROR_FILE_WRITE = -10,
    ERROR_FILE_READ = -11,
    ERROR_INVALID_COMMAND = -12,
    ERROR_MEM_ALLOC = -13,
};

typedef struct {
    char login[MAX_LOGIN_LENGTH + 1];
    char pin[7];
    long sanctions;
} User;

User* current_user = NULL;
int command_count = 0;

int valid_login(const char *str) {
    if (strlen(str) > MAX_LOGIN_LENGTH) {
        printf("Login must be no more than %d characters\n", MAX_LOGIN_LENGTH);
        return ERROR_TRASH_IN_LOGIN;
    }

    for (int i = 0; i < (int)strlen(str); ++i) {
        if (!isdigit(str[i]) && !isalpha(str[i])) {
            printf("Login can only contain letters and digits\n");
            return ERROR_TRASH_IN_LOGIN;
        }
    }

    return SUCCESS;
}

int valid_pin(const char *str) {
    if (strlen(str) > 6) {
        printf("PIN must be no more than 6 digits\n");
        return ERROR_INVALID_PIN;
    }

    for (int i = 0; i < (int)strlen(str); ++i) {
        if (!isdigit(str[i])) {
            printf("PIN can only contain digits\n");
            return ERROR_INVALID_PIN;
        }
    }

    char *endptr;
    int num = (int)strtol(str, &endptr, 10);
    if (*endptr != '\0') {
        printf("Invalid PIN format\n");
        return ERROR_INVALID_PIN;
    }

    if (num > 100000) {
        printf("PIN must be less than 100000\n");
        return ERROR_INVALID_PIN;
    }

    return SUCCESS;
}

User* find_user(const char* login) {
    FILE *file = fopen(DATABASE_FILE, "r");
    if (!file) {
        printf("Error: Could not open database file\n");
        return NULL;
    }
    
    User *user = (User*)malloc(sizeof(User));
    if (!user) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    while (fscanf(file, "%6s %6s %ld", user->login, user->pin, &user->sanctions) == 3) {
        if (strcmp(user->login, login) == 0) {
            fclose(file);
            return user;
        }
    }
    
    free(user);
    fclose(file);
    return NULL;
}

int login() {
    char *login = NULL;
    char *pin = NULL;
    size_t len1 = 0;
    size_t len2 = 0;

    printf("Login: ");
    if (getline(&login, &len1, stdin) == -1) {
        printf("Error reading input\n");
        return ERROR_INPUT_CHOICE;
    }

    login[strcspn(login, "\n")] = '\0';
    if (valid_login(login) != SUCCESS) {
        free(login);
        return ERROR_TRASH_IN_LOGIN;
    }

    printf("PIN: ");
    if (getline(&pin, &len2, stdin) == -1) {
        printf("Error reading input\n");
        free(login);
        return ERROR_INPUT_CHOICE;
    }

    pin[strcspn(pin, "\n")] = '\0';
    if (valid_pin(pin) != SUCCESS) {
        free(login);
        free(pin);
        return ERROR_INVALID_PIN;
    }

    User* user = find_user(login);
    if (user) {
        if (strcmp(pin, user->pin) == 0) {
            if (current_user) {
                free(current_user);
            }
            current_user = user;
            command_count = 0;
            printf("Welcome, %s\n", login);
        } else {
            printf("Invalid login or PIN\n");
            free(user);
        }
    } else {
        printf("Invalid login or PIN\n");
    }
    
    free(pin);
    free(login);
    return SUCCESS;
}

int add_user(const char* login, const char* pin) {
    FILE *file = fopen(DATABASE_FILE, "a");
    if (!file) {
        printf("Error: Could not open database file for writing\n");
        return ERROR_FILE_OPEN;
    }

    if (fprintf(file, "%s %s %d\n", login, pin, -1) < 0) {
        printf("Error: Failed to write to database file\n");
        fclose(file);
        return ERROR_FILE_WRITE;
    }

    fclose(file);
    return SUCCESS;
}

int register_user() {
    char *login = NULL;
    char *pin = NULL;
    size_t len1 = 0;
    size_t len2 = 0;
    
    printf("Login: ");
    if (getline(&login, &len1, stdin) == -1) {
        printf("Error reading input\n");
        return ERROR_INPUT_CHOICE;
    }
    
    login[strcspn(login, "\n")] = '\0';
    if (valid_login(login) != SUCCESS) {
        free(login);
        return ERROR_TRASH_IN_LOGIN;
    }

    printf("PIN: ");
    if (getline(&pin, &len2, stdin) == -1) {
        printf("Error reading input\n");
        free(login);
        return ERROR_INPUT_CHOICE;
    }

    pin[strcspn(pin, "\n")] = '\0';
    if (valid_pin(pin) != SUCCESS) {
        free(login);
        free(pin);
        return ERROR_INVALID_PIN;
    }
    
    User *user = find_user(login);
    if (user) {
        printf("User already exists\n");
        free(login);
        free(pin);
        free(user);
        return ERROR_USER_ALREADY_EXIST;
    }

    int result = add_user(login, pin);
    free(login);
    free(pin);
    
    if (result == SUCCESS) {
        printf("User registered successfully\n");
    }
    return result;
}

void logout() {
    if (current_user != NULL) {
        free(current_user);
        current_user = NULL;
    }
    command_count = 0;
    printf("Logged out successfully\n");
}

void print_time() {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    printf("Current time: %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void print_date() {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    printf("Current date: %02d:%02d:%04d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
}

int howmuch(const char* date_str, char flag) {
    int day, month, year;
    if (sscanf(date_str, "%d:%d:%d", &day, &month, &year) != 3) {
        printf("Invalid date format. Use dd:mm:yyyy\n");
        return ERROR_TIME_PARSING;
    }

    struct tm date = {0};
    date.tm_mday = day;
    date.tm_mon = month - 1;
    date.tm_year = year - 1900;

    time_t date_time = mktime(&date);
    if (date_time == -1) {
        printf("Invalid date\n");
        return ERROR_TIME_PARSING;
    }

    time_t now = time(NULL);
    double diff = difftime(now, date_time);

    switch (flag) {
        case 's':
            printf("Seconds passed: %.0f\n", diff);
            break;
        case 'm':
            printf("Minutes passed: %.0f\n", diff / 60);
            break;
        case 'h':
            printf("Hours passed: %.0f\n", diff / 3600);
            break;
        case 'y':
            printf("Years passed: %.2f\n", diff / (3600 * 24 * 365));
            break;
        default:
            printf("Invalid flag. Use s, m, h or y\n");
            return ERROR_INVALID_FLAG;
    }
    
    return SUCCESS;
}

int update_sanctions(const char* login, long sanctions) {
    FILE *file = fopen(DATABASE_FILE, "r");
    if (!file) {
        printf("Error: Could not open database file\n");
        return ERROR_FILE_OPEN;
    }

    FILE *temp_file = fopen("temp.txt", "w");
    if (!temp_file) {
        printf("Error: Could not create temporary file\n");
        fclose(file);
        return ERROR_FILE_OPEN;
    }

    User user;
    int found = 0;
    while (fscanf(file, "%6s %6s %ld", user.login, user.pin, &user.sanctions) == 3) {
        if (strcmp(user.login, login) == 0) {
            user.sanctions = sanctions;
            found = 1;
        }
        if (fprintf(temp_file, "%s %s %ld\n", user.login, user.pin, user.sanctions) < 0) {
            printf("Error: Failed to write to temporary file\n");
            fclose(file);
            fclose(temp_file);
            remove("temp.txt");
            return ERROR_FILE_WRITE;
        }
    }

    fclose(file);
    fclose(temp_file);

    if (!found) {
        printf("User not found\n");
        remove("temp.txt");
        return ERROR_NO_USER;
    }

    remove(DATABASE_FILE);
    rename("temp.txt", DATABASE_FILE);
    return SUCCESS;
}

int sanctions(const char* username, long number) {
    int confirm;
    printf("Enter secret-code to confirm sanctions: ");
    if (scanf("%d", &confirm) != 1) {
        printf("Invalid input\n");
        while (getchar() != '\n');
        return ERROR_INPUT_CHOICE;
    }
    while (getchar() != '\n');
    
    if (confirm != 12345) {
        printf("Sanctions not applied - wrong secret code\n");
        return ERROR_INPUT_CHOICE;
    }

    int result = update_sanctions(username, number);
    if (result == SUCCESS) {
          if (current_user && strcmp(current_user->login, username) == 0) {
               command_count = 0;
          }  
          printf("Sanctions successfully applied to %s\n", username);
    }
    return result;
}

int is_natural_number(const char *str) {
    if (*str == '-') {
        str++;
    }

    if (*str == '\0') return 0;

    while (*str) {
        if (*str == '.' || !isdigit(*str))
            return 0;
        str++;
    }
    return 1;
}

int main() {
    char *command = NULL;
    size_t command_size = 0;
    size_t command_capacity = INITIAL_COMMAND_SIZE;

    command = (char *)malloc(command_capacity * sizeof(char));
    if (!command) {
        printf("Error: Memory allocation failed\n");
        return ERROR_MEM_ALLOC;
    }

    while (1) {
        if (!current_user) {
            printf("1. Login\n2. Register\n3. Exit\n");

            command_size = 0;
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
                if (command_size >= command_capacity - 1) {
                    command_capacity *= 2;
                    char *new_command = (char *)realloc(command, command_capacity * sizeof(char));
                    if (!new_command) {
                        printf("Error: Memory allocation failed\n");
                        free(command);
                        return ERROR_MEM_ALLOC;
                    }
                    command = new_command;
                }
                command[command_size++] = ch;
            }
            command[command_size] = '\0';

            if (is_natural_number(command) || strlen(command) == 1) {
                char *endptr;
                long choice = strtol(command, &endptr, 10);
                if (*endptr != '\0') {
                    printf("Invalid choice - must be a number\n");
                    continue;
                }

                if (choice == 1) {
                    login();
                } else if (choice == 2) {
                    register_user();
                } else if (choice == 3) {
                    break;
                } else {
                    printf("Invalid choice - must be 1, 2 or 3\n");
                }
            } else {
                printf("Invalid choice - must be a number\n");
            }
        } else {
            if (current_user->sanctions != -1 && command_count >= current_user->sanctions) {
                printf("Command limit reached. Logging out.\n");
                logout();
                continue;
            }

            printf("> ");
            command_size = 0;
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
                if (command_size >= command_capacity - 1) {
                    command_capacity *= 2;
                    char *new_command = (char *)realloc(command, command_capacity * sizeof(char));
                    if (!new_command) {
                        printf("Error: Memory allocation failed\n");
                        free(command);
                        return ERROR_MEM_ALLOC;
                    }
                    command = new_command;
                }
                command[command_size++] = ch;
            }
            command[command_size] = '\0';

            if (strlen(command) == 20) {
                if (strncmp(command, "Howmuch", 7) == 0) {
                    char date_str[11];
                    char flag;
                    if (sscanf(command + 8, "%10s %c", date_str, &flag) == 2) {
                        if ((date_str[0] == '0' && date_str[1] == '0') || 
                            (date_str[3] == '0' && date_str[4] == '0') || 
                            (date_str[6] == '0' && date_str[7] == '0' && 
                             date_str[8] == '0' && date_str[9] == '0')) {
                            printf("Invalid date - cannot be all zeros\n");
                        } else {
                            howmuch(date_str, flag);
                        }
                    } else {
                        printf("Invalid Howmuch command format. Use: Howmuch dd:mm:yyyy [s|m|h|y]\n");
                    }
                } else {
                    printf("Unknown command\n");
                }
            } else if (strcmp(command, "Time") == 0) {
                print_time();
            } else if (strcmp(command, "Date") == 0) {
                print_date();
            } else if (strcmp(command, "Logout") == 0) {
                logout();
            } else if (strncmp(command, "Sanctions", 9) == 0) {
                char *token = strtok(command, " ");
                token = strtok(NULL, " ");
                if (token != NULL) {
                    char *username = token;
                    if (strlen(username) > 6) {
                        printf("Invalid login - must be 6 characters or less\n");
                        continue;
                    }
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        if (is_natural_number(token)) {
                            char *endptr;
                            long number = strtol(token, &endptr, 10);
                            if (number <= 0) {
                                printf("Number must be positive\n");
                                continue;
                            }
                            if (*endptr != '\0') {
                                printf("Invalid number format\n");
                            } else {
                                sanctions(username, number);
                            }
                        } else {
                            printf("Invalid number format\n");
                            continue;
                        }
                    } else {
                        printf("Missing number for Sanctions command. Use: Sanctions username number\n");
                    }
                } else {
                    printf("Missing username for Sanctions command. Use: Sanctions username number\n");
                }
            } else {
                printf("Unknown command\n");
            }

            command_count++;
        }
    }

    if (current_user) {
        free(current_user);
    }
    free(command);

    return SUCCESS;
}