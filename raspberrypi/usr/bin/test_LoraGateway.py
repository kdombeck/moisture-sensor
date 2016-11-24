# coding: utf-8

import unittest
from LoraGateway import LoraGateway

class TestLoraGateway(unittest.TestCase):
    # def test_process_message(self):
        # need to mock 'requests' for this to work
        # self.assertTrue(LoraGateway().process_message('Got:  32B,sensor,moisture1=259,moisture2=248,moisture3=190,battery=0.00'))

    def test_is_valid_message(self):
        self.assertFalse(LoraGateway().is_valid_message('blah'))
        self.assertFalse(LoraGateway().is_valid_message('Got:  €⁄€‹›ﬁﬂ‡°·‚'))
        self.assertFalse(LoraGateway().is_valid_message('Got:  foo,bar'))
        self.assertTrue(LoraGateway().is_valid_message('Got:  32B,sensor,moisture1=259,moisture2=248,moisture3=190,battery=0.00'))

    def test_create_body_for_db(self):
        self.assertEquals(LoraGateway().create_body_for_db('32B,sensor,moisture1=259,moisture2=248'), 'sensor,host=32B moisture1=259,moisture2=248')

if __name__ == '__main__':
    unittest.main()
