# Modbus/TCP to SPI Bridge - STM32F4

A complete industrial protocol bridge implementation that converts Modbus/TCP commands to SPI communications, enabling remote control of SPI slave devices over Ethernet.

## 🎯 Overview

This project transforms an STM32F4 microcontroller into a dual-role bridge device:
- **Modbus/TCP Slave**: Accepts commands from PC-based Modbus master applications
- **SPI Master**: Forwards commands to SPI slave devices using a custom ASCII protocol

```
[PC Modbus Client] ←→ [STM32F4 Bridge] ←→ [STM32F4 SPI Slave]
   (Modbus/TCP)         (This Project)         (Separate MCU)
```

## ✨ Features

- **Dual LED Control**: Manage both local (bridge) and remote (SPI slave) LEDs
- **Human-Readable Protocol**: ASCII-based SPI commands for easy debugging
- **Industrial-Grade**: Built on FreeModbus and lwIP stacks
- **Flexible Mapping**: 8 Modbus coils (4 local + 4 remote)
- **Special Commands**: Bulk operations via holding registers
- **Real-Time Status**: Query remote device status via Modbus

## 📋 Hardware Requirements

- **STM32F4 Discovery Board** (or compatible STM32F4xx)
- **Ethernet PHY Module** (LAN8720, DP83848, etc.)
- **SPI Connection** to slave device
- **4 LEDs** (typically on-board LEDs: Green, Orange, Red, Blue)

## 🔌 Pin Configuration

### Ethernet (via PHY)
Configure using STM32CubeMX based on your PHY chip.

### SPI1 (Master Mode)
| Pin  | Function | Description |
|------|----------|-------------|
| PA4  | GPIO_Output | Chip Select (CS) - Software controlled |
| PA5  | SPI1_SCK | Serial Clock |
| PA6  | SPI1_MISO | Master In Slave Out |
| PA7  | SPI1_MOSI | Master Out Slave In |

### LEDs (Local Control)
| Pin  | LED Color | Modbus Coil |
|------|-----------|-------------|
| PD12 | Green     | 0 (00001)   |
| PD13 | Orange    | 1 (00002)   |
| PD14 | Red       | 2 (00003)   |
| PD15 | Blue      | 3 (00004)   |

## 📦 Dependencies

- **STM32CubeF4 HAL**: Hardware abstraction layer
- **FreeModbus**: Modbus/TCP stack
- **lwIP**: Lightweight TCP/IP stack
- **Custom Integration**: `freemodbus-lwip-hal` (your existing integration)

## 🛠️ Configuration

### STM32CubeMX Settings

#### SPI1 Configuration
```
Mode: Full-Duplex Master
Hardware NSS: Disable (use GPIO)
Prescaler: 64 (for ~1.3 MHz @ APB 84MHz)
Data Size: 8 Bits
CPOL: Low
CPHA: 1 Edge
First Bit: MSB First
```

#### Network Configuration
Update in `main.h` or your lwIP configuration:
```c
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   1
#define IP_ADDR3   50
```

## 📂 Project Structure

```
Middlewares/
└── modbus/
    ├── modbus_callbacks.c  # Modbus register/coil handlers (MODIFY THIS)
    ├── modbus_callbacks.h
    └── port/
        ├── port.c     
        └── port.h

Core/
├── Inc/
│   ├── main.h
│   └── spi_protocol.h              # SPI communication API (ADD THIS)
└── Src/
    ├── main.c
    └── spi_protocol.c              # SPI master implementation (ADD THIS)

```

## 🔧 Integration

### 1. Add Files to Project
Copy these files to your project:
- `spi_protocol.h` → `Core/Inc/`
- `spi_protocol.c` → `Core/Src/`
- `modbus_callbacks.c` → `Core/Src/` 
- `modbus_callbacks.h` → `Core/Inc/`
- `port.c` → `Middlewares/modbus/port/` 
- `port.h` → `Middlewares/modbus/port/`

### 2. Update main.c

```c
#include "modbus_callbacks.h"
#include "spi_protocol.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_LWIP_Init();
    
    // Initialize Modbus (includes SPI initialization)
    Modbus_InitCallbacks();
    
    // Start Modbus TCP on port 502
    eMBErrorCode eStatus = eMBTCPInit(502);
    if (eStatus == MB_ENOERR) {
        eStatus = eMBEnable();
    }
    
    while (1) {
        eMBPoll();           // Modbus processing
        MX_LWIP_Process();   // Network stack processing
    }
}
```

