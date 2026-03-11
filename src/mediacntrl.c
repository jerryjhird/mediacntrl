#include <systemd/sd-bus.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

const char **g_argv;
int g_argc;

typedef enum {
    CMD_PLAY, CMD_PAUSE, CMD_PLAY_PAUSE,
    CMD_STOP, CMD_NEXT, CMD_PREVIOUS,
    CMD_POSITION, CMD_LIST, CMD_UNKNOWN
} command_t;

static const char *command_list[] = {
    "play", "pause", "play-pause", "stop",
    "next", "previous", "position", "list", NULL
};

static command_t parse_cmd(const char *s) {
    if (!s) return CMD_UNKNOWN;
    for (int i=0; command_list[i]; i++)
        if (!strcmp(s, command_list[i]))
            return (command_t)i;
    return CMD_UNKNOWN;
}

static char *copy_str(const char *s) {
    size_t len = strlen(s);
    char *out = malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len + 1);
    return out;
}

static void print_help(void) {
    printf(
        "Mediacntrl a cli for controlling media players\n\n"
        "Usage:\n"
        "  %s <command> [args]\n"
        "  %s <player> <command> [args]\n"
        "\n"
        #ifdef HTTP_SUPPORT
        "%s --http:\n"
        "  api-format: http://localhost:port/player-query/command\n\n"
        #endif
        "Commands:\n"
        "  play            start playback\n"
        "  pause           pause playback\n"
        "  play-pause      toggle playback\n"
        "  stop            stop playback\n"
        "  next            next track\n"
        "  previous        previous track\n"
        "  position <s>    jump to position (seconds)\n"
        "\n"
        "Examples:\n"
        "  %s play\n"
        "  %s vlc pause\n"
        "  %s firefox position 10\n"
        "  %s play-pause\n\n",
        g_argv[0], g_argv[0], g_argv[0],
        g_argv[0], g_argv[0], g_argv[0], g_argv[0]
    );
}

static char **list_players(sd_bus *bus, int *count) {
    char **acquired = NULL, **activatable = NULL;
    if (sd_bus_list_names(bus, &acquired, &activatable) < 0)
        return NULL;

    char **players = NULL;
    int n = 0;

    for (int i = 0; acquired && acquired[i]; i++) {
        if (!strncmp(acquired[i], "org.mpris.MediaPlayer2.", 23)) {
            char **tmp = realloc(players, (n + 1) * sizeof(char *));
            if (!tmp) break;
            players = tmp;
            players[n++] = copy_str(acquired[i]);
        }
    }

    for (int i = 0; acquired && acquired[i]; i++) free(acquired[i]);
    for (int i = 0; activatable && activatable[i]; i++) free(activatable[i]);
    free(acquired);
    free(activatable);

    *count = n;
    return players;
}

static char *choose_player(sd_bus *bus, const char *hint) {
    int n = 0;
    char **players = list_players(bus, &n);
    if (!players || n == 0) return NULL;

    char *sel = NULL;

    if (hint) {
        for (int i = 0; i < n; i++) {
            const char *name = players[i] + 23;
            if (strstr(name, hint)) {
                sel = copy_str(players[i]);
                break;
            }
        }
    }

    if (!sel)
        sel = copy_str(players[0]);

    for (int i = 0; i < n; i++) free(players[i]);
    free(players);
    return sel;
}

static char *get_track_id(sd_bus *bus, const char *player) {
    sd_bus_message *reply = NULL;
    char *track = NULL;

    if (sd_bus_call_method(bus, player,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "Get", NULL, &reply,
        "ss", "org.mpris.MediaPlayer2.Player", "Metadata") < 0) {
        sd_bus_message_unref(reply);
        return track;
    }

    sd_bus_message_enter_container(reply, 'v', "a{sv}");
    sd_bus_message_enter_container(reply, 'a', "{sv}");

    while (sd_bus_message_enter_container(reply, 'e', "sv") > 0) {
        const char *key;
        sd_bus_message_read(reply, "s", &key);

        if (!strcmp(key, "mpris:trackid")) {
            sd_bus_message_enter_container(reply, 'v', "o");
            const char *obj;
            sd_bus_message_read(reply, "o", &obj);
            track = copy_str(obj);
            
            sd_bus_message_unref(reply);
            return track;
        }
        sd_bus_message_skip(reply, "v");
        sd_bus_message_exit_container(reply);
    }
}

