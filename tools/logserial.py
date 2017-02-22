
import datetime
import serial
import sys

def main():
  port = "/dev/cu.usbmodem411"
  baud = 115200
  filename = sys.argv[1]
  
  print "writing to", filename
  file = open(filename, "w")

  with serial.Serial() as ser:
    print "got ser"
    ser.baudrate = baud
    ser.port = port
    ser.open()
    print "ser opened"
    while True:
      line = ser.readline()
      file.write(line);

if __name__ == "__main__":
  main()
