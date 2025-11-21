#!/usr/bin/env python3
import shutil
import subprocess
import sys
import time
from typing import Dict, List, Optional, Tuple

from pc_monitor import SerialCommunicator


class WindowsSystemMonitor:
    def __init__(self) -> None:
        self._ensure_wmic_available()
        self._network_ignore_tokens = (
            "loopback",
            "isatap",
            "teredo",
            "vmware",
            "virtual",
            "bluetooth",
        )

    def _ensure_wmic_available(self) -> None:
        if shutil.which("wmic") is None:
            raise RuntimeError("WMIC command not found. Install legacy WMIC components or use an alternative.")

    def _run_wmic_values(self, args: List[str]) -> Dict[str, List[str]]:
        try:
            result = subprocess.run([
                "wmic",
                *args,
            ], capture_output=True, text=True, check=True)
        except FileNotFoundError as exc:
            raise RuntimeError("WMIC command unavailable") from exc
        except subprocess.CalledProcessError as exc:
            raise RuntimeError(f"WMIC call failed: {' '.join(args)}") from exc

        lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
        values: Dict[str, List[str]] = {}
        for line in lines:
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            key_lower = key.strip().lower()
            values.setdefault(key_lower, []).append(value.strip())
        return values

    def _run_wmic_csv(self, args: List[str]) -> List[Dict[str, str]]:
        try:
            result = subprocess.run([
                "wmic",
                *args,
            ], capture_output=True, text=True, check=True)
        except FileNotFoundError as exc:
            raise RuntimeError("WMIC command unavailable") from exc
        except subprocess.CalledProcessError:
            return []

        lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
        if len(lines) < 2:
            return []

        headers = [section.strip().lower() for section in lines[0].split(",")]
        rows: List[Dict[str, str]] = []
        for line in lines[1:]:
            parts = [section.strip() for section in line.split(",")]
            if len(parts) != len(headers):
                continue
            row: Dict[str, str] = {}
            for index, header in enumerate(headers):
                row[header] = parts[index]
            rows.append(row)
        return rows

    def get_cpu_usage(self) -> float:
        values = self._run_wmic_values(["cpu", "get", "loadpercentage", "/value"])
        try:
            percentages = [float(item) for item in values.get("loadpercentage", []) if item]
        except ValueError:
            return 0.0
        if not percentages:
            return 0.0
        return round(sum(percentages) / len(percentages), 1)

    def get_cpu_frequency(self) -> float:
        values = self._run_wmic_values(["cpu", "get", "currentclockspeed", "/value"])
        speeds: List[float] = []
        for entry in values.get("currentclockspeed", []):
            try:
                speeds.append(float(entry))
            except ValueError:
                continue
        if not speeds:
            return 0.0
        frequency_ghz = (sum(speeds) / len(speeds)) / 1000.0
        return round(frequency_ghz, 1)

    def get_ram_usage(self) -> Tuple[float, float, float]:
        values = self._run_wmic_values(["os", "get", "TotalVisibleMemorySize,FreePhysicalMemory", "/value"])
        try:
            total_kb = float(values.get("totalvisiblememorysize", ["0"])[0])
            free_kb = float(values.get("freephysicalmemory", ["0"])[0])
        except (ValueError, IndexError):
            return (0.0, 0.0, 0.0)
        if total_kb <= 0:
            return (0.0, 0.0, 0.0)
        used_kb = total_kb - free_kb
        usage_percent = 100.0 * used_kb / total_kb
        used_gb = used_kb / 1024.0 / 1024.0
        total_gb = total_kb / 1024.0 / 1024.0
        return (round(usage_percent, 1), round(used_gb, 1), round(total_gb, 1))

    def get_temperature(self) -> float:
        try:
            values = self._run_wmic_values([
                "/namespace:\\root\\wmi",
                "PATH",
                "MSAcpi_ThermalZoneTemperature",
                "get",
                "CurrentTemperature",
                "/value",
            ])
        except RuntimeError:
            return 0.0
        readings: List[float] = []
        for entry in values.get("currenttemperature", []):
            try:
                raw = float(entry)
            except ValueError:
                continue
            if raw <= 0:
                continue
            celsius = raw / 10.0 - 273.15
            readings.append(celsius)
        if not readings:
            return 0.0
        return round(sum(readings) / len(readings), 1)

    def get_fan_speed(self) -> int:
        return 0

    def get_gpu_usage(self) -> float:
        return 0.0

    def get_network_speed(self) -> Tuple[float, float]:
        rows = self._run_wmic_csv([
            "path",
            "Win32_PerfFormattedData_Tcpip_NetworkInterface",
            "get",
            "BytesReceivedPersec,BytesSentPersec,Name",
            "/format:csv",
        ])
        if not rows:
            return (0.0, 0.0)
        total_rx = 0.0
        total_tx = 0.0
        for row in rows:
            name = row.get("name", "").lower()
            if not name:
                continue
            if any(token in name for token in self._network_ignore_tokens):
                continue
            try:
                total_rx += float(row.get("bytesreceivedpersec", "0") or 0.0)
                total_tx += float(row.get("bytessentpersec", "0") or 0.0)
            except ValueError:
                continue
        down_mb = total_rx / 1048576.0
        up_mb = total_tx / 1048576.0
        return (round(max(0.0, down_mb), 2), round(max(0.0, up_mb), 2))

    def get_battery_info(self) -> Tuple[int, float]:
        values = self._run_wmic_values([
            "path",
            "Win32_Battery",
            "get",
            "EstimatedChargeRemaining",
            "/value",
        ])
        entries = values.get("estimatedchargeremaining", [])
        if not entries:
            return (-1, 0.0)
        try:
            capacity = int(entries[0])
        except ValueError:
            return (-1, 0.0)
        return (capacity, 0.0)


