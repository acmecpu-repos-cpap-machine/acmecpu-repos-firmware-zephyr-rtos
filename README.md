
## Zephyr RTOS Setup
To work with this project on **Zephyr RTOS**, follow these steps:

1. **Clone the repository:**
    ```bash
    git clone git@github.com:acmecpu-repos-cpap-machine/acmecpu-repos-firmware-zephyr-rtos.git
    cd acmecpu-repos-firmware-zephyr-rtos
    ```

2. **Install Zephyr and Dependencies:**
    Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html) to install Zephyr RTOS and all required dependencies.

3. **Building the Firmware:**
    - Navigate to the desired application folder (e.g., `Blower_Driver_STSPIN32F0A`) and build the project:
    ```bash
    west build -b <your_board> .
    ```

4. **Flashing the Firmware:**
    Flash the firmware to the target hardware using your preferred flashing method (J-Link, ST-Link, etc.).

## CubeIDE Integration
For components that are developed using **STM32CubeIDE**, the IDE project files are located within relevant subfolders. To work with these projects, follow these steps:

1. Open STM32CubeIDE.
2. Import the relevant project (e.g., `Blower_Driver_STSPIN32F0A_E20x`).
3. Build and debug the project as per your requirements.

## Folder Details

### Blower_Driver_STSPIN32F0A
Contains the driver code for controlling the blower motor using **STSPIN32F0A** chip. The code is designed to work with **Zephyr RTOS** to control motor speed, direction, and provide fault monitoring.

### Blower_Driver_STSPIN32F0A_E20x
This folder contains the STM32CubeIDE project for the **STSPIN32F0A** driver, used for platform-specific configuration and debugging.

### altium_device_sheets
Contains the **Altium Designer** files for creating device schematics and PCB designs for Acme CPU hardware. These are used to design various control boards and sensor integration.

### bindings
This folder includes configuration files for various **GPIO** and **sensor** bindings that are required for integrating hardware components with Zephyr.

### bq25611d
Firmware integration for the **BQ25611D** battery management system. The code ensures proper charging, power monitoring, and management of lithium-ion batteries in embedded devices.

### buzzer_generic
This folder contains the driver for controlling a generic buzzer that is used for signaling in embedded applications. It is implemented using Zephyr's device driver framework.

### c201_esp32, c201_stm32g4
These folders contain the firmware for **C201 platform**, one based on **ESP32** and the other on **STM32G4**. These platforms are used for different hardware versions of the control board.

### c202_consolidate
Contains consolidated firmware for the **C202** platform, integrating all the different subsystems into one unified application.

### c20x_algo
This folder includes communication algorithms for the **C20X** series, used in embedded systems for efficient communication and data processing.

### control-board-c201
Contains hardware design files for the **C201 control board**. These are **Altium** files that include the circuit diagrams, PCB layout, and other related documents.

### lib_audio, lib_events, lib_file_oper, etc.
These libraries provide various functionalities such as audio signal processing, event handling, file operations, and more, all implemented for **Zephyr RTOS**. These are generic libraries that can be used across different embedded systems.

## Contributing
If you'd like to contribute to this repository, please follow these guidelines:
1. Fork the repository.
2. Create a new branch for your feature.
3. Make changes and commit them.
4. Submit a pull request to the main branch.
5. For more details please contact feel free to +1 617 401 8929

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements
Special thanks to **Zephyr RTOS** and **CubeIDE** for their continuous support in enabling embedded system development.

