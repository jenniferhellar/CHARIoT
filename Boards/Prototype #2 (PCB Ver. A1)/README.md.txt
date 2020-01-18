*** To set up Eagle for PCB Design, Prototype #2 ***

Download EAGLE.
Navigate to the EAGLE folder on your PC (on mine it was This PC > Documents > EAGLE).
Copy "MF_Standard_4_Layer" and "MF_Standard_2_Layer" into EAGLE > design rules.
Copy libraries into EAGLE > libraries.
Copy CHARIoT_Power, CHARIoT_Sensor, and CHARIoT_Comms2 into EAGLE > projects.
Open Eagle and navigate to desired project.

*** Project Descriptions ***
CHARIoT_Power
	- Top board design.
	- Function: Energy harvesting, storage, and monitoring.
	- Battery and solar cell included.
	- 6-pin PWR header (GND, 3.3V, 5.0V, NC, I2C_SCL, I2C_SDA) to both comms and sensor PCBs.
	- SCL and SDA lines connect processor on sensor PCB to fuel gauge on power PCB.

CHARIoT_Comms2
	- Middle board design.
	- Function: Bluetooth communications.
	- Trace antenna.
	- 6-pin PWR header (GND, 3.3V, NC, EN, NC, NC) to both power and sensor PCBs.
	- EN line from sensor PCB controls power to this board.
	- 10-pin COMMS I/O header (SPI, I2C, GPIO, etc.) to sensor board below.
	- 10-pin JTAG header for programming.

CHARIoT_Sensor
	- Bottom board design.
	- Function: Sensing, processing, and control.
	- 6-pin PWR header (GND, 3.3V, 5.0V, COMMS_EN, I2C_SCL, I2C_SDA) to both power and comms PCBs.
	- 10-pin COMMS I/O header (SPI, I2C, GPIO, etc.) to comms PCB.
	- 4-pin JTAG header for programming.
	- 10-pin SENSOR header (SPI, I2C, GPIO, Analog) to connect external dust sensor or other devices.
	- 3-pin SENSOR_PWR header (GND, 3.3V, 5.0V) to power external dust sensor or other devices. 
	- On-board HDC1080 Temperature/humidity sensor. I2C communication on same bus as fuel gauge.
	- 