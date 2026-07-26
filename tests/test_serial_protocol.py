"""Tests for SerialCommunicator message framing and checksum."""

from unittest.mock import MagicMock

import pytest

from pc_monitor import SerialCommunicator


@pytest.fixture
def connected_comm():
    """Provide a SerialCommunicator with a mocked open serial port."""
    comm = SerialCommunicator(port='/dev/ttyACM0', baudrate=115200)
    comm.serial = MagicMock()
    comm.serial.is_open = True
    return comm


def _decode_sent_message(comm):
    """Helper to extract the string written to the mock serial port."""
    comm.serial.write.assert_called_once()
    return comm.serial.write.call_args[0][0].decode()


def test_send_data_all_optional_fields(connected_comm):
    """Given all fields available, build message with every field and checksum."""
    result = connected_comm.send_data(
        cpu=25.5, ram=60.0, temp=55.5,
        cpu_freq=3.2, gpu_usage=30.0,
        ram_used_gb=8.5, ram_total_gb=16.0,
        fan_rpm=1200, net_down=1.23, net_up=0.45,
        battery_percent=85, power_watts=12.5
    )

    assert result is True
    message = _decode_sent_message(connected_comm)

    assert 'CPU:25.5' in message
    assert 'RAM:60.0' in message
    assert 'TEMP:55.5' in message
    assert 'FREQ:3.2' in message
    assert 'GPU:30.0' in message
    assert 'RAMGB:8.5/16.0' in message
    assert 'FAN:1200' in message
    assert 'NET:1.23,0.45' in message
    assert 'BAT:85' in message
    assert 'POWER:12.5' in message

    expected_sum = (
        25.5 + 60.0 + 55.5 + 3.2 + 30.0 +
        8.5 + 16.0 + 1200 + 1.23 + 0.45 + 85 + 12.5
    )
    expected_checksum = int(expected_sum) % 1000
    assert message.endswith(f'CHK:{expected_checksum}\n')


def test_send_data_required_fields_only(connected_comm):
    """Given only required values, omit optional fields and include NET zeros."""
    result = connected_comm.send_data(cpu=10.0, ram=20.0, temp=30.0)

    assert result is True
    message = _decode_sent_message(connected_comm)

    assert message.startswith('CPU:10.0,RAM:20.0,TEMP:30.0,GPU:0.0,NET:0.00,0.00,CHK:')
    assert 'FREQ' not in message
    assert 'RAMGB' not in message
    assert 'FAN' not in message
    assert 'BAT' not in message
    assert 'POWER' not in message


def test_send_data_disconnected_returns_false():
    """Given no open serial connection, return False without writing."""
    comm = SerialCommunicator(port='/dev/ttyACM0')
    # serial is None by default

    result = comm.send_data(cpu=10.0, ram=20.0, temp=30.0)

    assert result is False


def test_send_data_write_failure_returns_false(connected_comm):
    """Given serial.write raises an exception, return False."""
    connected_comm.serial.write.side_effect = Exception('Serial write failed')

    result = connected_comm.send_data(cpu=10.0, ram=20.0, temp=30.0)

    assert result is False
