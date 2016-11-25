# coding: utf-8

import requests
import unittest

from LoraGateway import LoraGateway
from mock import Mock, patch

class TestProcessMessage(unittest.TestCase):
    @patch('requests.post')
    def test_processing_invalid_message(self, mock_request):
        LoraGateway().process_message('Got:  32B,sensor')
        assert not mock_request.called

    @patch('requests.post')
    def test_processing_valid_message(self, mock_request):
        mock_request.return_value = Mock(spec=requests.Response, status_code=204, text='success')
        LoraGateway().process_message('Got:  32B,sensor,moisture1=259,moisture2=248,moisture3=190,battery=0.00')
        assert mock_request.called
        mock_request.assert_called_with('http://localhost:8086/write?db=sensordb', data='sensor,host=32B moisture1=259,moisture2=248,moisture3=190,battery=0.00')

class TestIsValidMessage(unittest.TestCase):
    def test_message_with_invalid_prefix(self):
        self.assertFalse(LoraGateway().is_valid_message('blah'))

    def test_message_with_non_ascii_payload(self):
        self.assertFalse(LoraGateway().is_valid_message('Got:  €⁄€‹›ﬁﬂ‡°·‚'))

    def test_message_with_too_few_commas(self):
        self.assertFalse(LoraGateway().is_valid_message('Got:  32B,sensor'))

    def test_valid_message(self):
        self.assertTrue(LoraGateway().is_valid_message('Got:  32B,sensor,moisture1=259,moisture2=248,moisture3=190,battery=0.00'))

class TestCreateBodyForDb(unittest.TestCase):
    def test_parsing_message(self):
        self.assertEquals(LoraGateway().create_body_for_db('32B,sensor,moisture1=259,moisture2=248'), 'sensor,host=32B moisture1=259,moisture2=248')

if __name__ == '__main__':
    unittest.main()
