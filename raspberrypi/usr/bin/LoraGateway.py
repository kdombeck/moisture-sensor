#!/usr/bin/env python3

import logging
import requests
import serial
import serial.tools.list_ports
import string
import time

MESSAGE_PREFIX                          = "Got:  "

USB_VENDOR_ID_ADAFRUIT                  = "239A"
USB_PRODUCT_ID_ADAFRUIT_FEATHER_32u4    = "800C"
USB_PRODUCT_ID_ADAFRUIT_FEATHER_M0      = "800B"
USB_PORT_REGEX                          = "%s:(%s|%s)" % (USB_VENDOR_ID_ADAFRUIT, USB_PRODUCT_ID_ADAFRUIT_FEATHER_32u4, USB_PRODUCT_ID_ADAFRUIT_FEATHER_M0)

class LoraGateway:
    logging.basicConfig(format='%(asctime)s %(levelname)s %(message)s', level=logging.INFO)

    def process_message(self, message):
        message = message.strip()
        if self.is_valid_message(message):
            logging.info('processing message: "' + message + '"')
            self.insert_into_db(message[len(MESSAGE_PREFIX):])
            logging.info('processed message: ' + message)
            return True
        else:
            logging.debug('not processing message: "' + message + '"')
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

    def insert_into_db(self, message):
        r = requests.post('http://localhost:8086/write?db=sensordb', data=self.create_body_for_db(message))
        if r.status_code != 204:
            logging.error('Failed to send payload to influxdb: ' + str(r.status_code) + ' ' + r.text)

    def create_body_for_db(self, message):
        host, measure, measurements = message.split(',', 2)
        return measure + ',host=' + host + ' ' + measurements

    def find_device_port(self):
        for port, desc, hwid in sorted(serial.tools.list_ports.grep(USB_PORT_REGEX)):
            logging.info("Found device " + port + ' ' + desc + ' ' + hwid)
            return port

    def main(self):
        ser = None

        while True:
            try:
                if ser is None:
                    ser = serial.Serial(self.find_device_port(), 9600)

                self.process_message(ser.readline().decode("utf-8").strip())
            except serial.SerialException as se:
                logging.warn("Failed to read message from device: " + str(se))
                ser = None
                time.sleep(5)
            except Exception as e:
                logging.exception("failed to process message")

if __name__ == '__main__':
    LoraGateway().main()
