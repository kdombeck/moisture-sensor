#!/usr/bin/env python

import serial
import string
import requests
import logging

MESSAGE_PREFIX = "Got:  "

class SensorService:
    logging.basicConfig(format='%(asctime)s %(levelname)s %(message)s', level=logging.INFO)

    def processMessage(self, message):
        message = message.strip()
        if self.isValidMessage(message):
            logging.info('processing message: ' + message)
            self.insertDB(message[len(MESSAGE_PREFIX):])
            return True
        else:
            logging.debug('not processing message: ' + message)
            return False

    def isValidMessage(self, message):
        if message.startswith(MESSAGE_PREFIX) == False:
            logging.debug('message doesn\'t start with "' + MESSAGE_PREFIX + '": ' + message)
            return False
        elif self.isAscii(message) == False:
            logging.warn('message is not ASCII: ' + message)
            return False
        elif message.count(',') <= 2:
            logging.warn('message does not have enough comma\'s: ' + message)
            return False
        else:
            return True

    def isAscii(self, message):
        for c in message:
            if c not in string.printable:
                return False
        return True

    def insertDB(self, message):
        r = requests.post("http://localhost:8086/write?db=sensordb", data=self.createBodyForDB(message))
        if r.status_code != 204:
            logging.error('Failed to send payload to influxdb: ' + str(r.status_code) + ' ' + r.text)

    def createBodyForDB(self, message):
        host, measure, measurements = message.split(',', 2)
        return measure + ',host=' + host + ' ' + measurements

    def main(self):
        ser = serial.Serial('/dev/ttyACM0', 9600)
        while True:
            try:
                self.processMessage(ser.readline())
            except Exception as e:
                logging.exception("failed to process message")

if __name__ == '__main__':
    SensorService().main()
