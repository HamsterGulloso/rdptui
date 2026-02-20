#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define TB_IMPL
#include "termbox2/termbox2.h"

#define DYNAMIC_RES "+dynamic-resolution"

typedef struct {
    char* hostname;
    char* username;
} Server;

Server server_create(char* h, char* u) {
    Server s;
    s.username = u;
    s.hostname = h;
    return s;
}

typedef struct {
    Server* s;
    size_t capacity;
    size_t size;
} ServerArray;

void serverarray_init(ServerArray* sa) {
    sa->size = 0;
    sa->capacity = 4;
    sa->s = malloc(sizeof(Server)*sa->capacity);
}

void serverarray_append(ServerArray* sa, Server s) {
    if (sa->size >= sa->capacity) {
        sa->capacity *= 2;
        sa->s = realloc(sa, sizeof(Server)*sa->capacity);
    }
    sa->s[sa->size++] = s;
}

void serverarray_remove(ServerArray* sa, size_t index) {
    Server aux;
    sa->size--;
    for (int i = index; i < sa->size; i ++) {
        sa->s[i] = sa->s[i+1];
    }
}

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

int handle_input(char* input_text, char *input) {
    struct tb_event e;
    char* texts[2];

    input[0] = 0;

    texts[0] = input_text;
    texts[1] = input;

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
            input[strlen(texts[1])-1] = 0;
            continue;
        }

        sprintf(input, "%s%c", input, e.ch);
    } while (1);
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
        execl("/bin/xfreerdp", "xfreerdp", username_string, hostname_string, password_string, DYNAMIC_RES, NULL);
    }
}

void update_items(char*** hostnames, char*** usernames, ServerArray* sa) {
    *hostnames = realloc(*hostnames, (sa->size+1)*sizeof(char*));
    *usernames = realloc(*usernames, (sa->size+1)*sizeof(char*));

    for(int i = 0; i < sa->size; i++) {
        (*hostnames)[i+1] = sa->s[i].hostname;
        (*usernames)[i+1] = sa->s[i].username;
    }
}

int main() {
    int width;
    int height;
    struct tb_event e;
    int selected_option = 0;
    ServerArray sa;
    char password[255];
    char* hostname;
    char* username;
    char** hostnames;
    char** usernames;

    serverarray_init(&sa);

    update_items(&hostnames, &usernames, &sa);
    hostnames[0] = "Hostnames:";
    usernames[0] = "Usernames:";

    tb_init();

    do {
        width = tb_width();
        height = tb_height();

        tb_clear();

        position_texts(hostnames, sa.size + 1, 1./3., .5, selected_option + 1);
        position_texts(usernames, sa.size + 1, 2./3., .5, selected_option + 1);
        tb_present();

        tb_poll_event(&e);

        if (
            e.key == TB_KEY_ESC
            || (e.ch | 0x20) == 'q'
        ) {
            break;
        }

        if (
            (e.ch | 0x20) == 'n' ||
            (e.ch | 0x20) == 'a'
        ) {
            hostname = malloc(255*sizeof(char));
            if (!handle_input("Hostname:", hostname)) continue;

            username = malloc(255*sizeof(char));
            if (!handle_input("Username:", username)) continue;

            serverarray_append(
                &sa,
                server_create(hostname, username)
            );
            update_items(&hostnames, &usernames, &sa);
        }

        if (e.key == TB_KEY_ARROW_UP) {
            selected_option = selected_option - 1;
            selected_option = selected_option < 0 ? sa.size - 1 : selected_option;
        }

        if (e.key == TB_KEY_ARROW_DOWN) {
            selected_option = (selected_option + 1) % sa.size;
        }

        if (e.key == TB_KEY_ENTER) {
            if (sa.size == 0) continue;
            int password_entered = handle_input("Password:", password);
            if (password_entered) {
                fork_xfreerdp(
                    sa.s[selected_option].hostname,
                    sa.s[selected_option].username,
                    password
                );
            }
        }

    } while (1);

    tb_shutdown();
}
