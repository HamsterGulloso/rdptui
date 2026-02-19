#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TB_IMPL
#include "termbox2/termbox2.h"

#define SCREEN_SIZE_STR  "/size:1920x1080"

typedef struct {
    struct {
        char *str;
        int size;
    } hostname;
    struct {
        char* str;
        int size;
    } username;
} Server;

void position_texts(char** text, int num_texts, float x, float y, int selected_index) {
    for(int i = 0; i < num_texts; i ++) {
        tb_printf(
            ((int) tb_width()*x) - strlen(text[i])/2,
            ((int) tb_height()*y) + i - num_texts/2,
            i == selected_index ? TB_REVERSE | TB_DEFAULT : TB_DEFAULT,
            i == selected_index ? TB_REVERSE | TB_DEFAULT : TB_DEFAULT,
            text[i]
        );
    }
}

int request_password(char *password) {
    struct tb_event e;
    char* texts[2];

    texts[0] = "Password:";
    texts[1] = password;

    do {
        tb_clear();

        position_texts(texts, 2, 0.5, 0.5, -1);
        tb_present();

        tb_poll_event(&e);
        if(e.key == TB_KEY_ESC) {
            return 0;
        }
        if(e.key == TB_KEY_ENTER) {
            return 1;
        }
        if(e.key == TB_KEY_BACKSPACE || e.key == TB_KEY_BACKSPACE2) {
            texts[1][strlen(texts[1])-1] = 0;
            continue;
        }
        sprintf(texts[1], "%s%c", texts[1], e.ch);
    } while (
        e.key != TB_KEY_ESC
        && e.key != TB_KEY_ENTER
    );
}

void fork_xfreerdp(char* hostname, char* username, char* password) {
    int pid;
    int size;
    char* username_string;
    char* hostname_string;
    char* password_string;
    struct tb_event e;

    pid = fork();
    if(pid == 0) {
        size = strlen(hostname);
        hostname_string = malloc((size+3)*sizeof(char));
        sprintf(hostname_string, "/v:%s", hostname);

        size = strlen(username);
        username_string = malloc((size+3)*sizeof(char));
        sprintf(username_string, "/u:%s", username);

        size = strlen(password);
        password_string = malloc((size+3)*sizeof(char));
        sprintf(password_string, "/p:%s", password);

        int dev_null = open("/dev/null", O_WRONLY);

        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
        execl("/bin/xfreerdp", "xfreerdp", SCREEN_SIZE_STR, username_string, hostname_string, password_string, NULL);
    }
}

int main() {
    int width;
    int height;
    struct tb_event e;
    int num_servers = 1;
    int selected_option = 0;
    Server servers[1];
    char password[255] = "";

    servers[0].hostname.str = "192.168.0.180";
    servers[0].hostname.size = strlen("192.168.0.180");
    servers[0].username.str = "bi";
    servers[0].username.size = strlen("bi");

    char* hostnames[2];
    char* usernames[2];

    hostnames[0] = "Hostnames:";
    usernames[0] = "Usernames:";
    for(int i = 0; i < num_servers; i++) {
        hostnames[i+1] = servers[i].hostname.str;
        usernames[i+1] = servers[i].username.str;
    }

    tb_init();

    do {
        width = tb_width();
        height = tb_height();

        tb_clear();

        position_texts(hostnames, num_servers + 1, 1./3., .5, selected_option + 1);
        position_texts(usernames, num_servers + 1, 2./3., .5, selected_option + 1);
        tb_present();

        tb_poll_event(&e);

        if (
            e.key == TB_KEY_ESC
            || e.ch == 'q'
        ) {
            break;
        }

        if (e.key == TB_KEY_ARROW_UP) {
            selected_option = selected_option - 1;
            selected_option = selected_option < 0 ? num_servers - 1 : selected_option;
        }

        if (e.key == TB_KEY_ARROW_DOWN) {
            selected_option = (selected_option + 1) % num_servers;
        }

        if (e.key == TB_KEY_ENTER) {
            password[0] = 0;
            int password_entered = request_password(password);
            if (password_entered) {
                fork_xfreerdp(
                    servers[selected_option].hostname.str,
                    servers[selected_option].username.str,
                    password
                );
            }
        }

    } while (1);

    tb_shutdown();
}
