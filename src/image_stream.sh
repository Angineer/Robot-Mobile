#!/usr/bin/env bash

PID_FILE=/home/pi/cam_pid

rm $PID_FILE
raspistill --nopreview --timeout 0 --rotation 270 --encoding bmp --width 800 --height 600 --signal --output /home/pi/camera.bmp &
echo $! > $PID_FILE
