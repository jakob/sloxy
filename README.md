# sloxy
A slow TCP proxy

Do you want your server to be slower? Sloxy can help!

We wanted a way to simulate a slow network for testing Postico. There are a lot of potential solutions out there.

All of them seemed too complicated, so I wrote my own.

## How it works

Sloxy listens for incoming TCP connections. It then connects to the destination address, and forwards all data with a delay.

Sloxy can enforce a maximum bandwidth and add a constant delay.

Sloxy allows multiple connections to the server.

Sloxy enforces limits both when sending and when receive.
This means the minimum network round-trip time is twice the configured delay.

Sloxy requires 6 arguments:

`./sloxy listen_addr listen_port destination_addr destination_port speed_limit delay`

`listen_addr` is the IP address you listen on.
Typically 127.0.0.1, or 0.0.0.0 if you want to listen on all interfaces.

`listen_port` the port to listen on.

`destination_addr` is the IP address of the server.

`destination_port` is the port the server runs on.

`speed_limit` is the maximum number of bytes per second.

`delay` is the transmission delay in seconds.

## Example

To make PostgreSQL infuriatingly slow, type:

`./sloxy 127.0.0.1 5433 127.0.0.1 5432 1000 0.1`

Then connect to port 5433 instead of 5432:

psql postgres://127.0.0.1:5433

## How to build

Just type `make sloxy` in the Terminal. Yes, I know that there is no makefile. Make can still do it.

## Known limitations

Argument validation could be better. Right now sloxy only makes sure there are 6 arguments.

Sloxy only supports numeric IPv4 addresses. Would be nice if we could type hostnames instead.

