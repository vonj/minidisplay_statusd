#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

#include <time.h>
#include <getopt.h>
#include <sys/time.h>


// Instantiate the display
ArduiPi_OLED display;


// Config Option
struct s_opts
{
    int oled;
    int verbose;
};


// default options values
s_opts opts = {
    OLED_ADAFRUIT_I2C_128x64,	// Default oled
    false                       // Not verbose
};

static void init_oled()
{
    // SPI change parameters to fit to your LCD
    if (display.oled_is_spi_proto(opts.oled)) {
        if (!display.init(OLED_SPI_DC, OLED_SPI_RESET, OLED_SPI_CS, opts.oled)) {
            exit(EXIT_FAILURE);
        }
    } else {
        if (!display.init(OLED_I2C_RESET, opts.oled)) {
            exit(EXIT_FAILURE);
        }
    }
}


static void get_ip_address(char ip[16])
{
    struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;  
        }

        s = getnameinfo(ifa->ifa_addr, sizeof (struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if ((0 == strcmp(ifa->ifa_name, "eth0")) && (ifa->ifa_addr->sa_family == AF_INET)) {
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return;
            }
            strcpy(ip, host);
            freeifaddrs(ifaddr);
            return;
        }
    }

    freeifaddrs(ifaddr);
}


static bool read_internet_status()
{
    return !system("nc -w 10 -z 10.8.0.1 22");
}

static int read_mails_received()
{
    int mails_received = 0;
    FILE* fp = fopen("/var/tmp/mails_received.txt", "r");
    if (!fp) {
        return -1;
    }

    fscanf(fp, "%d", &mails_received);
    fclose(fp);

    return mails_received;
}


static void show_status()
{
    time_t now;
    struct tm * timeinfo;
    double percent = 0;
    struct timespec tp;
    static bool up = true;
    const double cycle = 2; // seconds
    double half_cycle = cycle / 2.0;
    static bool internet_up = false;
    static int mails_received = -99;
    char ip[16];

    usleep(70000);

    get_ip_address(ip);

    display.clearDisplay();
    display.setCursor(0, 0);
    time(&now);
    timeinfo = localtime(&now);
    clock_gettime(CLOCK_REALTIME, &tp);
    double secs_fraction = (double)(tp.tv_nsec) / 1000000000.0;
    int where_in_cycle = tp.tv_sec % (int)cycle;
    up = where_in_cycle < half_cycle;
    double where_in_cycle_d = (double)where_in_cycle + secs_fraction;
    
    if (up) {
        percent = 100.0 * (where_in_cycle_d / half_cycle);
    } else {
        percent = 100.0 - 100.0 * ((double)(where_in_cycle_d - half_cycle) / half_cycle);
    }

    display.setTextSize(2);
    display.printf("%2.2d:%2.2d:%2.2d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    display.setTextSize(1);

    // Only do expensive stuff at startup, then once a minute
    if ((-99 == mails_received) || (0 == (tp.tv_sec % 60))) {
        internet_up = read_internet_status();
        mails_received = read_mails_received();
    }

    display.printf("IP: %s\n", ip);
    display.printf("Network %s\n", internet_up ? "UP" : "DOWN");
    display.printf("%d mails today.\n", mails_received);

    display.drawVerticalBargraph(121, 0, 6, (int16_t)display.height(), WHITE, (int)percent);
    display.display();
}

static void close_display()
{
    // Free PI GPIO ports
    display.close();
}

    // Free PI GPIO ports
int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp("-d", argv[1])) {
        if (daemon(0, 0)) {
            fprintf(stderr, "Daemon call failed.\n");

            return 1;
        }
    }

    atexit(close_display);

    init_oled();

    display.begin();

    display.setTextColor(WHITE);

    while (1) {
        show_status();
    }

    return 0;
}

