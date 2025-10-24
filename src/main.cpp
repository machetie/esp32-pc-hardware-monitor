#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "ui_hardware_monitor.h"
#include <esp_pm.h>
#include <esp_sleep.h>

// Serial communication settings
#define SERIAL_BAUDRATE 115200
#define SERIAL_BUFFER_SIZE 128
#define DATA_TIMEOUT_MS 5000  // 5 seconds without data = disconnected

// Power saving settings
#define POWER_SAVE_BACKLIGHT 0    // Backlight level when disconnected (0 = off)
#define NORMAL_BACKLIGHT 5         // Normal backlight level
#define POWER_SAVE_DELAY_MS 10000  // 10 seconds after disconnect before power saving
#define ENABLE_CPU_FREQ_SCALING true  // Enable CPU frequency reduction

// System metrics structure
struct SystemMetrics {
  // Required fields
  float cpu_usage;
  float ram_usage;
  float temperature;

  // Optional fields
  float cpu_freq_ghz;
  float gpu_usage;          // GPU usage percentage
  float ram_used_gb;
  float ram_total_gb;
  int fan_rpm;
  float net_download_mbps;  // Changed to float for decimal precision
  float net_upload_mbps;    // Changed to float for decimal precision
  int battery_percent;      // -1 indicates unavailable
  float power_watts;

  // Connection status
  unsigned long last_update;
  bool connected;
  
  // Power saving state
  bool power_save_mode;
  unsigned long disconnect_time;
};

SystemMetrics metrics = {
  0.0, 0.0, 0.0,           // cpu, ram, temp
  0.0, 0.0,                // cpu_freq, gpu_usage
  0.0, 0.0,                // ram_used, ram_total
  0, 0.0, 0.0,             // fan_rpm, net_down, net_up (floats for decimal precision)
  -1, 0.0,                 // battery_percent, power_watts
  0, false,                // last_update, connected
  false, 0                 // power_save_mode, disconnect_time
};

// Serial buffer for incoming data
char serialBuffer[SERIAL_BUFFER_SIZE];
int bufferIndex = 0;

// Function prototypes
void initSerial();
void processSerialData();
bool parseMessage(const char* message);
void updateDisplay();
bool validateChecksum(const char* message);
void checkConnectionStatus();
void enterPowerSaveMode();
void exitPowerSaveMode();
void managePowerSaving();

void setup() {
  // Initialize Serial first for debugging
  initSerial();

  // Initialize display
  LCD_Init();
  Lvgl_Init();
  ui_hardware_monitor_init();
  Set_Backlight(5);

  Serial.println("Hardware Monitor Started");
  Serial.println("Waiting for data from PC...");
}

void loop() {
  // Process incoming serial data
  processSerialData();
  
  // Check connection status
  checkConnectionStatus();
  
  // Manage power saving mode
  managePowerSaving();
  
  // Update display
  updateDisplay();
  
  // Handle LVGL tasks
  Timer_Loop();
  
  // Longer delay in power save mode to reduce CPU usage
  delay(metrics.power_save_mode ? 100 : 5);
}

void initSerial() {
  Serial.begin(SERIAL_BAUDRATE);
  // Wait for serial to be ready (important for ESP32-C6 USB CDC)
  delay(100);
}

void processSerialData() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    // Check for newline (end of message)
    if (c == '\n' || c == '\r') {
      if (bufferIndex > 0) {
        serialBuffer[bufferIndex] = '\0';  // Null terminate
        
        // Parse the message
        if (parseMessage(serialBuffer)) {
          metrics.last_update = millis();
          
          // If reconnecting, exit power save mode
          if (!metrics.connected) {
            Serial.println("Connection restored");
          }
          
          metrics.connected = true;
        }
        
        // Reset buffer
        bufferIndex = 0;
      }
    } else if (bufferIndex < SERIAL_BUFFER_SIZE - 1) {
      serialBuffer[bufferIndex++] = c;
    } else {
      // Buffer overflow, reset
      bufferIndex = 0;
      Serial.println("Error: Buffer overflow");
    }
  }
}

