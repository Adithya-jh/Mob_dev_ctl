#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>

#define NETLINK_MOBDEV 31
#define MAX_PAYLOAD 1024

/* This structure must match the kernel's definition */
struct mobdev_netlink_msg {
    int type;   // 1 = volume, 2 = screen mirror, 3 = screenshot
    int value;  // For type 1: 1 = volume up, 0 = volume down;
                // For type 2: 1 = start mirror, 0 = stop mirror;
                // For type 3: flag (always 1)
};

int main(void)
{
    int sock_fd;
    struct sockaddr_nl src_addr;
    char buffer[MAX_PAYLOAD];
    struct nlmsghdr *nlh;
    struct mobdev_netlink_msg *msg;
    int ret;

    /* Create a Netlink socket */
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_MOBDEV);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    /* Bind to our Netlink address and subscribe to multicast group 1 */
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();  /* Use process PID as unique identifier */
    src_addr.nl_groups = 1;       /* Join multicast group 1 */

    ret = bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
    if (ret < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    printf("Netlink listener started.\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ret = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (ret < 0) {
            perror("recv");
            continue;
        }

        nlh = (struct nlmsghdr *)buffer;
        if (nlh->nlmsg_len < NLMSG_SPACE(sizeof(struct mobdev_netlink_msg))) {
            fprintf(stderr, "Received message too short\n");
            continue;
        }
        msg = (struct mobdev_netlink_msg *)NLMSG_DATA(nlh);

        /* Process the message based on type */
        if (msg->type == 1) {  // Volume command
            if (msg->value == 1) {
                printf("Received volume up command.\n");
                system("/usr/bin/adb shell input keyevent KEYCODE_VOLUME_UP");
            } else if (msg->value == 0) {
                printf("Received volume down command.\n");
                system("/usr/bin/adb shell input keyevent KEYCODE_VOLUME_DOWN");
            } else {
                printf("Unknown volume command value: %d\n", msg->value);
            }
        }
        else if (msg->type == 2) {  // Screen mirror command
            if (msg->value == 1) {
                printf("Received screen mirror start command.\n");
                system("/usr/bin/scrcpy &");
            } else if (msg->value == 0) {
                printf("Received screen mirror stop command.\n");
                system("pkill scrcpy");
            } else {
                printf("Unknown screen mirror command value: %d\n", msg->value);
            }
        }
        else if (msg->type == 3) {  // Screenshot command
            printf("Received screenshot command.\n");
            /* Execute ADB commands to capture the screenshot, pull it, and remove it */
            system("/usr/bin/adb shell screencap /sdcard/screenshot.png && "
                   "/usr/bin/adb pull /sdcard/screenshot.png /home/user/Desktop/screenshot.png && "
                   "/usr/bin/adb shell rm /sdcard/screenshot.png");
        }
        else {
            printf("Received unknown netlink message type: %d\n", msg->type);
        }
    }

    close(sock_fd);
    return 0;
}
