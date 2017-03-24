import sys
import numpy as np
import matplotlib.pyplot as plt

# https://tomroelandts.com/articles/how-to-create-a-simple-high-pass-filter
# fc: Cutoff frequency as a fraction of the sampling rate (in (0, 0.5)).
# b: Transition band, as a fraction of the sampling rate (in (0, 0.5)).
def createHighPass(fc, b):
  N = int(np.ceil((4 / b)))
  if not N % 2: N += 1  # Make sure that N is odd.
  n = np.arange(N)
 
  # Compute a low-pass filter.
  h = np.sinc(2 * fc * (n - (N - 1) / 2.))
  w = np.blackman(N)
  h = h * w
  h = h / np.sum(h)
 
  # Create a high-pass filter from the low-pass filter through spectral inversion.
  h = -h
  h[(N - 1) / 2] += 1
  return h

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
      time += float(delay)/1000
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
  highpass = createHighPass(0.02, 0.05)

  plt.plot(X,raw, label='Original')
  averaged = filter(data)
  print "averaged", averaged[1].shape, len(X)
  plt.plot(X,averaged[1], label='Averaged')
  edge = np.convolve(averaged[1], highpass, mode="same")
  edge = np.multiply(edge, 10)
  #print "edge", edge.shape, len(X)
  #plt.plot(X,edge)
  plt.show()

def main():
  filename = sys.argv[1]
  file = open(filename, "r")
  contents = file.read()
  data = parse(contents)

  graph(data)

if __name__ == "__main__":
  main()
