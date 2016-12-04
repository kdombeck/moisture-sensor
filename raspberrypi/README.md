# LoRa data collector
This will take the LoRa sensor data and store it in [InfluxDB](https://www.influxdata.com/time-series-platform/influxdb/) and [Grafana](http://grafana.org/) will be used to graph the data.

* Install [Raspbian Lite](https://www.raspberrypi.org/downloads/raspbian/)
* Execute the following command `curl https://api.github.com/repos/kdombeck/moisture-sensor/releases/latest | grep tarball_url | head -n 1 | cut -d '"' -f 4 | xargs curl -L --output source.tar.gz; rm -rf kdombeck-moisture-sensor-*; tar -xzf source.tar.gz; cd kdombeck-moisture-sensor-*/raspberrypi; sudo ./install.sh`
* Plug [LoRa gateway](https://github.com/kdombeck/moisture-sensor/tree/master/gateway) into Raspberry Pi via USB
* Login into Grafana `http://<raspberrypi ip>:3000/dashboard/db/sensor-dashboard` with username `admin` password `admin`
