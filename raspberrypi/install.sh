#!/bin/bash -e

apt-get update && apt-get -y upgrade
apt-get install -y apt-transport-https

echo "!!! Installing Python !!!!"
# apt-get install -y python3 python3-pip
pip install pyserial

echo "!!!! Installing InfluxDB !!!!"
curl -sL https://repos.influxdata.com/influxdb.key | apt-key add -
source /etc/os-release
test $VERSION_ID = "7" && echo "deb https://repos.influxdata.com/debian wheezy stable" | tee /etc/apt/sources.list.d/influxdb.list
test $VERSION_ID = "8" && echo "deb https://repos.influxdata.com/debian jessie stable" | tee /etc/apt/sources.list.d/influxdb.list

apt-get update && apt-get install -y influxdb
systemctl start influxdb

curl -i -XPOST http://localhost:8086/query --data-urlencode "q=CREATE DATABASE sensordb"

echo "!!!! Installing Lora Gateway !!!!"
cp usr/bin/loragateway.py /usr/bin/LoraGateway.py
cp lib/systemd/system/loragateway.service /lib/systemd/system/loragateway.service

systemctl daemon-reload
systemctl enable loragateway.service
systemctl start loragateway

echo "!!!! Installing Grafana !!!!"
curl -OL https://github.com/fg2it/grafana-on-raspberry/releases/download/v3.1.1-wheezy-jessie/grafana_3.1.1-1472506485_armhf.deb
apt-get install -y adduser libfontconfig
dpkg -i grafana_3.1.1-1472506485_armhf.deb

# iptables -I INPUT -p tcp --dport 80 -j ACCEPT
# iptables -I INPUT -p tcp --dport 3000 -j ACCEPT
# iptables -t nat -A PREROUTING -p tcp -m tcp --dport 80 -j REDIRECT --to-port 3000
# 
# iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-port 3000

systemctl daemon-reload
systemctl enable grafana-server
systemctl start grafana-server

curl -X POST --user admin:admin localhost:3000/api/datasources -H 'Content-Type: application/json' -d '{"name":"Sensor Database","type":"influxdb","access":"proxy","url":"http://localhost:8086","database":"sensordb"}'
echo "finished installing"
