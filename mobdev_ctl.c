#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#define SYS_mobdev_control 467  // System call number

// Commands must match what the kernel expects
enum mobdev_cmd {
    MOBDEV_DETECT = 0,
    MOBDEV_FILE_TRANSFER,
    MOBDEV_TETHERING,
    MOBDEV_NOTIFICATIONS,
    MOBDEV_CALL_CONTROL   // 📞 Call handling
};

// Updated structure to include 'action' for call control.
struct mobdev_args {
    int enable;       // For file transfer, tethering, and notifications (e.g., 1 = push or on)
    char path[128];   // File path for transfer
    char ifname[32];  // Network interface name (for tethering)
    int action;       // For call control: 1 = answer, 0 = reject
};

static void usage(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s detect\n", prog);
    fprintf(stderr, "  %s transfer [push|pull] <file_path>\n", prog);
    fprintf(stderr, "  %s tether [on|off] <interface>\n", prog);
    fprintf(stderr, "  %s notify [on|off]\n", prog);
    fprintf(stderr, "  %s call [answer|reject]\n", prog);
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

    // 1️⃣ DETECT command
    if (!strcmp(argv[1], "detect")) {
        cmd = MOBDEV_DETECT;
    } 
    // 2️⃣ FILE TRANSFER (Push/Pull)
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
    }
    // 3️⃣ TETHERING CONTROL
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
    // 4️⃣ NOTIFICATIONS MIRRORING
    else if (!strcmp(argv[1], "notify")) {
        cmd = MOBDEV_NOTIFICATIONS;
        if (argc >= 3 && !strcmp(argv[2], "on")) {
            args.enable = 1;
        } else {
            args.enable = 0;
        }
    }
    // 5️⃣ CALL HANDLING (Answer/Reject)
    else if (!strcmp(argv[1], "call")) {
        cmd = MOBDEV_CALL_CONTROL;
        if (argc < 3) {
            fprintf(stderr, "Usage: %s call [answer|reject]\n", argv[0]);
            return 1;
        }
        if (!strcmp(argv[2], "answer")) {
            args.action = 1;  // 1 for answering
        } else if (!strcmp(argv[2], "reject")) {
            args.action = 0;  // 0 for rejecting
        } else {
            fprintf(stderr, "Invalid call option. Use 'answer' or 'reject'.\n");
            return 1;
        }
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        usage(argv[0]);
        return 1;
    }

    // Invoke the system call.
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
