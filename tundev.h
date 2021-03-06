#ifndef TUNDEV_H
#define TUNDEV_H

#ifdef __APPLE__
#include <net/if_utun.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#elif defined __linux__
#include <linux/if_tun.h>
#include <net/route.h>
#endif
#include <net/if.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <string.h>
#include <fstream>
#define TUN_NET "10.0.0.%d"
using namespace std;

extern string m_sRootPassword = "";
extern string m_sDefaultRoute = ""; // system default route
extern string m_sDevice = "";       // internet device

namespace tundev
{
#ifdef __APPLE__
    static int open_tun_socket ()
    {
        struct sockaddr_ctl addr;
        struct ctl_info info;
        char ifname[10];
        socklen_t ifname_len = sizeof(ifname);
        int fd = -1;
        int err = 0;

        fd = socket (PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
        if (fd < 0)
        {
            return fd;
        }

        bzero(&info, sizeof (info));
        strncpy(info.ctl_name, UTUN_CONTROL_NAME, MAX_KCTL_NAME);

        err = ioctl(fd, CTLIOCGINFO, &info);
        if (err != 0) goto on_error;

        addr.sc_len = sizeof(addr);
        addr.sc_family = AF_SYSTEM;
        addr.ss_sysaddr = AF_SYS_CONTROL;
        addr.sc_id = info.ctl_id;
        addr.sc_unit = 101;

        err = connect(fd, (struct sockaddr *)&addr, sizeof (addr));
        if (err != 0)
        {
            goto on_error;
        }

        // TODO: forward ifname (we just expect it to be utun0 for now...)
        err = getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, &ifname_len);
        if (err != 0)
            goto on_error;

        err = fcntl(fd, F_SETFL, O_NONBLOCK);
        if (err != 0)
            goto on_error;

        fcntl(fd, F_SETFD, FD_CLOEXEC);
        if (err != 0)
            goto on_error;

    on_error:
        if (err != 0)
        {
            close(fd);
            return err;
        }
        //strncpy(if_name, ifname, if_name_size);
        return fd;
    }

    static string get_default_gateway()
    {
        std::array<char, 1024> buffer;
        std::string line;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("route -n get default", "r"), pclose);

        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            line = buffer.data();
            string sFindStrGtw = "gateway: ";
            if (line.find(sFindStrGtw) != string::npos) // this is masquerade line
            {
                string gateway = line.substr(line.find(sFindStrGtw) + sFindStrGtw.length());
                gateway.erase(std::remove(gateway.begin(), gateway.end(), '\n'), gateway.end());
                return gateway;
            }
        }
        return "";
    }

    static void protect (string sServerAddress)
    {
        auto defaultGW = get_default_gateway();
        string sCmd = string("echo ") + m_sRootPassword + " | " + string("sudo -S ") + string("route add ") + sServerAddress + " " + defaultGW;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(sCmd.c_str(), "r"), pclose);
        
        std::array<char, 1024> buffer;
        std::string line;

        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            line = buffer.data();
        }
    }

    static void unprotect(string sAddr)
    {
        string sCmd = string("echo ") + m_sRootPassword + " | " + string("sudo -S ") + string("route delete ") + sAddr;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(sCmd.c_str(), "r"), pclose);
        
        std::array<char, 1024> buffer;
        std::string line;

        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            line = buffer.data();
        }
    }

    static void set_tun_address(string sAddr)
    {
        m_sDefaultRoute = get_default_gateway();
        string sCmd = string("echo ") + m_sRootPassword + " | " + string("sudo -S ") + string("ifconfig utun100 up ") + sAddr + " " + sAddr;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(sCmd.c_str(), "r"), pclose);
        
        std::array<char, 1024> buffer;
        std::string line;

        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            line = buffer.data();
        }
    }

    static void unset_default_route()
    {
        string sCmd = string("echo ") + m_sRootPassword + " | " + string("sudo -S ") + string("route -n delete default");
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(sCmd.c_str(), "r"), pclose);
        
        std::array<char, 1024> buffer;
        std::string line;

        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            line = buffer.data();
        }
    }

    static void set_default_route(string sAddr)
    {
        unset_default_route();
        string sCmd = string("echo ") + m_sRootPassword + " | " + string("sudo -S ") + string("route add default ") + sAddr;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(sCmd.c_str(), "r"), pclose);
        
        std::array<char, 1024> buffer;
        std::string line;

        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            line = buffer.data();
        }
    }


