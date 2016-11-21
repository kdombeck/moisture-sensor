#!/bin/bash -e

apt-get update && apt-get -y upgrade
apt-get install -y apt-transport-https

echo "install Python3"
apt-get install -y python3 python3-pip
pip3 install pyserial
# pip install pyserial -- for python2

echo "install InfluxDB"
curl -sL https://repos.influxdata.com/influxdb.key | apt-key add -
source /etc/os-release
test $VERSION_ID = "7" && echo "deb https://repos.influxdata.com/debian wheezy stable" | tee /etc/apt/sources.list.d/influxdb.list
test $VERSION_ID = "8" && echo "deb https://repos.influxdata.com/debian jessie stable" | tee /etc/apt/sources.list.d/influxdb.list

apt-get update && apt-get install -y influxdb
systemctl start influxdb

echo "install Grafana"
curl -OL https://github.com/fg2it/grafana-on-raspberry/releases/download/v3.1.1-wheezy-jessie/grafana_3.1.1-1472506485_armhf.deb
apt-get install -y adduser libfontconfig
dpkg -i grafana_3.1.1-1472506485_armhf.deb

#sed -i 's/;http_port = 3000/http_port = 80/g' /etc/grafana/grafana.ini
iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-port 3000

systemctl daemon-reload
systemctl enable grafana-server
systemctl start grafana-server

echo "finished installing"
