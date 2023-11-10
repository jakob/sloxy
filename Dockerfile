from alpine:latest as build

RUN apk update && apk add gcc musl-dev

ADD sloxy.c /sloxy.c

RUN gcc sloxy.c -lm -o sloxy

CMD sloxy

from alpine:latest

COPY --from=build /sloxy /bin/sloxy

ENTRYPOINT /bin/sloxy
