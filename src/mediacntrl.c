#define _POSIX_C_SOURCE 200809L

#include <systemd/sd-bus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MPRIS_PREFIX "org.mpris.MediaPlayer2"
#define MPRIS_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_INTERFACE "org.mpris.MediaPlayer2.Player"

int strcasestr_in(const char *haystack, const char *needle) {
    if (!needle || !*needle) return 1;
    for (; *haystack; haystack++) {
        if (tolower(*haystack) == tolower(*needle)) {
            const char *h = haystack, *n = needle;
            while (*h && *n && tolower(*h) == tolower(*n)) h++, n++;
            if (!*n) return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    char *query = NULL, *command = NULL;
    int quiet = 0;
    int i = 1;

    for (int j = 1; j < argc; j++) {
        if (strcmp(argv[j], "--nstdout") == 0) {
            quiet = 1;
            for (int k = j; k < argc - 1; k++) {
                argv[k] = argv[k+1];
            }
            argc--;
            j--;
        }
    }

    if (argc < 2) {
        if (!quiet)
            fprintf(stderr, "Usage: %s [-q query] command\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-q") == 0) {
        if (argc < 4) {
            if (!quiet)
                fprintf(stderr, "Usage: %s [-q query] command\n", argv[0]);
            return 1;
        }
        query = argv[2];
        command = argv[3];
        i = 4;
    } else {
        command = argv[1];
        i = 2;
    }

    sd_bus *bus = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *msg = NULL;
    int ret;

    ret = sd_bus_open_user(&bus);
    if (ret < 0) {
        if (!quiet)
            fprintf(stderr, "failed to connect to user bus: %s\n", strerror(-ret));
        return 1;
    }

    ret = sd_bus_call_method(bus,
                             "org.freedesktop.DBus",
                             "/org/freedesktop/DBus",
                             "org.freedesktop.DBus",
                             "ListNames",
                             &error,
                             &msg,
                             NULL);
    if (ret < 0) {
        if (!quiet)
            fprintf(stderr, "failed to list interfaces: %s\n", error.message);
        sd_bus_error_free(&error);
        return 1;
    }

    const char *name;
    int count = 0;
    char *match = NULL;

    ret = sd_bus_message_enter_container(msg, 'a', "s");
    if (ret < 0) {
        if (!quiet)
            fprintf(stderr, "failed to parse reply\n");
        return 1;
    }

    while ((ret = sd_bus_message_read(msg, "s", &name)) > 0) {
        if (strncmp(name, MPRIS_PREFIX, strlen(MPRIS_PREFIX)) == 0) {
            if (!query || strcasestr_in(name, query)) {
                if (!quiet)
                    printf("%s\n", name);
                count++;
                if (count == 1) {
                    match = strdup(name);
                }
            }
        }
    }

    sd_bus_message_exit_container(msg);

    if (count == 0) {
        if (!quiet)
            fprintf(stderr, "no matching players\n");
        goto cleanup;
    }

    if (count > 1) {
        if (!quiet)
            fprintf(stderr, "multiple interfaces match pattern\n");
        goto cleanup;
    }

    const char *method = NULL;
    if (strcmp(command, "play") == 0) method = "Play";
    else if (strcmp(command, "pause") == 0) method = "Pause";
    else if (strcmp(command, "play-pause") == 0) method = "PlayPause";
    else if (strcmp(command, "next") == 0) method = "Next";
    else if (strcmp(command, "previous") == 0) method = "Previous";
    else {
        if (!quiet)
            fprintf(stderr, "unknown command: %s\n", command);
        goto cleanup;
    }

    ret = sd_bus_call_method(bus, match, MPRIS_PATH, MPRIS_INTERFACE, method, &error, NULL, NULL);
    if (ret < 0) {
        if (!quiet)
            fprintf(stderr, "failed to call method: %s\n", error.message);
        sd_bus_error_free(&error);
    } else {
        if (!quiet)
            printf("'%s' > '%s'\n", method, match);
    }

cleanup:
    free(match);
    sd_bus_message_unref(msg);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return (count == 1 && ret >= 0) ? 0 : 1;
}
