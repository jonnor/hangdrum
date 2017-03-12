
import yaml

import subprocess
import sys, os
import copy
import time
import json

def simulate(sensorpath, tracepath, options):
    options_string = json.dumps(options)
    args = ['./bin/simulator', sensorpath, options_string, tracepath]
    out = subprocess.check_output(args)
    print out
    time.sleep(1)
    tracefile = open(tracepath, 'r')
    trace = tracefile.read()
    return json.loads(trace)

def data_to(node, port):
    def pred(e):
        is_data = e['command'] == 'data'
        tgt = e['payload']['tgt']
        node_matches = tgt['node'] == node
        port_matches = tgt['port'] == port
        return is_data and node_matches and port_matches
    return pred

def check_triggering(testcase):
    name_sanitized = testcase['name'].replace(' ', '-').lower()
    tracepath = os.path.join('./test/temp', name_sanitized+'.trace')
    testoptions = copy.copy(testcase['inputs'])
    sensorpath = testoptions['sensor']  
    del testoptions['sensor']

    trace = simulate(sensorpath, tracepath, testoptions)
    note_events = filter(data_to('send', 'in'), trace['events'])

    expected_notes = testcase['expect']['detected']['equal']
    actual_notes = len(note_events)/2 # XXX: assuming matching note on/off
    assert actual_notes == expected_notes, \
            "number of notes %d != %d" % (actual_notes, expected_notes)

# Data-driven testing with Nose
def test_triggering():
    testspath = './test/triggering.yaml'
    testsfile = open(testspath, 'r')
    tests = yaml.load(testsfile.read())
    for testcase in tests['cases']:
        yield check_triggering, testcase

