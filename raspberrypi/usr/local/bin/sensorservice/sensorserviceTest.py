# coding: utf-8

import unittest
import sensorservice

class TestSensorService(unittest.TestCase):
    def setUp(self):
        self.service = sensorservice.SensorService()

    # def test_process_message(self):
        # need to mock 'requests' for this to work
        # self.assertTrue(self.service.processMessage('Got:  32B,sensor,moisture1=259,moisture2=248,moisture3=190,battery=0.00'))

    def test_is_valid_message(self):
        self.assertFalse(self.service.isValidMessage('blah'))
        self.assertFalse(self.service.isValidMessage('Got:  €⁄€‹›ﬁﬂ‡°·‚'))
        self.assertFalse(self.service.isValidMessage('Got:  foo,bar'))
        self.assertTrue(self.service.isValidMessage('Got:  32B,sensor,moisture1=259,moisture2=248,moisture3=190,battery=0.00'))

    def test_create_body_for_db(self):
        self.assertEquals(self.service.createBodyForDB('32B,sensor,moisture1=259,moisture2=248'), 'sensor,host=32B moisture1=259,moisture2=248')

if __name__ == '__main__':
    unittest.main()