### 3. Configure SPI CS Pin
In STM32CubeMX or `main.h`, ensure CS pin is defined:
```c
#define SPI_CS_Pin GPIO_PIN_4
#define SPI_CS_GPIO_Port GPIOA
```

## 📡 Modbus Register Map

### Coils (Read/Write)
| Address | Function | Location |
|---------|----------|----------|
| 0-3 (00001-00004) | Bridge LEDs (G, O, R, B) | Local |
| 4-7 (00005-00008) | Slave LEDs (G, O, R, B) | Via SPI |

### Holding Registers (Read/Write)
| Address | Function | Description |
|---------|----------|-------------|
| 0 (40001) | Counter | Auto-increments on read |
| 1 (40002) | Command | Special bulk commands |

**Special Commands (Register 1):**
- `0xAA00` (43520): Turn OFF all slave LEDs
- `0xAA01` (43521): Turn ON all slave LEDs

## 🔌 SPI Protocol Specification

### Command Format
All commands are ASCII strings terminated with `\n`:

```
LED:G1\n    - Turn ON Green LED
LED:G0\n    - Turn OFF Green LED
LED:O1\n    - Turn ON Orange LED
LED:O0\n    - Turn OFF Orange LED
LED:R1\n    - Turn ON Red LED
LED:R0\n    - Turn OFF Red LED
LED:B1\n    - Turn ON Blue LED
LED:B0\n    - Turn OFF Blue LED
LED:A0\n    - Turn OFF ALL LEDs
LED:A1\n    - Turn ON ALL LEDs
GET:LED\n   - Get LED status
```

### Response Format
```
OK\n         - Command successful
ERR\n        - Command failed
STA:GORB\n   - Status (each digit 0=OFF, 1=ON)
```

**Status Example**: `STA:1010\n` means Green=ON, Orange=OFF, Red=ON, Blue=OFF

## 🚀 Building and Flashing

### Using STM32CubeIDE
1. Import project into STM32CubeIDE
2. Build: `Project → Build Project`
3. Flash: `Run → Debug` or `Run → Run`

### Using Command Line
```bash
# Build
make

# Flash using ST-Link
st-flash write build/project.bin 0x8000000
```

## 🧪 Testing

### Quick Test with Modbus Client
1. Connect to bridge IP: `192.168.1.50:502`
2. Set Unit ID: `1`
3. **Test local LED**: Write coil 0 = `1` (Green LED ON)
4. **Test SPI LED**: Write coil 4 = `1` (Slave Green LED ON via SPI)
5. **Read status**: Read coils 0-7 (get all LED states)
6. **Bulk command**: Write register 1 = `0xAA00` (all slave LEDs OFF)

## 🐛 Troubleshooting

### Modbus Connection Issues
- Verify IP configuration matches your network
- Check Ethernet link LED is active
- Ping the bridge: `ping 192.168.1.50`
- Verify port 502 is not blocked by firewall

### SPI Communication Issues
- Check wiring between bridge and slave
- Verify GND connection between MCUs
- Reduce SPI clock speed if experiencing errors
- Add pull-up resistor (10kΩ) on NSS line
- Use logic analyzer to verify SPI signals

## 📊 Performance Characteristics

- **Modbus Response Time**: < 10 ms (local operations)
- **SPI Transaction Time**: ~5-10 ms per command
- **End-to-End Latency**: 50-100 ms (PC → Bridge → Slave)
- **Maximum Throughput**: ~10-20 commands/second

## 📝 API Reference

### Public Functions (spi_protocol.h)

```c
void SPI_Protocol_Init(void);
// Initialize SPI protocol and CS pin

SPI_Result SPI_SetLED(SPI_LED_Color color, uint8_t state);
// Control individual slave LED

SPI_Result SPI_GetLEDStatus(uint8_t *green, uint8_t *orange, 
                            uint8_t *red, uint8_t *blue);
// Query all slave LED states

void SPI_TurnOffAllLEDs(void);
void SPI_TurnOnAllLEDs(void);
// Bulk LED control
```

## 👤 Author

**Samet Arslan**
- Project Created: September 2025

## 🔗 Related Projects

- [SPI Slave Device Firmware](https://github.com/samarslan/STM32F407-MODBUS-OVER-ETH-SPI-SLAVE)
- [C# Modbus Client](https://github.com/samarslan/ModbusMaster)

---

**Status**: ✅ Production Ready  
**Version**: 1.0.0  
**Last Updated**: September 30, 2025
