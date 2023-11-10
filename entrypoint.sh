#!/bin/sh

if [ -z "$LISTEN_ADDRESS" ]; then
  LISTEN_ADDRESS="0.0.0.0"
fi
if [ -z "$LISTEN_PORT" ]; then
  LISTEN_PORT=2000
fi
if [ -z "$TARGET_ADDRESS" ]; then
  if [ -z "$TARGET_HOST" ]; then
    echo "Either TARGET_ADDRESS or TARGET_HOST must be set"
    exit 1
  fi
  TARGET_ADDRESS=$( getent ahostsv4 "$TARGET_HOST" | head -n 1 | awk '{ print $1 }' )
  if [ -z "$TARGET_ADDRESS" ]; then
    echo "Unable to resolve '$TARGET_HOST'"
    exit 1
  fi
fi
if [ -z "$TARGET_PORT" ]; then
  echo "TARGET_PORT must be set"
  exit 1
fi
if [ -z "$LIMIT_BYTES_PER_SECOND" ]; then
  echo "LIMIT_BYTES_PER_SECOND must be set"
  exit 1
fi
if [ -z "$DELAY_SECONDS" ]; then
  echo "DELAY_SECONDS must be set"
  exit 1
fi

echo "Running: /bin/sloxy $LISTEN_ADDRESS $LISTEN_PORT $TARGET_ADDRESS $TARGET_PORT $LIMIT_BYTES_PER_SECOND $DELAY_SECONDS"
exec /bin/sloxy $LISTEN_ADDRESS $LISTEN_PORT $TARGET_ADDRESS $TARGET_PORT $LIMIT_BYTES_PER_SECOND $DELAY_SECONDS
