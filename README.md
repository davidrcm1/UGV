UGV 

<img width="300" height="200" alt="image" src="https://github.com/user-attachments/assets/bc938610-f73b-4159-bcc3-e55fd650950c" />


This project implements a fully integrated tele-operation and sensing system for an Unmanned Ground Vehicle (UGV). 
The software is written in C++ and follows a modular, object-oriented architecture, where each major subsystem runs as an independent thread under a central Thread Management Module. 
Shared memory is used to enable safe and efficient inter-thread communication, allowing sensor data, control inputs, and vehicle commands to be exchanged in real time. 

<img width="788" height="585" alt="image" src="https://github.com/user-attachments/assets/45c55806-dcd7-4f24-b11a-5f8aceb7891d" />

The system interfaces with multiple onboard and simulated hardware components, including:

LIDAR (LMS151 Laser Rangefinder) for real-time obstacle detection and environment mapping

GNSS receiver providing simulated global position data

X-box controller for user input and tele-operation

Vehicle control server for steering and propulsion commands

MATLAB display engine for graphical visualisation via TCP streaming

The program features true multithreading, with separate threads managing the laser, GNSS, controller input, vehicle control, display, and crash-avoidance logic.
The Thread Management Module monitors thread heartbeats, performs error handling, restarts failed non-critical threads, and coordinates system shutdown.

A crash avoidance module processes LIDAR data and control inputs to automatically inhibit motion when obstacles are detected within 1 m, enhancing operational safety during tele-operation.

Overall, the project demonstrates real-time systems integration, networked communication, sensor processing, multithreaded programming, and autonomous safety behaviour within a structured software architecture.
