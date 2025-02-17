#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#define SYS_mobdev_control 467  // Syscall number

// Commands must match what the kernel expects
enum mobdev_cmd {
    MOBDEV_DETECT = 0,
    MOBDEV_FILE_TRANSFER,
    MOBDEV_TETHERING,
    MOBDEV_NOTIFICATIONS
};

struct mobdev_args {
    int  enable;       // 1 = push, 0 = pull
    char path[128];    // File path for transfer
    char ifname[32];   // Network interface name
};

static void usage(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s detect\n", prog);
    fprintf(stderr, "  %s transfer [push|pull] <file_path>\n", prog);
    fprintf(stderr, "  %s tether [on|off] <interface>\n", prog);
    fprintf(stderr, "  %s notify [on|off]\n", prog);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    int cmd = -1;
    struct mobdev_args args;
    memset(&args, 0, sizeof(args));

    // Map command-line argument to our command enum
    if (!strcmp(argv[1], "detect")) {
        cmd = MOBDEV_DETECT;
    } 
    
    else if (!strcmp(argv[1], "transfer")) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s transfer [push|pull] <file_path>\n", argv[0]);
            return 1;
        }

        cmd = MOBDEV_FILE_TRANSFER;
        if (!strcmp(argv[2], "push")) {
            args.enable = 1;
        } else if (!strcmp(argv[2], "pull")) {
            args.enable = 0;
        } else {
            fprintf(stderr, "Invalid transfer mode. Use 'push' or 'pull'.\n");
            return 1;
        }

        strncpy(args.path, argv[3], sizeof(args.path) - 1);
        args.path[sizeof(args.path) - 1] = '\0';

        long ret = syscall(SYS_mobdev_control, cmd, &args);
        if (ret < 0) {
            perror("mobdev_control syscall failed");
            return 1;
        }

        // If kernel detected an MTP device, use ADB for file transfer
        if (args.enable) {
            char adb_cmd[256];
            snprintf(adb_cmd, sizeof(adb_cmd), "adb push %s /sdcard/", args.path);
            printf("Executing: %s\n", adb_cmd);
            system(adb_cmd);
        } else {
            char adb_cmd[256];
            snprintf(adb_cmd, sizeof(adb_cmd), "adb pull /sdcard/%s .", args.path);
            printf("Executing: %s\n", adb_cmd);
            system(adb_cmd);
        }
        
        return 0;
    } 

    else if (!strcmp(argv[1], "tether")) {
        cmd = MOBDEV_TETHERING;
        if (argc >= 3 && !strcmp(argv[2], "on")) {
            args.enable = 1;
        } else {
            args.enable = 0;
        }

        if (argc >= 4) {
            strncpy(args.ifname, argv[3], sizeof(args.ifname) - 1);
            args.ifname[sizeof(args.ifname) - 1] = '\0';
        } else {
            strcpy(args.ifname, "usb0");
        }
    }

    else if (!strcmp(argv[1], "notify")) {
        cmd = MOBDEV_NOTIFICATIONS;
        if (argc >= 3 && !strcmp(argv[2], "on")) {
            args.enable = 1;
        } else {
            args.enable = 0;
        }
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        usage(argv[0]);
        return 1;
    }

    // Call syscall
    long ret;
    if (cmd == MOBDEV_DETECT) {
        ret = syscall(SYS_mobdev_control, cmd, 0);
    } else {
        ret = syscall(SYS_mobdev_control, cmd, &args);
    }

    if (ret < 0) {
        perror("mobdev_control syscall failed");
        return 1;
    }

    printf("mobdev_control returned: %ld\n", ret);
    return 0;
}
