# minidisplay_statusd
Tiny daemon to display current mail server status on a $5 OLED screen


Note - this daemon requires the ArduiPi_OLED library, it
can be found here:

https://github.com/hallard/ArduiPi_OLED

http://hallard.me/adafruit-oled-display-driver-for-pi/


Just in case, I forked the library here:
https://github.com/vonj/ArduiPi_OLED


The daemon expects a file to be up to date: /var/tmp/mails_received.txt
The first word in this file, should the number of mails received today.


To do that, I added this crontab:

    0,5,10,15,20,25,30,35,40,45,50,55 *    *   *   *     /usr/local/sbin/mails_received.sh

/usr/local/sbin/mails_received.sh itself contains:

    #!/bin/sh

    /usr/sbin/pflogsumm -d today /var/log/mail.log |grep received |head -1  > /var/tmp/mails_received.txt_new
    mv /var/tmp/mails_received.txt_new /var/tmp/mails_received.txt
    chmod a+r /var/tmp/mails_received.txt