int eval_command(const char *player_hint, const char *cmd_str, const char *arg1) {
    command_t cmd = parse_cmd(cmd_str);
    if (cmd == CMD_UNKNOWN) return -1;

    sd_bus *bus = NULL;
    if (sd_bus_open_user(&bus) < 0) return -2;

    if (cmd == CMD_LIST) {
        int n = 0;
        char **players = list_players(bus, &n);
        if (players) {
            for (int i = 0; i < n; i++) {
                printf("%s\n", players[i] + 23);
                free(players[i]);
            }
            free(players);
        }
        sd_bus_unref(bus);
        return 0;
    }

    char *player = choose_player(bus, player_hint);
    if (!player) {
        sd_bus_unref(bus);
        return -3;
    }

    int ret = 0;
    if (cmd == CMD_PLAY)
        sd_bus_call_method(bus, player, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Play", NULL, NULL, "");
    else if (cmd == CMD_PAUSE)
        sd_bus_call_method(bus, player, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Pause", NULL, NULL, "");
    else if (cmd == CMD_PLAY_PAUSE)
        sd_bus_call_method(bus, player, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "PlayPause", NULL, NULL, "");
    else if (cmd == CMD_STOP)
        sd_bus_call_method(bus, player, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Stop", NULL, NULL, "");
    else if (cmd == CMD_NEXT)
        sd_bus_call_method(bus, player, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Next", NULL, NULL, "");
    else if (cmd == CMD_PREVIOUS)
        sd_bus_call_method(bus, player, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Previous", NULL, NULL, "");
    else if (cmd == CMD_POSITION) {
        if (!arg1 || *arg1 == '\0') {
            ret = -4;
        } else {
            char *end;
            double sec = strtod(arg1, &end);
            if (end == arg1 || *end != '\0' || sec < 0) {
                ret = -4;
            } else {
                int64_t usec = (int64_t)(sec * 1000000.0);
                char *track = get_track_id(bus, player);
                if (track) {
                    sd_bus_call_method(bus, player, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "SetPosition", NULL, NULL, "ox", track, usec);
                    free(track);
                } else {
                    ret = -5;
                }
            }
        }
    }

    free(player);
    sd_bus_unref(bus);
    return ret;
}

#ifdef HTTP_SUPPORT
#include "http.c"
#endif

int main(int argc, char **argv) {
    g_argc = argc;
    g_argv = (const char **)argv;

    if (argc < 2) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--http") == 0) {
        #ifdef HTTP_SUPPORT
            int port = (argc >= 3) ? atoi(argv[2]) : 8000;
            start_http_server(port);
            return 0;
        #else
            fprintf(stderr, "ERROR: this build of mediacntrl was not compiled with HTTP support\n");
            return 1;
        #endif
    }
    const char *player_hint = NULL;
    const char *cmd_str = NULL;
    const char *arg1 = NULL;

    command_t c1 = parse_cmd(argv[1]);
    if (c1 != CMD_UNKNOWN) {
        cmd_str = argv[1];
        arg1 = (argc >= 3) ? argv[2] : NULL;
    } else {
        if (argc < 3) {
            print_help();
            return 0;
        }
        player_hint = argv[1];
        cmd_str = argv[2];
        arg1 = (argc >= 4) ? argv[3] : NULL;
    }

    int res = eval_command(player_hint, cmd_str, arg1);
    if (res < 0) {
        if (res == -4) print_help();
        return 1;
    }

    return 0;
}
