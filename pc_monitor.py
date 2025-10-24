#!/usr/bin/env python3
"""
PC Hardware Monitor for ESP32-C6-LCD-1.47
Reads CPU usage, RAM usage, and k10temp temperature and sends to ESP32 via serial
"""

import time
import serial
import serial.tools.list_ports
import glob
import sys
import os
from typing import Optional, Tuple

class SystemMonitor:
    """Monitor system metrics: CPU, RAM, Temperature, Fan, Network, and Battery"""

    def __init__(self):
        self.prev_cpu_stats = None
        self.k10temp_path = None
        self.fan_sensor_path = None
        self.battery_path = None
        self.network_interface = None
        self.prev_net_stats = None
        self.prev_net_time = None
        self.gpu_device_path = None

        # Initialize sensors
        self._find_k10temp()
        self._find_fan_sensor()
        self._find_battery()
        self._find_network_interface()
        self._find_gpu_device()
    
    def _find_k10temp(self):
        """Find k10temp sensor in /sys/class/hwmon/"""
        hwmon_paths = glob.glob('/sys/class/hwmon/hwmon*/name')
        for path in hwmon_paths:
            try:
                with open(path, 'r') as f:
                    if f.read().strip() == 'k10temp':
                        # Found k10temp, get the directory
                        self.k10temp_path = os.path.dirname(path)
                        print(f"Found k10temp at: {self.k10temp_path}")
                        return
            except Exception as e:
                continue

        if not self.k10temp_path:
            print("Warning: k10temp sensor not found. Temperature will be 0.")

    def _find_fan_sensor(self):
        """Find fan sensor in /sys/class/hwmon/"""
        fan_paths = glob.glob('/sys/class/hwmon/hwmon*/fan*_input')
        if fan_paths:
            self.fan_sensor_path = fan_paths[0]
            print(f"Found fan sensor at: {self.fan_sensor_path}")
        else:
            print("Warning: No fan sensors found. Fan speed will be omitted.")

    def _find_battery(self):
        """Find battery in /sys/class/power_supply/"""
        battery_dirs = glob.glob('/sys/class/power_supply/BAT*/')
        if battery_dirs:
            self.battery_path = battery_dirs[0]
            print(f"Found battery at: {self.battery_path}")
        else:
            print("Info: No battery found (desktop system). Battery info will be omitted.")

    def _find_network_interface(self):
        """Find active network interface"""
        try:
            # Try to find default route interface from /proc/net/route
            with open('/proc/net/route', 'r') as f:
                lines = f.readlines()
                for line in lines[1:]:  # Skip header line
                    fields = line.split()
                    if len(fields) >= 2 and fields[1] == '00000000':  # Default route (0.0.0.0)
                        iface = fields[0]
                        # Verify interface exists and is not loopback
                        if iface != 'lo' and os.path.exists(f'/sys/class/net/{iface}'):
                            self.network_interface = iface
                            print(f"Found network interface from default route: {self.network_interface}")
                            return
        except Exception as e:
            print(f"Warning: Could not read /proc/net/route: {e}")

        # Fallback: Scan /sys/class/net/ for active interfaces
        try:
            net_dir = '/sys/class/net'
            if os.path.exists(net_dir):
                interfaces = os.listdir(net_dir)
                # Filter out loopback and virtual interfaces, prioritize physical interfaces
                physical_ifaces = []
                for iface in interfaces:
                    if iface == 'lo' or iface.startswith('tailscale') or iface.startswith('docker'):
                        continue
                    # Check if interface is UP
                    operstate_path = f'{net_dir}/{iface}/operstate'
                    try:
                        with open(operstate_path, 'r') as f:
                            state = f.read().strip()
                            if state == 'up' or state == 'unknown':
                                physical_ifaces.append(iface)
                    except Exception:
                        continue

                # Prioritize WiFi interfaces (wl*), then Ethernet (e*)
                for iface in physical_ifaces:
                    if iface.startswith('wl'):
                        self.network_interface = iface
                        print(f"Found WiFi interface: {self.network_interface}")
                        return

                for iface in physical_ifaces:
                    if iface.startswith('e'):
                        self.network_interface = iface
                        print(f"Found Ethernet interface: {self.network_interface}")
                        return

                # Use first available interface
                if physical_ifaces:
                    self.network_interface = physical_ifaces[0]
                    print(f"Found network interface: {self.network_interface}")
                    return
        except Exception as e:
            print(f"Warning: Could not scan network interfaces: {e}")

        print("Warning: No network interface found. Network speed will be omitted.")

    def _find_gpu_device(self):
        """Find GPU device in /sys/class/drm/"""
        try:
            # Look for DRM card devices
            drm_cards = glob.glob('/sys/class/drm/card*')
            for card_path in sorted(drm_cards):
                # Skip connector devices (e.g., card1-DP-1)
                if '-' in os.path.basename(card_path):
                    continue

                # Check for gpu_busy_percent file (AMD GPUs)
                gpu_busy_path = os.path.join(card_path, 'device', 'gpu_busy_percent')
                if os.path.exists(gpu_busy_path):
                    try:
                        # Test read to verify permissions
                        with open(gpu_busy_path, 'r') as f:
                            f.read()
                        self.gpu_device_path = gpu_busy_path
                        print(f"Found GPU device at: {self.gpu_device_path}")
                        return
                    except PermissionError:
                        print(f"Warning: Found GPU at {gpu_busy_path} but no read permission")
                        continue
        except Exception as e:
            print(f"Warning: Error scanning for GPU devices: {e}")

        if not self.gpu_device_path:
            print("Info: No GPU device found. GPU usage will be omitted.")

    def get_cpu_usage(self) -> float:
        """Calculate CPU usage percentage from /proc/stat"""
        try:
            with open('/proc/stat', 'r') as f:
                line = f.readline()  # First line is total CPU
                fields = line.split()

                # CPU fields: user, nice, system, idle, iowait, irq, softirq, steal
                user = int(fields[1])
                nice = int(fields[2])
                system = int(fields[3])
                idle = int(fields[4])
                iowait = int(fields[5])
                irq = int(fields[6])
                softirq = int(fields[7])

                # Calculate total and idle time
                total = user + nice + system + idle + iowait + irq + softirq
                idle_total = idle + iowait

                if self.prev_cpu_stats:
                    prev_total, prev_idle = self.prev_cpu_stats
                    total_diff = total - prev_total
                    idle_diff = idle_total - prev_idle

                    if total_diff > 0:
                        cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff
                    else:
                        cpu_usage = 0.0
                else:
                    cpu_usage = 0.0

                self.prev_cpu_stats = (total, idle_total)
                return round(cpu_usage, 1)

        except Exception as e:
            print(f"Error reading CPU usage: {e}")
            return 0.0

    def get_cpu_frequency(self) -> float:
        """Get CPU frequency in GHz from /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"""
        try:
            freq_path = '/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq'
            with open(freq_path, 'r') as f:
                freq_khz = int(f.read().strip())
                freq_ghz = freq_khz / 1000000.0  # Convert kHz to GHz
                return round(freq_ghz, 1)
        except (FileNotFoundError, PermissionError, ValueError) as e:
            # CPU frequency not available on this system
            return 0.0
        except Exception as e:
            print(f"Error reading CPU frequency: {e}")
            return 0.0

    def get_gpu_usage(self) -> float:
        """Get GPU usage percentage from DRM sysfs interface. Returns 0.0 if unavailable."""
        if not self.gpu_device_path:
            return 0.0

        try:
            with open(self.gpu_device_path, 'r') as f:
                gpu_percent = int(f.read().strip())
                return float(gpu_percent)
        except FileNotFoundError:
            # GPU device no longer available
            return 0.0
        except PermissionError:
            # No permission to read GPU stats
            return 0.0
        except ValueError as e:
            print(f"Error parsing GPU usage value: {e}")
            return 0.0
        except Exception as e:
            print(f"Error reading GPU usage: {e}")
            return 0.0
    
    def get_ram_usage(self) -> Tuple[float, float, float]:
        """Get RAM usage percentage and used/total in GB"""
        try:
            mem_total = 0
            mem_available = 0

            with open('/proc/meminfo', 'r') as f:
                for line in f:
                    if line.startswith('MemTotal:'):
                        mem_total = int(line.split()[1])  # in KB
                    elif line.startswith('MemAvailable:'):
                        mem_available = int(line.split()[1])  # in KB

                    if mem_total and mem_available:
                        break

            if mem_total > 0:
                mem_used = mem_total - mem_available
                usage_percent = 100.0 * mem_used / mem_total
                used_gb = mem_used / 1024.0 / 1024.0  # Convert KB to GB
                total_gb = mem_total / 1024.0 / 1024.0  # Convert KB to GB
                return (round(usage_percent, 1),
                       round(used_gb, 1),
                       round(total_gb, 1))
            else:
                return (0.0, 0.0, 0.0)

        except Exception as e:
            print(f"Error reading RAM usage: {e}")
            return (0.0, 0.0, 0.0)
    
    def get_temperature(self) -> float:
        """Get Tctl temperature from k10temp sensor"""
        if not self.k10temp_path:
            return 0.0

        try:
            temp_file = os.path.join(self.k10temp_path, 'temp1_input')
            with open(temp_file, 'r') as f:
                # Temperature is in millidegrees Celsius
                temp_millidegrees = int(f.read().strip())
                temp_celsius = temp_millidegrees / 1000.0
                return round(temp_celsius, 1)
        except Exception as e:
            print(f"Error reading temperature: {e}")
            return 0.0

    def get_fan_speed(self) -> int:
        """Get fan speed in RPM"""
        if not self.fan_sensor_path:
            return 0

        try:
            with open(self.fan_sensor_path, 'r') as f:
                rpm = int(f.read().strip())
                return rpm
        except Exception as e:
            print(f"Error reading fan speed: {e}")
            return 0

    def get_network_speed(self) -> Tuple[float, float]:
        """Get network download and upload speed in MB/s (with 1 decimal precision)"""
        if not self.network_interface:
            return (0.0, 0.0)

        try:
            rx_path = f'/sys/class/net/{self.network_interface}/statistics/rx_bytes'
            tx_path = f'/sys/class/net/{self.network_interface}/statistics/tx_bytes'

            # Verify paths exist
            if not os.path.exists(rx_path) or not os.path.exists(tx_path):
                print(f"Error: Network statistics files not found for {self.network_interface}")
                return (0.0, 0.0)

            with open(rx_path, 'r') as f:
                rx_bytes = int(f.read().strip())
            with open(tx_path, 'r') as f:
                tx_bytes = int(f.read().strip())

            current_time = time.time()

            # First reading - store baseline
            if self.prev_net_stats is None:
                self.prev_net_stats = (rx_bytes, tx_bytes)
                self.prev_net_time = current_time
                # Debug: Print initial values
                # print(f"Network baseline: RX={rx_bytes} bytes, TX={tx_bytes} bytes")
                return (0.0, 0.0)

            # Calculate speed
            time_delta = current_time - self.prev_net_time
            if time_delta > 0:
                rx_bytes_delta = rx_bytes - self.prev_net_stats[0]
                tx_bytes_delta = tx_bytes - self.prev_net_stats[1]

                # Calculate speed in MB/s
                rx_speed = rx_bytes_delta / time_delta / 1048576  # Convert to MB/s
                tx_speed = tx_bytes_delta / time_delta / 1048576  # Convert to MB/s

                # Clamp negative values to 0 (can happen if interface resets)
                rx_speed = max(0.0, rx_speed)
                tx_speed = max(0.0, tx_speed)

                # Update previous values
                self.prev_net_stats = (rx_bytes, tx_bytes)
                self.prev_net_time = current_time

                # Debug: Print speed calculation
                # print(f"Network speed: RX={rx_speed:.2f} MB/s ({rx_bytes_delta} bytes in {time_delta:.2f}s), "
                #       f"TX={tx_speed:.2f} MB/s ({tx_bytes_delta} bytes in {time_delta:.2f}s)")

                # Return with 2 decimal precision for better small-value visibility
                return (round(rx_speed, 2), round(tx_speed, 2))
            else:
                return (0.0, 0.0)

        except FileNotFoundError as e:
            print(f"Error: Network interface {self.network_interface} statistics not found: {e}")
            return (0.0, 0.0)
        except ValueError as e:
            print(f"Error: Invalid network statistics value: {e}")
            return (0.0, 0.0)
        except Exception as e:
            print(f"Error reading network speed from {self.network_interface}: {e}")
            return (0.0, 0.0)

    def get_battery_info(self) -> Tuple[int, float]:
        """Get battery percentage and power draw in watts. Returns (-1, 0.0) if no battery."""
        if not self.battery_path:
            return (-1, 0.0)

        try:
            capacity_path = os.path.join(self.battery_path, 'capacity')
            power_now_path = os.path.join(self.battery_path, 'power_now')
            current_now_path = os.path.join(self.battery_path, 'current_now')
            voltage_now_path = os.path.join(self.battery_path, 'voltage_now')

            # Read battery percentage
            with open(capacity_path, 'r') as f:
                capacity = int(f.read().strip())

            # Try to read power_now first
            power_watts = 0.0
            if os.path.exists(power_now_path):
                with open(power_now_path, 'r') as f:
                    power_microwatts = int(f.read().strip())
                    power_watts = power_microwatts / 1000000.0
            elif os.path.exists(current_now_path) and os.path.exists(voltage_now_path):
                # Calculate power from current and voltage
                with open(current_now_path, 'r') as f:
                    current_microamps = int(f.read().strip())
                with open(voltage_now_path, 'r') as f:
                    voltage_microvolts = int(f.read().strip())
                power_watts = (current_microamps * voltage_microvolts) / 1000000000000.0

            return (capacity, round(power_watts, 1))

        except Exception as e:
            print(f"Error reading battery info: {e}")
            return (-1, 0.0)


