#!/bin/bash -e

apt-get update && apt-get -y upgrade
apt-get install -y apt-transport-https

echo "!!! Installing Python !!!!"
pip install pyserial

echo "!!!! Installing InfluxDB !!!!"
curl -sL https://repos.influxdata.com/influxdb.key | apt-key add -
source /etc/os-release
test $VERSION_ID = "7" && echo "deb https://repos.influxdata.com/debian wheezy stable" | tee /etc/apt/sources.list.d/influxdb.list
test $VERSION_ID = "8" && echo "deb https://repos.influxdata.com/debian jessie stable" | tee /etc/apt/sources.list.d/influxdb.list

apt-get update && apt-get install -y influxdb
systemctl restart influxdb
sleep 15

curl -i -XPOST http://localhost:8086/query --data-urlencode "q=CREATE DATABASE sensordb"

echo "!!!! Installing Lora Gateway !!!!"
cp usr/bin/LoraGateway.py /usr/bin/LoraGateway.py
cp lib/systemd/system/loragateway.service /lib/systemd/system/loragateway.service

systemctl daemon-reload
systemctl enable loragateway.service
systemctl restart loragateway

echo "!!!! Installing Weather Collection !!!!"
cp usr/bin/CollectWeatherData.* /usr/bin/.

echo 'Enter in your zip code:'
read zipCode
sed -i "s/ZipCode =/ZipCode = $zipCode/" /usr/bin/CollectWeatherData.ini

echo 'Enter in your http://openweathermap.org/ api key:'
read apiKey
sed -i "s/OpenWeatherMapApiKey =/OpenWeatherMapApiKey = $apiKey/" /usr/bin/CollectWeatherData.ini

echo '0,30 * * * * root cd /usr/bin; ./CollectWeatherData.py' > /etc/cron.d/collectWeatherData

echo "!!!! Installing Grafana !!!!"
curl -OL https://github.com/fg2it/grafana-on-raspberry/releases/download/v4.0.2/grafana_4.0.2-1481228559_armhf.deb
apt-get install -y adduser libfontconfig
dpkg -i grafana_4.0.2-1481228559_armhf.deb

systemctl daemon-reload
systemctl enable grafana-server
systemctl restart grafana-server
sleep 10

curl -X POST --user admin:admin localhost:3000/api/datasources -H 'Content-Type: application/json' -d '{"name":"Sensor Database","type":"influxdb","access":"proxy","url":"http://localhost:8086","database":"sensordb"}'
curl -X POST --user admin:admin localhost:3000/api/dashboards/db -H 'Content-Type: application/json' -d @grafanaDashboard.json

echo "finished installing"
