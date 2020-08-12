# robot-mobile

Code for my service robot. This repo covers everything that runs on the mobile platform, including:
- Communication with the base station
- Navigation (via line following)
- Motor control
- Localization (via apriltags)
- Obstacle detection

Dependencies:
- robot-comm (https://github.com/Angineer/robot-comm)
- apriltag (https://github.com/AprilRobotics/apriltag)
- raspistill dependencies
  - This uses a lot of code from raspistill, so it uses the same dependencies under the hood
  - See https://github.com/raspberrypi/userland/tree/master/host_applications/linux/apps/raspicam

To install, ensure you have the dependencies installed, then build with CMake:

`cd robot-mobile`\
`mkdir build && cd build`\
`cmake ..`\
`make`\
`sudo make install`

There is also a systemd service file included for running the program as a service.

See https://www.adtme.com/projects/Robot.html for more info about the overall project.
