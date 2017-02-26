import sys
import numpy as np
import matplotlib.pyplot as plt
import json
import dateutil

def graph(dataseries):

  for name, series in dataseries.items():
    times = map(lambda d: d['seconds'], series)
    values = map(lambda d: d['data'], series)
    plt.plot(times, values, label=name)

  plt.show()

def edgeid(e):
  tgt = e['tgt']
  src = e['src']
  return "%s %s -> %s %s" % (src['node'], src['port'], tgt['port'], tgt['node'])

def parse_isotime(s):
  return 

def analyze_data(trace):
  start_time = None
  edge_data = {}
  for event in trace['events']:
    payload = event['payload']
    edge = edgeid(payload)
    time = dateutil.parser.parse(payload['time'])
    if not start_time:
      # XXX: picks the first one, assumes events ordered ascending
      start_time = time
    if not edge_data.get(edge):
      edge_data[edge] = []
    d = {
      'time': time,
      'seconds': (time - start_time).total_seconds(),
      'data': int(payload['data']),
    }
    #print edge, d['seconds']
    edge_data[edge].append(d)

  return edge_data

def main():
  filename = sys.argv[1]
  file = open(filename, "r")
  contents = file.read()
  trace = json.loads(contents)

  series = analyze_data(trace)

  graph(series)

if __name__ == "__main__":
  main()
