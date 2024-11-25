#!/bin/bash

HOST="127.0.0.1" 
PORT="8080"
PID=$(sudo netstat -tulnp | grep "$HOST:$PORT" | awk '{print $7}' | cut -d'/' -f1)
[[ "$PID" =~ ^[0-9]+$ ]] && sudo kill -9 $PID

command=$1
./main $command
