
default: build

%.json: %.fbp
	./node_modules/.bin/fbp $< > $@

env:
	mkdir bin || echo 'Exists'

bin/simulator: env ./graphs/pad.json
	g++ -Wall -g ./tools/simulator.cpp -o ./bin/simulator `pkg-config --cflags --libs alsa`

build: bin/simulator

clean:
	git clean -dfx --exclude node_modules
