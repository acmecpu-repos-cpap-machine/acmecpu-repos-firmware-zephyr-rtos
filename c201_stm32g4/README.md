# c201_stm32g4

Repository to contain source files for the STM32G473 based firmware for Control Board Model C201


# Build the project

Assuming the path to the project is $HOME/git/acme_cpu/c201_stm32g4/ and $HOME/zephyrproject/zephyr is the path to the Zephyr source code:

cd $HOME/zephyrproject/zephyr

source zephyr-env.sh (one time action to configure environment variables)

west build -p always -d $HOME/git/acme_cpu/c201_stm32g4/build $HOME/git/acme_cpu/c201_stm32g4/source -G"Eclipse CDT4 - Ninja"

or

west build -p auto -d $HOME/git/acme_cpu/c201_stm32g4/build $HOME/git/acme_cpu/c201_stm32g4/source -G"Eclipse CDT4 - Ninja"

if you do not necessarily need to build the project in a "pristine" build directory.


# Open the project in Eclipse IDE

Menu -> File -> Open Projects from File System...-> Select $HOME/git/acme_cpu/c201_stm32g4/ folder -> Open -> Select c201_stm32g4/build -> Finish


# Launch a GUI-based configuration tool (Kconfig)

cd $HOME/zephyrproject/zephyr

source zephyr-env.sh (one time action to configure environment variables)

west build -d $HOME/git/acme_cpu/c201_stm32g4/build -t guiconfig

# Flash a MCU with west

cd $HOME/zephyrproject/zephyr

source zephyr-env.sh (one time action to configure environment variables)

west flash -d $HOME/git/acme_cpu/c201_stm32g4/build --board-dir $HOME/git/acme_cpu/c201_stm32g4/source/boards\
	 -r openocd --config $HOME/git/acme_cpu/c201_stm32g4/source/boards/arm/stm32g473_acme_cpu_c201/support/openocd_st-link.cfg
