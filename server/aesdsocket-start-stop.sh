#!/bin/sh

start() {
    echo "Starting aesdsocket..."
    start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
    if [ $? -eq 0 ]; then
        echo "aesdsocket started successfully."
    else
        echo "Failed to start aesdsocket."
    fi
}

stop() {
    echo "Stopping aesdsocket..."
    start-stop-daemon -K -n aesdsocket
    if [ $? -eq 0 ]; then
        echo "aesdsocket stopped."
    else
        echo "Failed to stop aesdsocket."
    fi
    rm -f $PIDFILE
}

case "$1" in
    start) start ;;
    stop) stop ;;
    *) echo "Usage: $0 {start|stop}" ; exit 1 ;;
esac

exit 0
