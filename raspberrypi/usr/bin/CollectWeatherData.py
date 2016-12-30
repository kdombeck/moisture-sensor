#!/usr/bin/env python3

import logging
import requests
import configparser

logging.basicConfig(format='%(asctime)s %(levelname)s %(message)s', level=logging.INFO)

def insert_into_db(message):
    r = requests.post('http://localhost:8086/write?db=sensordb&precision=s', data=message)
    if r.status_code != 204:
        logging.error('Failed to send payload to influxdb: ' + str(r.status_code) + ' ' + r.text)

def create_body_for_db(zipCode, dict):
    return 'weather,host=' + zipCode + ' temp=' + str(dict['main']['temp']) \
        + ',humidity=' + str(dict['main']['humidity']) \
        + ',windSpeed=' + str(dict['wind']['speed']) \
        + ',windDirection=' + str(dict['wind']['deg']) \
        + ' ' + str(dict['dt'])

def find_weather_data(zipCode, apiKey):
    r = requests.get('http://api.openweathermap.org/data/2.5/weather?units=imperial&zip=' + zipCode + ',us&APPID=' + apiKey)
    if r.status_code == 200:
        return r.json()

config = configparser.ConfigParser()
config.read('CollectWeatherData.ini')
zipCode = config['DEFAULT']['ZipCode']
apiKey = config['DEFAULT']['OpenWeatherMapApiKey']

json = find_weather_data(zipCode, apiKey)
message = create_body_for_db(zipCode, json)
insert_into_db(message)
logging.info(message)
