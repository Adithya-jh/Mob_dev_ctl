#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>

#define NETLINK_MOBDEV 31
#define MAX_PAYLOAD 1024

// Define the structure that matches the one used in kernel
struct mobdev_netlink_msg {
    int type;   // 1 = volume, 2 = screen mirroring
    int value;  // For type 2: 1 = start mirror, 0 = stop mirror
};

int main(void)
{
    int sock_fd;
    struct sockaddr_nl src_addr;
    char buffer[MAX_PAYLOAD];
    struct nlmsghdr *nlh;
    struct mobdev_netlink_msg *msg;
    int ret;

    // Create a Netlink socket
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_MOBDEV);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    // Bind to our Netlink address and subscribe to multicast group 1
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();  // Use the process PID as the unique identifier
    src_addr.nl_groups = 1;       // Subscribe to multicast group 1

    ret = bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
    if (ret < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    printf("Netlink listener started for screen mirroring commands.\n");

    // Main loop: receive Netlink messages and process them
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ret = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (ret < 0) {
            perror("recv");
            continue;
        }

        nlh = (struct nlmsghdr *)buffer;
        // Check that the message is large enough
        if (nlh->nlmsg_len < NLMSG_SPACE(sizeof(struct mobdev_netlink_msg))) {
            fprintf(stderr, "Received message too short\n");
            continue;
        }

        msg = (struct mobdev_netlink_msg *)NLMSG_DATA(nlh);

        // Process only screen mirroring commands (type == 2)
        if (msg->type == 2) {
            if (msg->value == 1) {
                printf("Received screen mirroring start command.\n");
                // Launch scrcpy in the background
                ret = system("scrcpy &");
                if (ret < 0)
                    perror("system");
            } else if (msg->value == 0) {
                printf("Received screen mirroring stop command.\n");
                // Stop scrcpy by killing its process
                ret = system("pkill scrcpy");
                if (ret < 0)
                    perror("system");
            } else {
                printf("Received unknown screen mirroring command value: %d\n", msg->value);
            }
        } else {
            // For other types, we simply print a message (or ignore them)
            printf("Received Netlink message with unknown type: %d\n", msg->type);
        }
    }

    close(sock_fd);
    return 0;
}
