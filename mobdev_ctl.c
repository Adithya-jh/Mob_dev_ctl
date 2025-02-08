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
    int  enable;
    char path[128];
     char ifname[32];  // <-- Add a field for interface name
};

static void usage(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s detect\n", prog);
    fprintf(stderr, "  %s transfer <path>\n", prog);
    fprintf(stderr, "  %s tether [on|off]\n", prog);
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
    cmd = MOBDEV_FILE_TRANSFER;
    // We have args.path if we want a file path, etc.
    if (argc > 2) {
        strncpy(args.path, argv[2], sizeof(args.path) - 1);
    }

    long ret = syscall(SYS_mobdev_control, cmd, &args);
    if (ret < 0) {
        perror("mobdev_control syscall failed");
        return 1;
    }

    // Kernel side says "MTP device found"? Let's do the actual file copy in user space:
    system("jmtpfs /mnt/phone");
    // now copy files to/from /mnt/phone/DCIM ...
    // eventually "fusermount -u /mnt/phone"

    // or directly: system("simple-mtpfs /mnt/phone");
}
     
 else if (!strcmp(argv[1], "tether")) {
    cmd = MOBDEV_TETHERING;
    // Example usage: ./mobdev_ctl tether on enxc6307ebd3033
    if (argc >= 3 && !strcmp(argv[2], "on")) {
        args.enable = 1;
    } else {
        args.enable = 0;
    }
    if (argc >= 4) {
        // copy interface name from argv[3] to args.ifname
        strncpy(args.ifname, argv[3], sizeof(args.ifname) - 1);
        args.ifname[sizeof(args.ifname) - 1] = '\0';
    } else {
        // fallback if no interface is specified
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

    // Call our new syscall
    long ret;
    if (cmd == MOBDEV_DETECT) {
        // DETECT doesn't use the struct, so pass NULL (0).
        ret = syscall(SYS_mobdev_control, cmd, 0);
    } else {
        // The other commands use the 'mobdev_args' struct
        ret = syscall(SYS_mobdev_control, cmd, &args);
    }

    if (ret < 0) {
        perror("mobdev_control syscall failed");
        return 1;
    }

    printf("mobdev_control returned: %ld\n", ret);
    return 0;
}
