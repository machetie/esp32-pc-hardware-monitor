"""Tests for SystemMonitor network rolling-window speed calculation."""

import builtins
from unittest.mock import MagicMock, mock_open, patch

import pytest

from pc_monitor import SystemMonitor


@pytest.fixture
def monitor():
    """Provide a SystemMonitor with filesystem discovery disabled."""
    with (
        patch.object(SystemMonitor, '_find_k10temp'),
        patch.object(SystemMonitor, '_find_fan_sensor'),
        patch.object(SystemMonitor, '_find_battery'),
        patch.object(SystemMonitor, '_find_network_interface'),
        patch.object(SystemMonitor, '_find_gpu_device'),
    ):
        m = SystemMonitor()
        m.network_interface = 'eth0'
        yield m


def _mock_network_files(monkeypatch, rx_bytes, tx_bytes, iface='eth0'):
    """Mock os.path.exists and open for the target interface's rx/tx counters."""
    rx_path = f'/sys/class/net/{iface}/statistics/rx_bytes'
    tx_path = f'/sys/class/net/{iface}/statistics/tx_bytes'

    def fake_exists(path):
        return path in (rx_path, tx_path)

    def fake_open(path, *args, **kwargs):
        if path == rx_path:
            return mock_open(read_data=str(rx_bytes))(path, *args, **kwargs)
        if path == tx_path:
            return mock_open(read_data=str(tx_bytes))(path, *args, **kwargs)
        raise FileNotFoundError(path)

    monkeypatch.setattr('os.path.exists', fake_exists)
    monkeypatch.setattr(builtins, 'open', fake_open)


def test_get_network_speed_happy_path(monitor, monkeypatch):
    """Given two samples 1 second apart, return correct MB/s rates."""
    # Given: mocked counters and two timestamps 1s apart
    _mock_network_files(monkeypatch, rx_bytes=0, tx_bytes=0)
    monkeypatch.setattr('time.time', MagicMock(side_effect=[1000.0, 1001.0]))

    # When: first sample is collected
    result1 = monitor.get_network_speed()
    # Then: need two samples, so first call returns zeros
    assert result1 == (0.0, 0.0)

    # Given: counters increased by 1 MiB in each direction
    _mock_network_files(monkeypatch, rx_bytes=1048576, tx_bytes=1048576)
    # When: second sample is collected
    result2 = monitor.get_network_speed()
    # Then: 1 MiB / 1s = 1.0 MB/s rounded to 2 decimals
    assert result2 == (1.0, 1.0)


def test_get_network_speed_single_sample_returns_zero(monitor, monkeypatch):
    """Given only one sample, return (0.0, 0.0) until a second is available."""
    # Given: one sample in history
    _mock_network_files(monkeypatch, rx_bytes=100, tx_bytes=200)
    monkeypatch.setattr('time.time', MagicMock(return_value=1000.0))

    # When/Then: single sample cannot produce a rate
    assert monitor.get_network_speed() == (0.0, 0.0)


def test_get_network_speed_evicts_samples_outside_window(monitor, monkeypatch):
    """Given samples outside the 3s rolling window, only use in-window samples."""
    # Given: window is 3.0s; call sequence: t=0, 2, 5
    # At t=5, t=0 sample should be evicted, leaving t=2 and t=5 (3s window)
    _mock_network_files(monkeypatch, rx_bytes=0, tx_bytes=0)
    monkeypatch.setattr('time.time', MagicMock(side_effect=[0.0, 2.0, 5.0]))

    # First two samples establish a baseline over 2s
    monitor.get_network_speed()
    _mock_network_files(monkeypatch, rx_bytes=2097152, tx_bytes=1048576)
    monitor.get_network_speed()  # rate over 2s

    # Third sample: only t=2 to t=5 remain (window 3s)
    _mock_network_files(monkeypatch, rx_bytes=5242880, tx_bytes=1572864)
    result = monitor.get_network_speed()

    # Then: effective window is 3s; rx diff = 5242880 - 2097152 = 3145728 bytes
    # rate = 3145728 / 3 / 1048576 = 1.0 MB/s
    # tx diff = 1572864 - 1048576 = 524288 bytes
    # rate = 524288 / 3 / 1048576 = 0.17 MB/s (rounded)
    assert result == (1.0, 0.17)


