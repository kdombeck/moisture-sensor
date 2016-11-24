# coding: utf-8

import unittest
import loragateway

class TestLoraGateway(unittest.TestCase):
    def setUp(self):
        self.service = loragateway.LoraGateway()

    # def test_process_message(self):
        # need to mock 'requests' for this to work
        # self.assertTrue(self.service.process_message('Got:  32B,sensor,moisture1=259,moisture2=248,moisture3=190,battery=0.00'))

    def test_is_valid_message(self):
        self.assertFalse(self.service.is_valid_message('blah'))
        self.assertFalse(self.service.is_valid_message('Got:  €⁄€‹›ﬁﬂ‡°·‚'))
        self.assertFalse(self.service.is_valid_message('Got:  foo,bar'))
        self.assertTrue(self.service.is_valid_message('Got:  32B,sensor,moisture1=259,moisture2=248,moisture3=190,battery=0.00'))

    def test_create_body_for_db(self):
        self.assertEquals(self.service.create_body_for_db('32B,sensor,moisture1=259,moisture2=248'), 'sensor,host=32B moisture1=259,moisture2=248')

if __name__ == '__main__':
    unittest.main()
