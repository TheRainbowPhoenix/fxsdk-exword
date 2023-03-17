//---------------------------------------------------------------------------//
// ==>/[_]\   fxlink: A community communication tool for CASIO calculators.  //
//    |:::|   Made by Lephe' as part of the fxSDK.                           //
//    \___/   License: MIT <https://opensource.org/licenses/MIT>             //
//---------------------------------------------------------------------------//

#include "tui.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

//---
// Parsing utilities
//---

struct command {
    int argc;
    char const **argv;
    char *data;
};

static struct command parse_command(char const *input)
{
    struct command cmd;
    cmd.argc = 0;
    cmd.argv = NULL;
    cmd.data = malloc(strlen(input) + 1);
    if(!cmd.data)
        return cmd;

    char const *escapes1 = "\\nter";
    char const *escapes2 = "\\\n\t\e\r";

    /* Whether a new word needs to be created at the next character */
    bool word_finished = true;
    /* Offset into cmd.data */
    int i = 0;

    /* Read words eagerly, appending to cmd.data as we go */
    for(int j = 0; input[j]; j++) {
        int c = input[j];

        /* Stop words at spaces */
        if(isspace(c)) {
            if(!word_finished)
                cmd.data[i++] = 0;
            word_finished = true;
            continue;
        }

        /* Translate escapes */
        if(c == '\\') {
            char *p = strchr(escapes1, input[j+1]);
            if(p) {
                c = escapes2[p - escapes1];
                j++;
            }
        }

        /* Add a new word if necessary */
        if(word_finished) {
            cmd.argv = realloc(cmd.argv, (++cmd.argc) * sizeof *cmd.argv);
            cmd.argv[cmd.argc - 1] = cmd.data + i;
            word_finished = false;
        }

        /* Copy literals */
        cmd.data[i++] = c;
    }

    cmd.data[i++] = 0;
    return cmd;
}

static void dump_command(struct command const *cmd)
{
    print(TUI.wConsole, "command(%d)", cmd->argc);
    for(int i = 0; i < cmd->argc; i++) {
        char const *arg = cmd->argv[i];
        print(TUI.wConsole, " '%s'(%d)", arg, (int)strlen(arg));
    }
    print(TUI.wConsole, "\n");
}

static void free_command(struct command *cmd)
{
    free(cmd->argv);
    free(cmd->data);
}

/* Parse a list of arguments into strings/integers/etc. The format is a string
   of argument specifiers, which can be:
     s   String
     d   Integer
     *   Other variadic arguments (integer holding first index)
   (-- will probably be expanded later.)
   Returns true if parsing succeeded, false otherwise (including if arguments
   are missing) after printing an error message. */
static bool parse_arguments(int argc, char const **argv, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int i = 1;
    char const *spec = fmt;

    while(*spec) {
        if(i >= argc) {
            fprint(TUI.wConsole, FMT_RED, "error: ");
            print(TUI.wConsole, "too few arguments\n");
            goto failure;
        }

        /* Parse an argument specifier */
        if(*spec == 's') {
            char const **ptr = va_arg(args, char const **);
            *ptr = argv[i];
        }
        else if(*spec == 'd') {
            int *ptr = va_arg(args, int *);
            char *endptr;
            long l = strtol(argv[i], &endptr, 0);
            if(*endptr) {
                fprint(TUI.wConsole, FMT_RED, "error: ");
                print(TUI.wConsole, "not a valid integer: '%s'\n", argv[i]);
                goto failure;
            }
            *ptr = l;
        }

        spec++;
    }

    va_end(args);
    return true;

failure:
    va_end(args);
    return false;
}

static bool run_no_device(int argc, char const **argv)
{
    (void)argc;
    (void)argv;
    return false;
}

static void run_gintctl_command(int argc, char const **argv,
    struct fxlink_device *fdev)
{
    if(!strcmp(argv[0], "read-long")) {
        int count = 0;
        if(!parse_arguments(argc, argv, "d", &count))
            return;
        if(count < 0 || count > 8192)
            return;

        uint32_t *data = malloc(count * 4);
        for(int i = 0; i < count; i++)
            data[i] = i;
        fxlink_device_start_bulk_OUT(fdev,
            "gintctl", "echo-bounds", data, count * 4, true);
    }
}

static bool run_with_device(int argc, char const **argv,
    struct fxlink_device *fdev)
{
    if(!strcmp(argv[0], "/echo")) {
        int l = 5, j = 5;
        for(int i = 1; i < argc; i++)
            l += strlen(argv[i]) + 1;

        char *concat = malloc(l + 1);
        strcpy(concat, "echo ");
        for(int i = 1; i < argc; i++) {
            strcpy(concat + j, argv[i]);
            j += strlen(argv[i]);
            concat[j++] = (i == argc - 1) ? '\n' : ' ';
        }
        concat[j] = '\0';

        fxlink_device_start_bulk_OUT(fdev,
            "fxlink", "command", concat, l, true);
    }
    else if(!strcmp(argv[0], "/identify")) {
        fxlink_device_start_bulk_OUT(fdev,
            "fxlink", "command", "identify", 8, false);
    }
    else if(!strcmp(argv[0], "gintctl")) {
        if(argc <= 1) {
            fprint(TUI.wConsole, FMT_RED, "error: ");
            print(TUI.wConsole, "gintctl command needs sub-command\n");
            return true;
        }
        run_gintctl_command(argc-1, argv+1, fdev);
        return true;
    }
    else
        return false;
    return true;
}

void TUI_execute_command(char const *command)
{
    struct command cmd = parse_command(command);
    (void)dump_command;

    /* Connected device (TODO: Use the "selected" device) */
    struct fxlink_device *fdev = NULL;
    for(int i = 0; i < TUI.devices.count; i++) {
        if(TUI.devices.devices[i].status == FXLINK_FDEV_STATUS_CONNECTED) {
            fdev = &TUI.devices.devices[i];
            break;
        }
    }

    bool b = run_no_device(cmd.argc, cmd.argv);
    if(b) {
        free_command(&cmd);
        return;
    }

    /* The following commands require a connected device */
    if(!fdev) {
        print(TUI.wConsole, "no connected device!\n");
        return;
    }
    print(TUI.wConsole, "using device %s (%s)\n",
        fxlink_device_id(fdev), fdev->calc->serial);

    b = run_with_device(cmd.argc, cmd.argv, fdev);
    if(!b) {
        fprint(TUI.wConsole, FMT_RED, "error: ");
        print(TUI.wConsole, "unrecognized command '%s'\n", cmd.argv[0]);
    }
    free_command(&cmd);
}