bool parseMessage(const char* message) {
  // Expected format: CPU:45.2,RAM:67.8,TEMP:58.5[,FREQ:3.8][,RAMGB:11.9/31.3][,FAN:1500][,NET:125,15][,BAT:85][,POWER:10.0],CHK:XXX

  // First validate checksum
  if (!validateChecksum(message)) {
    Serial.println("Error: Invalid checksum");
    return false;
  }

  // Reset optional fields to default values
  metrics.cpu_freq_ghz = 0.0;
  metrics.gpu_usage = 0.0;
  metrics.ram_used_gb = 0.0;
  metrics.ram_total_gb = 0.0;
  metrics.fan_rpm = 0;
  metrics.net_download_mbps = 0.0;
  metrics.net_upload_mbps = 0.0;
  metrics.battery_percent = -1;
  metrics.power_watts = 0.0;

  // Parse required fields using strstr (doesn't modify string)
  const char* pos;

  // CPU (required)
  pos = strstr(message, "CPU:");
  if (pos) {
    metrics.cpu_usage = atof(pos + 4);
  } else {
    Serial.println("Error: CPU field missing");
    return false;
  }

  // RAM (required)
  pos = strstr(message, "RAM:");
  if (pos) {
    metrics.ram_usage = atof(pos + 4);
  } else {
    Serial.println("Error: RAM field missing");
    return false;
  }

  // TEMP (required)
  pos = strstr(message, "TEMP:");
  if (pos) {
    metrics.temperature = atof(pos + 5);
  } else {
    Serial.println("Error: TEMP field missing");
    return false;
  }

  // Parse optional fields

  // CPU Frequency (optional)
  pos = strstr(message, "FREQ:");
  if (pos) {
    metrics.cpu_freq_ghz = atof(pos + 5);
  }

  // GPU Usage (optional)
  pos = strstr(message, "GPU:");
  if (pos) {
    metrics.gpu_usage = atof(pos + 4);
  }

  // RAM GB (optional) - format: RAMGB:11.9/31.3
  pos = strstr(message, "RAMGB:");
  if (pos) {
    sscanf(pos + 6, "%f/%f", &metrics.ram_used_gb, &metrics.ram_total_gb);
  }

  // Fan RPM (optional)
  pos = strstr(message, "FAN:");
  if (pos) {
    metrics.fan_rpm = atoi(pos + 4);
  }

  // Network speed (optional) - format: NET:125.50,15.20 (float values with 2 decimals)
  pos = strstr(message, "NET:");
  if (pos) {
    sscanf(pos + 4, "%f,%f", &metrics.net_download_mbps, &metrics.net_upload_mbps);
  }

  // Battery percentage (optional)
  pos = strstr(message, "BAT:");
  if (pos) {
    metrics.battery_percent = atoi(pos + 4);
  }

  // Power watts (optional)
  pos = strstr(message, "POWER:");
  if (pos) {
    metrics.power_watts = atof(pos + 6);
  }

  // Validate ranges for required fields
  if (metrics.cpu_usage < 0.0 || metrics.cpu_usage > 100.0 ||
      metrics.ram_usage < 0.0 || metrics.ram_usage > 100.0 ||
      metrics.temperature < 0.0 || metrics.temperature > 150.0) {
    Serial.println("Error: Values out of range");
    return false;
  }

  return true;
}

bool validateChecksum(const char* message) {
  // Find the checksum field
  const char* chk_pos = strstr(message, ",CHK:");
  if (!chk_pos) {
    return false;
  }

  // Parse received checksum
  int received_checksum = atoi(chk_pos + 5);

  // Calculate expected checksum: sum of all numeric values mod 1000
  // This is a simplified validation - we'll just check if checksum field exists
  // Full validation would require parsing all values first
  // For now, accept any checksum value (Python calculates it correctly)
  // TODO: Implement full checksum validation after parsing all fields

  return true;  // Simplified validation for now
}

void checkConnectionStatus() {
  // Check if we've received data recently
  if (metrics.connected) {
    unsigned long timeSinceUpdate = millis() - metrics.last_update;
    if (timeSinceUpdate > DATA_TIMEOUT_MS) {
      metrics.connected = false;
      metrics.disconnect_time = millis();
      Serial.println("Connection lost - no data received");
    }
  }
}

void enterPowerSaveMode() {
  if (metrics.power_save_mode) {
    return;  // Already in power save mode
  }
  
  Serial.println("Entering power save mode...");
  metrics.power_save_mode = true;
  
  // Dim/turn off backlight to save power
  Set_Backlight(POWER_SAVE_BACKLIGHT);
  
  // Reduce CPU frequency if enabled
  if (ENABLE_CPU_FREQ_SCALING) {
    setCpuFrequencyMhz(80);  // Reduce from 160MHz to 80MHz
    Serial.println("CPU frequency reduced to 80MHz");
  }
  
  Serial.println("Power save mode active");
}

void exitPowerSaveMode() {
  if (!metrics.power_save_mode) {
    return;  // Not in power save mode
  }
  
  Serial.println("Exiting power save mode...");
  metrics.power_save_mode = false;
  
  // Restore normal backlight
  Set_Backlight(NORMAL_BACKLIGHT);
  
  // Restore CPU frequency if it was scaled
  if (ENABLE_CPU_FREQ_SCALING) {
    setCpuFrequencyMhz(160);  // Restore to default 160MHz
    Serial.println("CPU frequency restored to 160MHz");
  }
  
  Serial.println("Power save mode disabled");
}

void managePowerSaving() {
  if (!metrics.connected) {
    // Check if enough time has passed to enter power save mode
    unsigned long timeSinceDisconnect = millis() - metrics.disconnect_time;
    
    if (timeSinceDisconnect > POWER_SAVE_DELAY_MS && !metrics.power_save_mode) {
      enterPowerSaveMode();
    }
  } else {
    // Connected - ensure power save mode is disabled
    if (metrics.power_save_mode) {
      exitPowerSaveMode();
    }
  }
}

void updateDisplay() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();

  // Update display every 500ms
  if (now - lastUpdate < 500) {
    return;
  }
  lastUpdate = now;

  // Update UI using enhanced functions with additional parameters
  ui_update_cpu(metrics.cpu_usage, metrics.cpu_freq_ghz);
  ui_update_gpu(metrics.gpu_usage);
  ui_update_ram(metrics.ram_usage, metrics.ram_used_gb, metrics.ram_total_gb);
  ui_update_temp(metrics.temperature, metrics.fan_rpm);
  ui_update_network(metrics.net_download_mbps, metrics.net_upload_mbps);
  ui_update_battery(metrics.battery_percent, metrics.power_watts);
}

