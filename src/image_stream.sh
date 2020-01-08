#!/usr/bin/env bash

PID_FILE=/home/pi/cam_pid

rm $PID_FILE
raspistill --nopreview --timeout 0 --rotation 270 --signal --output /home/pi/camera.jpg &
echo $! > $PID_FILE
