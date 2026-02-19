#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

#define SCREEN_SIZE_STR  "/size:1920x1080"

int main() {
    char* username;
    char* username_string;
    char* hostname;
    char* hostname_string;
    char* password;
    char* password_string;
    int pid;
    int size;

    printf("Insert hostname: ");
    scanf("%ms", &hostname);
    size = strlen(hostname);
    hostname_string = malloc((size+3)*sizeof(char));
    sprintf(hostname_string, "/v:%s", hostname);

    printf("Insert username: ");
    scanf("%ms", &username);
    size = strlen(username);
    username_string = malloc((size+3)*sizeof(char));
    sprintf(username_string, "/u:%s", username);

    printf("Insert password: ");
    scanf("%ms", &password);
    size = strlen(password);
    password_string = malloc((size+3)*sizeof(char));
    sprintf(password_string, "/p:%s", password);

    printf("%s %s %s %s %s %s", "/bin/xfreerdp", "xfreerdp", SCREEN_SIZE_STR, username_string, hostname_string, password_string);
    execl("/bin/xfreerdp", "xfreerdp", SCREEN_SIZE_STR, username_string, hostname_string, password_string, NULL);
}