def main() -> int:
    print("=" * 60)
    print("PC Hardware Monitor for ESP32-C6-LCD-1.47 (Windows)")
    print("=" * 60)

    try:
        monitor = WindowsSystemMonitor()
    except RuntimeError as exc:
        print(f"Initialization error: {exc}")
        return 1

    serial_port: Optional[str] = None
    if len(sys.argv) > 1:
        serial_port = sys.argv[1]
        print(f"Using specified port: {serial_port}")

    comm = SerialCommunicator(port=serial_port, baudrate=115200)
    if not comm.connect():
        print("\nFailed to connect to ESP32. Exiting...")
        return 1

    print("\nMonitoring started. Press Ctrl+C to stop.\n")
    update_interval = 1.0

    try:
        while True:
            cpu_usage = monitor.get_cpu_usage()
            cpu_freq = monitor.get_cpu_frequency()
            ram_usage, ram_used_gb, ram_total_gb = monitor.get_ram_usage()
            temperature = monitor.get_temperature()
            fan_rpm = monitor.get_fan_speed()
            net_down, net_up = monitor.get_network_speed()
            battery_percent, power_watts = monitor.get_battery_info()
            gpu_usage = monitor.get_gpu_usage()

            console_parts: List[str] = []
            console_parts.append(f"CPU: {cpu_usage:5.1f}%")
            if cpu_freq > 0.0:
                console_parts.append(f"{cpu_freq:.1f}GHz")
            if gpu_usage > 0.0:
                console_parts.append(f"GPU: {gpu_usage:.1f}%")
            console_parts.append(f"| RAM: {ram_usage:5.1f}%")
            if ram_used_gb > 0.0 and ram_total_gb > 0.0:
                console_parts.append(f"({ram_used_gb:.1f}/{ram_total_gb:.1f}GB)")
            console_parts.append(f"| TEMP: {temperature:5.1f}°C")
            if fan_rpm > 0:
                console_parts.append(f"{fan_rpm}rpm")
            console_parts.append(f"| NET: ↓{net_down:.2f} ↑{net_up:.2f} MB/s")
            if battery_percent >= 0:
                console_parts.append(f"| BAT: {battery_percent}% {power_watts:.1f}W")

            print(" ".join(console_parts), end="\r")

            if not comm.send_data(
                cpu_usage,
                ram_usage,
                temperature,
                cpu_freq,
                gpu_usage,
                ram_used_gb,
                ram_total_gb,
                fan_rpm,
                net_down,
                net_up,
                battery_percent,
                power_watts,
            ):
                print("\nError sending data. Attempting to reconnect...")
                comm.disconnect()
                time.sleep(2)
                if not comm.connect():
                    print("Reconnection failed. Exiting...")
                    break

            time.sleep(update_interval)

    except KeyboardInterrupt:
        print("\n\nMonitoring stopped by user.")
    except Exception as exc:
        print(f"\n\nUnexpected error: {exc}")
    finally:
        comm.disconnect()

    return 0


if __name__ == "__main__":
    sys.exit(main())
