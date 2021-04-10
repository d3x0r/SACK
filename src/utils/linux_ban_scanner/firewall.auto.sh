
#/etc/systemd/network/firewall/firewall.auto.sh

EXT=enp3s0

cd /etc/systemd/network/firewall

mv banlist.auto banlist.issue
touch banlist.auto

for i in $(cat /etc/systemd/network/firewall/banlist.issue); do
#       echo update ban: $i >> /tmp/bans.log
        nft add rule ip filter INPUT iifname "$EXT" ip saddr $i/24 counter drop
#       conntrack -D -s $i >> /tmp/bans.log 2>&1
done

cat banlist.issue >> banlist.issued
rm banlist.issue