#elif defined __linux__

    static int get_default_gateway(char* out_gateway, size_t len, char* device, size_t dev_len)
    {
        long destination, gateway;
        char iface[IF_NAMESIZE];

        FILE* file = fopen("/proc/net/route", "r");
        if (!file)
        {
            perror("could not open file:");
            return -1;
        }

        char buffer[2048];

        while (fgets(buffer, sizeof(buffer), file))
        {
            if (sscanf(buffer, "%s %lx %lx", iface, &destination, &gateway) == 3)
            {
                if (destination == 0)  /* default */
                {
                    char *gw = inet_ntoa(*(struct in_addr *) &gateway);
                    strncpy(out_gateway, gw, len);
                    strncpy(device, iface, dev_len);
                    fclose(file);
                    break;
                }
            }
        }

        return 0;
    }

    static void unprotect (string sServerAddress)
    {
        int sockfd;
        struct rtentry rt;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            perror("socket creation failed\n");
            return;
        }

        struct sockaddr_in *sockinfo = (struct sockaddr_in *)&rt.rt_gateway;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr(m_sDefaultRoute.c_str());

        sockinfo  = (struct sockaddr_in *)&rt.rt_dst;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr(sServerAddress.c_str());

        sockinfo = (struct sockaddr_in *)&rt.rt_genmask;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr("255.255.255.255");

        rt.rt_dev = (char*)m_sDevice.c_str();
        rt.rt_flags = RTF_UP | RTF_GATEWAY | RTF_HOST;
        rt.rt_metric = 0;

        if(ioctl(sockfd, SIOCDELRT, &rt) < 0 )
        {
            perror("ioctl");
        }
    }

    static void protect (string sServerAddress)
    {
        int sockfd;
        struct rtentry rt;

        char gateway[128] = { 0 };
        char device[128] = { 0 };
        if (get_default_gateway(gateway, sizeof(gateway), device, sizeof(device)) != 0)
        {
            perror("could not get default gateway\n");
            return;
        }

        m_sDevice = device;
        m_sDefaultRoute = gateway;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            perror("socket creation failed\n");
            return;
        }

        struct sockaddr_in *sockinfo = (struct sockaddr_in *)&rt.rt_gateway;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr(gateway);

        sockinfo = (struct sockaddr_in *)&rt.rt_dst;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr(sServerAddress.c_str());

        sockinfo = (struct sockaddr_in *)&rt.rt_genmask;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr("255.255.255.255");

        rt.rt_dev = (char*)m_sDevice.c_str();
        rt.rt_flags = RTF_UP | RTF_GATEWAY;
        rt.rt_metric = 0;

        if(ioctl(sockfd, SIOCADDRT, &rt) < 0 )
        {
            perror("ioctl");
        }
    }

    static void unset_default_route()
    {
        int sockfd;
        struct rtentry rt;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            perror("socket creation failed\n");
            return;
        }

        struct sockaddr_in *sockinfo = (struct sockaddr_in *)&rt.rt_gateway;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr("0.0.0.0");

        sockinfo = (struct sockaddr_in *)&rt.rt_dst;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = INADDR_ANY;

        sockinfo = (struct sockaddr_in *)&rt.rt_genmask;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = INADDR_ANY;

        rt.rt_flags = RTF_UP | RTF_GATEWAY;
        rt.rt_dev = "jvpndesktop0";
        rt.rt_metric = 0;

        if(ioctl(sockfd, SIOCDELRT, &rt) < 0 )
        {
            perror("ioctl");
        }
    }

    static void set_default_route(string sRoute)
    {
        unset_default_route();
        int sockfd;
        struct rtentry rt;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            perror("socket creation failed\n");
            return;
        }

        struct sockaddr_in *sockinfo = (struct sockaddr_in *)&rt.rt_gateway;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr(sRoute.c_str());

        sockinfo = (struct sockaddr_in *)&rt.rt_dst;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = INADDR_ANY;

        sockinfo = (struct sockaddr_in *)&rt.rt_genmask;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = INADDR_ANY;

        rt.rt_flags = RTF_UP | RTF_GATEWAY;
        rt.rt_dev = "jvpndesktop0";
        rt.rt_metric = 0;

        if(ioctl(sockfd, SIOCADDRT, &rt) < 0 )
        {
            perror("ioctl");
        }

        return;
    }

    static int set_tun_address (string sTunAddress)
    {
        // set address
        struct sockaddr_in addr;
        struct ifreq ifr;

        memset(&addr, 0, sizeof(addr));
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, "jvpndesktop0", IFNAMSIZ);

        addr.sin_family = AF_INET;
        int s = socket(addr.sin_family, SOCK_DGRAM, 0);
        addr.sin_family = AF_INET;

        if( inet_pton(addr.sin_family, sTunAddress.c_str(), &addr.sin_addr) != 1)
        {
            close(s);
            return -1;
        }

        ifr.ifr_addr = *(struct sockaddr*) &addr;
        if (ioctl(s,SIOCSIFADDR,(caddr_t)&ifr)== -1)
        {
            close(s);
            return -1;
        }

        // up the interface
        ifr.ifr_flags |= IFF_UP;
        if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr)== -1)
        {
            close(s);
            return -1;
        }
    }

    static int open_tun_socket ()
    {
        string sTunDevName = "jvpndesktop0";
        struct ifreq ifr;
        int fd, err, i;

        memset(&ifr, 0, sizeof(ifr));

        ifr.ifr_flags = IFF_NO_PI | IFF_TUN | IFF_UP;
        strncpy(ifr.ifr_name, sTunDevName.c_str(), IFNAMSIZ);

        if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
        {
            close(fd);
            return -1;
        }

        if (ioctl(fd, TUNSETIFF, (void*)&ifr))
        {
            close(fd);
            return -1;
        }

        return fd;
    }

#endif // ifdef __APPLE__

    static void restore_default_route()
    {
        if (!m_sDefaultRoute.empty())
        {
            set_default_route(m_sDefaultRoute);
        }
    }
}


#endif // TUNDEV_H
