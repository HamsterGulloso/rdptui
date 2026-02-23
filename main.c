#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#define TB_IMPL
#include "termbox2/termbox2.h"

#define DYNAMIC_RES "+dynamic-resolution"

#define PROGRAM_NAME "rdptui"
#define SERVER_DIR "/.local/share/"  PROGRAM_NAME  "/"
#define SERVER_FILE SERVER_DIR "saved_servers"

char server_dir[255];
char server_file[255];

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

int read_till_null(char* output_str, FILE* file) {
    int fread_result;
    char read_char;
    size_t strsize = 0;

    do {
        fread_result = fread(&read_char, sizeof(char), 1, file);

        if (fread_result != 1) return -1;
        if (read_char == 0) break;

        output_str[strsize++] = read_char;
    } while (1);

    output_str[strsize] = 0;
    return strsize;
}

ServerArray* storage_read() {
    int read_size;
    char read_str[255];
    Server aux;
    ServerArray* sa;
    FILE* storage = NULL;

    sa = malloc(sizeof(ServerArray));
    serverarray_init(sa);

    storage = fopen(server_file, "rb");

    if (storage == NULL) {
        return sa;
    }

    do {
        read_size = read_till_null(read_str, storage);
        if(read_size == -1) break;
        aux.hostname = malloc(sizeof(char)*read_size);
        strcpy(aux.hostname, read_str);

        read_size = read_till_null(read_str, storage);
        if(read_size == -1) break;
        aux.username = malloc(sizeof(char)*read_size);
        strcpy(aux.username, read_str);

        serverarray_append(sa, aux);
    } while(1);

    fclose(storage);
    return sa;
}

int storage_save(ServerArray* sa) {
    DIR* dir = opendir(server_dir);
    FILE* file;

    if (dir == NULL) {
        mkdir(server_dir, 0777);
    }

    file = fopen(server_file, "w+b");

    if (file == NULL) {
        return 0;
    }

    for (int i = 0; i < sa->size; i ++) {
        fwrite(sa->s[i].hostname, sizeof(char), strlen(sa->s[i].hostname), file);
        fwrite("\0", sizeof(char), 1, file);

        fwrite(sa->s[i].username, sizeof(char), strlen(sa->s[i].username), file);
        fwrite("\0", sizeof(char), 1, file);
    }

    fclose(file);

    return 1;
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

int confirm_dialog(){
    struct tb_event e;
    char text[] = "Are you sure?(y/n)";

    tb_clear();
    tb_printf(
        ((int) tb_width()*0.5) - strlen(text)/2,
        ((int) tb_height()*0.5),
        0, 0,
        text
    );
    tb_present();

    do {
        tb_poll_event(&e);

        if (
            ((e.ch | 0x20) == 'n') ||
            e.key == TB_KEY_ESC
        ) return 0;
        
        if ((e.ch | 0x20) == 'y') return 1;
    } while (1);
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
        execl("/bin/xfreerdp", "/bin/xfreerdp", username_string, hostname_string, password_string, DYNAMIC_RES, NULL);
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

int main(int argc, char** argv) {
    int width;
    int height;
    struct tb_event e;
    int selected_option = 0;
    ServerArray* sa;
    char password[255];
    char* hostname;
    char* username;
    char** hostnames;
    char** usernames;
    
    if (getenv("HOME") == NULL) {
        perror("AAAAAAA");
        return 1;
    }

    sprintf(server_dir, "%s" SERVER_DIR, getenv("HOME"));
    sprintf(server_file, "%s" SERVER_FILE, getenv("HOME"));

    sa = storage_read(); 

    hostnames = malloc((sa->size+1)*sizeof(char*));
    usernames = malloc((sa->size+1)*sizeof(char*));
    update_items(&hostnames, &usernames, sa);
    hostnames[0] = "Hostnames:";
    usernames[0] = "Usernames:";

    tb_init();

    do {
        width = tb_width();
        height = tb_height();

        tb_clear();

        position_texts(hostnames, sa->size + 1, 1./3., .5, selected_option + 1);
        position_texts(usernames, sa->size + 1, 2./3., .5, selected_option + 1);
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
            (e.ch | 0x20) == 'i' ||
            (e.ch | 0x20) == 'a'
        ) {
            hostname = malloc(255*sizeof(char));
            if (!handle_input("Hostname:", hostname)) continue;

            username = malloc(255*sizeof(char));
            if (!handle_input("Username:", username)) continue;

            serverarray_append(
                sa,
                server_create(hostname, username)
            );
            update_items(&hostnames, &usernames, sa);
            storage_save(sa);
        }

        if (
            (e.ch | 0x20) == 'x' ||
            (e.ch | 0x20) == 'r' ||
            (e.ch | 0x20) == 'd'
        ) {
            if (confirm_dialog()) {
                serverarray_remove(
                    sa,
                    selected_option
                );
                update_items(&hostnames, &usernames, sa);
                storage_save(sa);
            }
        }

        if (e.key == TB_KEY_ARROW_UP) {
            selected_option = selected_option - 1;
            selected_option = selected_option < 0 ? sa->size - 1 : selected_option;
        }

        if (e.key == TB_KEY_ARROW_DOWN) {
            selected_option = (selected_option + 1) % sa->size;
        }

        if (e.key == TB_KEY_ENTER) {
            if (sa->size == 0) continue;
            int password_entered = handle_input("Password:", password);
            if (password_entered) {
                fork_xfreerdp(
                    sa->s[selected_option].hostname,
                    sa->s[selected_option].username,
                    password
                );
            }
        }

    } while (1);

    tb_shutdown();
}
