.. _acme_cpu_c201:

Acme CPU Sample
################

Description
***********

This sample application periodically ....................... driver. The result.................... .

Requirements
************

This sample needs a ....... sensor connected to the target board's I2C connector.


Wiring
******

The sensor operates at 3.3V and uses I2C to communicate with the board.

External Wires:

Building and Running
********************

In order to build the sample, connect the board to the computer with a USB cable and enter the
following commands:

.. zephyr-app-commands::
   :zephyr-app: 
   :board: stm32g473_acme_cpu
   :goals: build flash
   :compact:

Sample Output
*************
The output can be seen via ......................................
Connect the board with a ................. to the computer and open /dev/ttyXXX with the below serial settings:

* Baudrate: 115200
* Parity: None
* Data: 8
* Stop bits: 1

The output should look like this:

.. code-block:: console

    Device XXXXXX - XXXXXXXXXX is ready
    xxxx is xx.xxxxxxx xx
    xxxx is xx.xxxxxxx xx
    ...
