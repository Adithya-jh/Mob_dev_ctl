#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_MOBDEV 31
#define MAX_PAYLOAD 1024

int main(void)
{
    int sock_fd;
    struct sockaddr_nl src_addr;
    char buffer[MAX_PAYLOAD];
    struct nlmsghdr *nlh;
    int ret;
    int vol_cmd;

    // Create a Netlink socket
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_MOBDEV);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    // Bind to our own Netlink address, subscribing to multicast group 1
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); // Use process PID as unique ID
    src_addr.nl_groups = 1;      // Join multicast group 1

    ret = bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
    if (ret < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    printf("Mobdev Netlink Listener started. Waiting for volume commands...\n");

    // Main loop: receive messages and execute the appropriate ADB command
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ret = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (ret < 0) {
            perror("recv");
            continue;
        }

        // The message header
        nlh = (struct nlmsghdr *)buffer;

        // Extract the payload (an integer)
        memcpy(&vol_cmd, NLMSG_DATA(nlh), sizeof(int));
        if (vol_cmd == 1) {
            printf("Received volume command: up\n");
            system("adb shell input keyevent KEYCODE_VOLUME_UP");
        } else if (vol_cmd == 0) {
            printf("Received volume command: down\n");
            system("adb shell input keyevent KEYCODE_VOLUME_DOWN");
        } else {
            printf("Received unknown volume command: %d\n", vol_cmd);
        }
    }

    close(sock_fd);
    return 0;
}