def test_get_network_speed_recovers_when_interface_appears(monitor, monkeypatch):
    """Given no interface at startup, rediscover it and sample its counters."""
    # Given: startup discovery missed the interface, which is available on retry
    monitor.network_interface = None

    def discover_interface():
        monitor.network_interface = 'eth0'

    monkeypatch.setattr(monitor, '_find_network_interface', discover_interface)
    _mock_network_files(monkeypatch, rx_bytes=0, tx_bytes=0)
    monkeypatch.setattr('time.time', MagicMock(side_effect=[1000.0, 1001.0]))

    # When: the first and second samples are collected after rediscovery
    first_result = monitor.get_network_speed()
    _mock_network_files(monkeypatch, rx_bytes=1048576, tx_bytes=1048576)
    second_result = monitor.get_network_speed()

    # Then: the baseline is zero and the next sample reports a positive rate
    assert first_result == (0.0, 0.0)
    assert second_result == (1.0, 1.0)


def test_get_network_speed_no_interface_after_retry(monitor, monkeypatch):
    """Given no interface remains available, return zero without reading counters."""
    # Given: no interface is configured and rediscovery finds none
    monitor.network_interface = None
    discover_interface = MagicMock()
    monkeypatch.setattr(monitor, '_find_network_interface', discover_interface)

    # When: network speed is requested
    result = monitor.get_network_speed()

    # Then: the retry occurs once and no rate is fabricated
    assert result == (0.0, 0.0)
    discover_interface.assert_called_once()


def test_get_network_speed_missing_stats_files(monitor, monkeypatch):
    """Given statistics files missing, return (0.0, 0.0)."""
    monkeypatch.setattr('os.path.exists', lambda _path: False)
    assert monitor.get_network_speed() == (0.0, 0.0)


def test_get_network_speed_negative_diff_clamped(monitor, monkeypatch):
    """Given counter wraps or resets, clamp rate to 0.0 instead of negative."""
    # Given: first sample at higher counter value
    _mock_network_files(monkeypatch, rx_bytes=2000000, tx_bytes=2000000)
    monkeypatch.setattr('time.time', MagicMock(side_effect=[1000.0, 1001.0]))
    monitor.get_network_speed()

    # When: second sample has a lower counter value
    _mock_network_files(monkeypatch, rx_bytes=1000000, tx_bytes=1000000)
    result = monitor.get_network_speed()

    # Then: negative diff is clamped to 0.0
    assert result == (0.0, 0.0)


def test_get_network_speed_zero_window_returns_zero(monitor, monkeypatch):
    """Given identical timestamps, avoid division by zero and return (0.0, 0.0)."""
    _mock_network_files(monkeypatch, rx_bytes=0, tx_bytes=0)
    monkeypatch.setattr('time.time', MagicMock(return_value=1000.0))

    monitor.get_network_speed()
    _mock_network_files(monkeypatch, rx_bytes=1048576, tx_bytes=1048576)
    result = monitor.get_network_speed()

    assert result == (0.0, 0.0)


def test_get_network_speed_invalid_stats_value(monitor, monkeypatch):
    """Given invalid content in the stats file, return (0.0, 0.0)."""
    rx_path = '/sys/class/net/eth0/statistics/rx_bytes'
    tx_path = '/sys/class/net/eth0/statistics/tx_bytes'

    def fake_exists(path):
        return path in (rx_path, tx_path)

    def fake_open(path, *args, **kwargs):
        if path == rx_path:
            return mock_open(read_data='not_a_number')(path, *args, **kwargs)
        if path == tx_path:
            return mock_open(read_data='0')(path, *args, **kwargs)
        raise FileNotFoundError(path)

    monkeypatch.setattr('os.path.exists', fake_exists)
    monkeypatch.setattr(builtins, 'open', fake_open)

    assert monitor.get_network_speed() == (0.0, 0.0)
