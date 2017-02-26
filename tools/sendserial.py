
import datetime
import serial
import sys

def main():
  port = "/dev/ttyACM0"
  baud = 115200
  filename = sys.argv[1]
  
  print "reading from", filename
  file = open(filename, "r")
  lines = file.read().split('\n')

  with serial.Serial() as ser:
    ser.baudrate = baud
    ser.port = port
    ser.open()
    print "ser opened"
    for line in lines:
      ser.write(line+'\n')
      print 'sent', line
      response = ser.readline()
      print 'response', response

if __name__ == "__main__":
  main()
