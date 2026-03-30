#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>

#include "util.h"
#include "ether.h"

int
main (int argc, char *argv[])
{
    int soc;
    char *ifname;
    struct ifreq ifr;
    struct sockaddr_ll addr;
    uint8_t buf[2048];
    ssize_t n;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <ifname>\n", argv[0]);
        return -1;
    }
    soc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soc == -1) {
        errorf("socket: %s", strerror(errno));
        return -1;
    }
    ifname = argv[1];
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (ioctl(soc, SIOCGIFINDEX, &ifr) == -1) {
        errorf("ioctl [SIOCGIFINDEX]: %s", strerror(errno));
        close(soc);
        return -1;
    }
    memset(&addr, 0x00, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = ifr.ifr_ifindex;
    if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        errorf("bind: %s", strerror(errno));
        close(soc);
        return -1;
    }
    if (ioctl(soc, SIOCGIFFLAGS, &ifr) == -1) {
        errorf("ioctl [SIOCGIFFLAGS]: %s", strerror(errno));
        close(soc);
        return -1;
    }
    ifr.ifr_flags = ifr.ifr_flags | IFF_PROMISC;
    if (ioctl(soc, SIOCSIFFLAGS, &ifr) == -1) {
        errorf("ioctl [SIOCSIFFLAGS]: %s", strerror(errno));
        close(soc);
        return -1;
    }
    infof("waiting for packets from <%s>...", ifname);
    while (1) {
        n = recv(soc, buf, sizeof(buf), 0);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            errorf("recv: %s", strerror(errno));
            close(soc);
            return -1;
        }
        debugf("receive %zd bytes data", n);
        ether_print(buf, n);
    }
    close(soc);
    return 0;
}
