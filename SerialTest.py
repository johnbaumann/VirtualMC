import time
import serial

global GID		# get identifier
global GFV		# get firmware version
global MCR	 	# mcr read command, should be followed by a verify memcard
global MCW		# mcr write command, should be followed by a verify memcard
global MCID		# read mc identifier

GID = b'\xA0'
GFV = b'\xA1'
MCR = b'\xA2'
MCW = 'b\xA3'
MCID = b'\xA4'

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
	port='COM14',
	baudrate=38400,
    rtscts=true,
	parity=serial.PARITY_NONE,
	stopbits=serial.STOPBITS_ONE,
	bytesize=serial.EIGHTBITS
)

ser.open()
ser.isOpen()

ser.write(input + '\r\n')
out = ''
# let's wait one second before reading output (let's give device time to answer)
time.sleep(1)
while ser.inWaiting() > 0:
	out += ser.read(1)
	
if out != '':
	print ">>" + out