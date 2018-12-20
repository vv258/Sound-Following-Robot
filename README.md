# Sound-Following-Robot
A robot based on PIC32 microcontroller that follows a person carrying a beacon which emits two frequencies periodically, using sound localisation

The idea is to mount the microcontroller on a robotic platform and use the controller to identify the direction and
distance of a predetermined audio signal and move the S-Robot (Sound localization Robot) towards it. The sound transmitter can be carried by a person, hence making the robot follow him. The robot can be used in various applications. The distance and direction of the audio transmitter will be determined by using an array of microphones and measuring the time
difference of when the sound is received between the microphones. 

The measurement of the distance and direction is on the basis of triangulation using time difference of arrival of an audio signal at three mics located on the corners of an equilateral triangle.

