import sys
import numpy as np
import matplotlib.pyplot as plt
import json
import dateutil
import numbers

def graph(dataseries):

  for name, series in dataseries.items():
    times = map(lambda d: d['seconds'], series)

    print 'plotting', name
    if name.endswith('threshold trigger'):
      # HACK: hardcoded.
      for v in series:
        plt.axhline(y=v['data'], color='red')
      continue

    first = series[0]['data'] # we expect homogenous data...
    if isinstance(first, numbers.Number):
      values = map(lambda d: int(d['data']), series)
      plt.plot(times, values, label=name)
    elif first.get('type') and first.get('type').startswith("note"):
      # HACK: hardcoded to reconize notes
      # booleans should default to this visualization?
      # should be a way to use a predicate function to get this for any value
      # XXX: also does not work with multiple different overlapping messages a the same time
      # need a way to establish their "identity"? A "group by" construct might be fine, ref Eve lang
      values = map(lambda d: d['data']['type'], series)
      span_start = None
      for time, data in zip(times, values):
        value = data == "noteon"
        if value:
          plt.axvline(x=time, color='black')
          if span_start:
            pass
          else:
            span_start = { 'time': time, 'data': data }
        else:
          plt.axvline(x=time, color='black')
          if span_start:
            plt.axvspan(span_start['time'], time, alpha=0.5)
            span_start = None
          else:
            pass
            # TODO: handle case where we (seemingly) start on, then transition off?  

    else:
      print "Unknown data type", values[0]

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
      'data': payload['data'],
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
