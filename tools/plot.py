import sys
import numpy as np
import matplotlib.pyplot as plt

def runningMeanFast(x, N):
  return np.convolve(x, np.ones((N,))/N)[(N-1):]

def filter(values):
  filtered = runningMeanFast(values[1], 7)
  return [ values[0], filtered ]

# return a numpy array with the data
def parse(input):
  times = []
  values = []

  start = 0.0
  time = start
  for line in input.split("\n"):
    line = line.strip()
    if not line:
      continue
    #print line
    data = line.rstrip(")").lstrip("(")
    try:
      delay, value = data.split(",")
      time += float(value)/1000
      value = int(value)
      times.append(time)
      values.append(value)
      #print time, value
    except ValueError, e:
      print "ERROR"
      print "wrong data '%s': %s" % (line, str(e))
      print "END ERROR"

  print "parsed", time, len(times), len(values)
  return [ times, values ]

def graph(data):
  X,raw = data 

  plt.plot(X,raw)
  filtered = filter(data)
  plt.plot(X,filtered[1])
  plt.show()

def main():
  filename = sys.argv[1]
  file = open(filename, "r")
  contents = file.read()
  data = parse(contents)

  graph(data)

if __name__ == "__main__":
  main()
