#!/usr/bin/env python

import serial
import string
import requests

MESSAGE_PREFIX = "Got:  "

class SensorService:

    def processMessage(self, message):
        message = message.strip()
        if self.isValidMessage(message):
            print('processing message: ' + message)
            self.insertDB(message[len(MESSAGE_PREFIX):])
            return True
        else:
            print('not processing message: ' + message)
            return False

    def isValidMessage(self, message):
        if message.startswith(MESSAGE_PREFIX) == False:
            print('message doesn\'t start with "' + MESSAGE_PREFIX + '": ' + message)
            return False
        elif self.isAscii(message) == False:
            print('message is not ASCII: ' + message)
            return False
        elif message.count(',') <= 2:
            print('message does not have enough comma\'s: ' + message)
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
            print('Failed to send payload to influxdb: ' + str(r.status_code) + ' ' + r.text)

    def createBodyForDB(self, message):
        host, measure, measurements = message.split(',', 2)
        return measure + ',host=' + host + ' ' + measurements

    def main(self):
        ser = serial.Serial('/dev/ttyACM0', 9600)
        while True:
             self.processMessage(ser.readline())

if __name__ == '__main__':
    SensorService().main()
