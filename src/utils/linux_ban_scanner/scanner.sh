killall linux_syslog_scanner.portable
killall httpd_access_scanner.portable

journalctl -f | /usr/local/bin/linux_syslog_scanner.portable > /tmp/syslog_bans.log &
tail -f /var/log/httpd/access_log | /usr/local/bin/httpd_access_scanner.portable >/tmp/httpd_bans.log &
tail -f /var/log/httpd/d3x0r-access | /usr/local/bin/httpd_access_scanner.portable >/tmp/httpd_bans_d3x.log &
tail -f /var/log/httpd/pub-access | /usr/local/bin/httpd_access_scanner.portable >/tmp/httpd_bans_pub.log &
tail -f /var/log/httpd/sp-access | /usr/local/bin/httpd_access_scanner.portable >/tmp/httpd_bans_sp.log &



