#!/usr/bin/env python

import logging
import requests
import serial
import string
import time

MESSAGE_PREFIX = "Got:  "

class LoraGateway:
    logging.basicConfig(format='%(asctime)s %(levelname)s %(message)s', level=logging.INFO)

    def process_message(self, message):
        message = message.strip()
        if self.is_valid_message(message):
            logging.info('processing message: ' + message)
            self.insert_df(message[len(MESSAGE_PREFIX):])
            logging.info('processed message: ' + message)
            return True
        else:
            logging.debug('not processing message: ' + message)
            return False

    def is_valid_message(self, message):
        if not message.startswith(MESSAGE_PREFIX):
            logging.debug('message doesn\'t start with "' + MESSAGE_PREFIX + '": ' + message)
            return False
        elif not self.is_ascii(message):
            logging.warn('message is not ASCII: ' + message)
            return False
        elif message.count(',') <= 2:
            logging.warn('message does not have enough comma\'s: ' + message)
            return False
        else:
            return True

    def is_ascii(self, message):
        for c in message:
            if c not in string.printable:
                return False
        return True

    def insert_db(self, message):
        r = requests.post("http://localhost:8086/write?db=sensordb", data=self.create_body_for_db(message))
        if r.status_code != 204:
            logging.error('Failed to send payload to influxdb: ' + str(r.status_code) + ' ' + r.text)

    def create_body_for_db(self, message):
        host, measure, measurements = message.split(',', 2)
        return measure + ',host=' + host + ' ' + measurements

    def main(self):
        ser = None

        while True:
            try:
                if ser is None:
                    ser = serial.Serial('/dev/ttyACM0', 9600)

                self.process_message(ser.readline())
            except serial.SerialException as se:
                logging.warn("Failed to read message from device: " + str(se))
                ser = None
                time.sleep(5)
            except Exception as e:
                logging.exception("failed to process message")

if __name__ == '__main__':
    LoraGateway().main()