class SerialCommunicator:
    """Handle serial communication with ESP32"""
    
    def __init__(self, port: Optional[str] = None, baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        self.auto_detect = (port is None)
    
    def find_esp32_port(self) -> Optional[str]:
        """Try to find ESP32 serial port automatically"""
        ports = serial.tools.list_ports.comports()

        # First, prioritize /dev/ttyACM* and /dev/ttyUSB* ports (real hardware)
        priority_ports = []
        other_ports = []

        for port in ports:
            if '/dev/ttyACM' in port.device or '/dev/ttyUSB' in port.device:
                priority_ports.append(port)
            else:
                other_ports.append(port)

        # Look for common ESP32 identifiers in priority ports first
        for port in priority_ports:
            description = port.description.lower()
            manufacturer = (port.manufacturer or "").lower()

            if any(keyword in description or keyword in manufacturer
                   for keyword in ['esp32', 'cp210', 'ch340', 'usb serial', 'uart', 'jtag']):
                print(f"Found ESP32 at: {port.device} ({port.description})")
                return port.device

        # If no match found but priority ports exist, use first priority port
        if priority_ports:
            port = priority_ports[0]
            print(f"Using first USB port: {port.device} ({port.description})")
            return port.device

        # If no priority ports, list all available ports
        if ports:
            print("\nAvailable serial ports:")
            for port in ports:
                print(f"  {port.device}: {port.description}")
            print("\nNo /dev/ttyACM* or /dev/ttyUSB* ports found.")
            print("Please specify port manually: python3 pc_monitor.py /dev/ttyACM0")

        return None
    
    def connect(self) -> bool:
        """Connect to serial port"""
        if self.auto_detect:
            self.port = self.find_esp32_port()
        
        if not self.port:
            print("Error: No serial port found!")
            return False
        
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1,
                write_timeout=1
            )
            time.sleep(2)  # Wait for connection to stabilize
            print(f"Connected to {self.port} at {self.baudrate} baud")
            return True
        except Exception as e:
            print(f"Error connecting to {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from serial port"""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("Disconnected from serial port")
    
    def send_data(self, cpu: float, ram: float, temp: float,
                  cpu_freq: float = 0.0, gpu_usage: float = 0.0,
                  ram_used_gb: float = 0.0, ram_total_gb: float = 0.0,
                  fan_rpm: int = 0, net_down: float = 0.0, net_up: float = 0.0,
                  battery_percent: int = -1, power_watts: float = 0.0) -> bool:
        """Send data to ESP32 using enhanced protocol with optional fields"""
        if not self.serial or not self.serial.is_open:
            return False

        try:
            # Build message with required fields
            fields = []
            fields.append(f"CPU:{cpu:.1f}")
            fields.append(f"RAM:{ram:.1f}")
            fields.append(f"TEMP:{temp:.1f}")

            # Add optional fields only if available
            if cpu_freq > 0.0:
                fields.append(f"FREQ:{cpu_freq:.1f}")

            fields.append(f"GPU:{gpu_usage:.1f}")

            if ram_used_gb > 0.0 and ram_total_gb > 0.0:
                fields.append(f"RAMGB:{ram_used_gb:.1f}/{ram_total_gb:.1f}")

            if fan_rpm > 0:
                fields.append(f"FAN:{fan_rpm}")

            # Network is always sent (can be 0.0,0.0) with 2 decimal precision
            if net_down >= 0.0 and net_up >= 0.0:
                fields.append(f"NET:{net_down:.2f},{net_up:.2f}")

            # Battery only if available (not desktop)
            if battery_percent >= 0:
                fields.append(f"BAT:{battery_percent}")
                if power_watts >= 0.0:
                    fields.append(f"POWER:{power_watts:.1f}")

            # Join all fields
            message = ",".join(fields)

            # Calculate checksum: sum of all numeric values mod 1000
            checksum_sum = cpu + ram + temp
            if cpu_freq > 0.0:
                checksum_sum += cpu_freq
            checksum_sum += gpu_usage
            if ram_used_gb > 0.0 and ram_total_gb > 0.0:
                checksum_sum += ram_used_gb + ram_total_gb
            if fan_rpm > 0:
                checksum_sum += fan_rpm
            if net_down >= 0.0 and net_up >= 0.0:
                checksum_sum += net_down + net_up
            if battery_percent >= 0:
                checksum_sum += battery_percent
                if power_watts >= 0.0:
                    checksum_sum += power_watts

            checksum = int(checksum_sum) % 1000

            # Add checksum and newline
            full_message = f"{message},CHK:{checksum}\n"

            # Send to ESP32
            self.serial.write(full_message.encode())
            self.serial.flush()
            return True

        except Exception as e:
            print(f"Error sending data: {e}")
            return False


def main():
    """Main monitoring loop"""
    print("=" * 60)
    print("PC Hardware Monitor for ESP32-C6-LCD-1.47")
    print("=" * 60)
    
    # Initialize monitor
    monitor = SystemMonitor()
    
    # Initialize serial communication
    serial_port = None
    if len(sys.argv) > 1:
        serial_port = sys.argv[1]
        print(f"Using specified port: {serial_port}")
    
    comm = SerialCommunicator(port=serial_port, baudrate=115200)
    
    # Connect to ESP32
    if not comm.connect():
        print("\nFailed to connect to ESP32. Exiting...")
        return 1
    
    print("\nMonitoring started. Press Ctrl+C to stop.\n")
    
    update_interval = 1.0  # Update every 1 second
    
    try:
        while True:
            # Get system metrics
            cpu_usage = monitor.get_cpu_usage()
            cpu_freq = monitor.get_cpu_frequency()
            gpu_usage = monitor.get_gpu_usage()
            ram_usage, ram_used_gb, ram_total_gb = monitor.get_ram_usage()
            temperature = monitor.get_temperature()
            fan_rpm = monitor.get_fan_speed()
            net_down, net_up = monitor.get_network_speed()
            battery_percent, power_watts = monitor.get_battery_info()

            # Display on console (enhanced)
            console_parts = []
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

            print(" ".join(console_parts), end='\r')

            # Send to ESP32
            if not comm.send_data(cpu_usage, ram_usage, temperature,
                                 cpu_freq, gpu_usage,
                                 ram_used_gb, ram_total_gb,
                                 fan_rpm, net_down, net_up,
                                 battery_percent, power_watts):
                print("\nError sending data. Attempting to reconnect...")
                comm.disconnect()
                time.sleep(2)
                if not comm.connect():
                    print("Reconnection failed. Exiting...")
                    break

            time.sleep(update_interval)
            
    except KeyboardInterrupt:
        print("\n\nMonitoring stopped by user.")
    except Exception as e:
        print(f"\n\nUnexpected error: {e}")
    finally:
        comm.disconnect()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

