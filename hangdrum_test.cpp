
#include "./hangdrum.hpp"

#include <cstdio>

int main() {
    hangdrum::Config config;
    hangdrum::State state;

    for ( auto &p : config.pads ) {
        printf("pad: %d, %d\n", p.pin, p.note);
    }

    for ( auto &s : state.pads ) {
        printf("state: %d\n", s.value);
    }
}
