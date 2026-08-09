# Intelligent Guard System

An embedded intelligent guard system developed on the **Orange Pi Zero Plus 2 H5**. The system captures live video from a USB camera, performs human detection, exchanges data between C and Python components through shared memory, provides web-based monitoring, exposes REST-style APIs, collects system telemetry, maintains persistent detection history, and uses HTTPS for secure communication.

The project is primarily implemented in **C**, with Python used for the computer-vision pipeline and the API documentation/routing layer.

---

## Overview

The system is designed as a modular embedded Linux application consisting of:

* V4L2-based camera acquisition
* JPEG/MJPEG video streaming
* Human detection using MobileNet-SSD
* Shared-memory communication
* System telemetry collection
* Persistent detection history
* C-based API library
* FastAPI/Swagger API layer
* Mongoose HTTP/HTTPS server
* SSL/TLS support
* systemd service management
* Automatic service recovery

The architecture separates hardware acquisition, image processing, system monitoring, API functionality, and service management into independent components.

---

## System Architecture

```text
                         USB Camera
                         /dev/video1
                              │
                              ▼
                    ┌──────────────────┐
                    │   C Camera       │
                    │   V4L2 + MMAP    │
                    │   camera.c       │
                    └────────┬─────────┘
                             │
                       Latest YUYV Frame
                             │
                             ▼
                    ┌──────────────────┐
                    │   Shared Memory  │
                    │   telemetry_t    │
                    └────────┬─────────┘
                             │
                             ▼
                    ┌──────────────────┐
                    │ Python Vision    │
                    │ NumPy / OpenCV   │
                    │ MobileNet-SSD    │
                    └────────┬─────────┘
                             │
                       person_count
                             │
                             ▼
                    ┌──────────────────┐
                    │   Shared Memory  │
                    └────────┬─────────┘
                             │
              ┌──────────────┼───────────────┐
              │              │               │
              ▼              ▼               ▼
        C Web Server     C API Library   FastAPI / Swagger
         Mongoose          libguard_api       │
              │              │               │
              └──────────────┼───────────────┘
                             │
                             ▼
                       HTTPS / REST API
                             │
                             ▼
                        Web Browser
```

---

# Main Features

## Camera and Video Processing

* USB camera support through Linux **V4L2**
* Camera device: `/dev/video1`
* Resolution: `640 × 480`
* Camera frame rate: `30 FPS`
* MMAP-based frame acquisition
* YUYV raw frame sharing
* JPEG frame generation
* Latest-frame buffering
* MJPEG live streaming

## Human Detection

The vision pipeline is implemented in Python using:

* NumPy
* OpenCV
* MobileNet-SSD
* C/Python shared memory

The C camera process provides the latest YUYV frame through shared memory. The Python vision process reads the frame, performs detection, and writes the resulting `person_count` back to shared memory.

This allows the camera acquisition and vision processing components to operate as separate processes without transferring frames through network sockets.

## System Telemetry

The C implementation reads system information directly from Linux interfaces:

```text
/sys/class/thermal/thermal_zone0/temp
/proc/meminfo
/proc/stat
```

The API provides:

* CPU temperature
* Free memory
* CPU usage

No external Linux utilities such as `top`, `free`, `sensors`, or `cat` are required by the telemetry implementation.

## Shared Memory

The project uses POSIX shared memory for communication between the C and Python components.

The shared structure contains camera, detection, system, and frame information, including:

```text
frame_number
person_count
fps
cpu_temperature
guard_mode
person_detected
timestamp
jpeg_path
frame_sequence
frame_width
frame_height
frame_size
frame_yuyv[]
```

The raw camera frame is stored as:

```text
640 × 480 × 2 = 614400 bytes
```

in YUYV 4:2:2 format.

A frame-sequence mechanism is used to identify stable frames while the C camera component updates the shared buffer.

---

# Part 2: REST API and Live Monitoring

Part 2 extends the Part 1 web infrastructure with a C-based API library and a FastAPI/Swagger interface.

The C library is compiled as:

```text
build/libguard_api.so
```

and loaded by the Python API layer using `ctypes`.

This keeps the system-level functionality in C while allowing FastAPI to provide API routing and interactive Swagger documentation.

## Implemented API Endpoints

```text
GET  /API/V1/TELEMETRY
GET  /API/V1/PERSONS
GET  /API/V1/HISTORY
GET  /API/V1/STREAM
POST /API/V1/COMMAND
```

### Telemetry

```text
GET /API/V1/TELEMETRY
```

Returns:

```json
{
    "cpu_temperature": 48.52,
    "free_memory_mb": 214,
    "cpu_usage": 93.75
}
```

The values are obtained from the C API.

### Person Count

```text
GET /API/V1/PERSONS
```

Returns the current detected person count together with an ISO-style timestamp.

Example:

```json
{
    "person_count": 1,
    "timestamp": "2026-08-09T15:53:39"
}
```

### History

```text
GET /API/V1/HISTORY
```

The C history module stores the most recent five detection records in persistent storage.

Each record contains:

```text
timestamp
person_count
cpu_temperature
```

The storage file is:

```text
/var/lib/guard_history.bin
```

The history therefore survives application restarts and system reboots.

### Live Stream

```text
GET /API/V1/STREAM
```

The API forwards the live MJPEG stream produced by the C web server.

For Swagger testing, the endpoint can return a single JPEG frame so that the request can complete normally in the Swagger interface.

The underlying C server provides the continuous MJPEG stream using:

```text
multipart/x-mixed-replace
```

### Command

```text
POST /API/V1/COMMAND
```

The currently implemented command interface accepts the controlled `reboot` command.

The API validates the command before execution and prevents duplicate reboot scheduling.

Example request:

```json
{
    "cmd": "reboot"
}
```

---

# C API Layer

The C API is defined in:

```text
include/api/guard_api.h
```

and implemented in:

```text
src/api/guard_api.c
```

The API currently provides functions for:

```c
guard_api_get_telemetry()
guard_api_get_person_count()
guard_api_get_timestamp()
guard_api_get_stream_path()
```

The Python FastAPI layer loads:

```text
build/libguard_api.so
```

using:

```python
ctypes.CDLL()
```

and calls the C functions directly.

This architecture prevents telemetry and person-count logic from being reimplemented in Python.

---

# FastAPI and Swagger

The API documentation layer is implemented in:

```text
python/api/main.py
```

FastAPI provides:

* REST endpoint routing
* JSON responses
* HTTP status handling
* OpenAPI generation
* Swagger UI

Swagger is available through:

```text
/docs
```

The API layer communicates with the C implementation through the compiled shared library.

---

# HTTPS

The system supports HTTPS through the Mongoose web server.

Certificates are stored in:

```text
cert/
├── server.crt
└── server.key
```

The C web server listens on:

```text
HTTP   : 8080
HTTPS  : 8443
```

HTTP requests are redirected to HTTPS using HTTP status:

```text
301 Moved Permanently
```

The HTTPS server uses the self-signed certificate generated for the embedded system.

---

# Web Server

The C web server is implemented using **Mongoose**.

Main source:

```text
src/web/web_server.c
```

The server provides:

```text
/
 /stream
 /live/latest.jpg
 /telemetry
 /persons
```

The MJPEG stream continuously sends the newest camera frame and avoids building a backlog of old frames.

A frame ID is used to ensure that the same camera frame is not repeatedly transmitted.

---

# Persistent History

Detection history is implemented in:

```text
src/history/history.c
include/history/history.h
```

The history manager maintains a maximum of five entries.

When the history is full, the oldest record is removed and the newest record is appended.

```text
Entry 1
Entry 2
Entry 3
Entry 4
Entry 5
        ↓
new event
        ↓
Entry 2
Entry 3
Entry 4
Entry 5
New Entry
```

The data is stored in:

```text
/var/lib/guard_history.bin
```

and restored when the application starts.

---

# Configuration

The main configuration is stored in:

```text
configs/settings.json
```

Example:

```json
{
    "vision": {
        "jpeg_path": "html/live/latest.jpg",
        "jpeg_quality": 90
    },

    "camera": {
        "device": 1,
        "width": 640,
        "height": 480,
        "fps": 30
    }
}
```

Detection can also be controlled through:

```text
configs/detection.enabled
```

---

# Project Structure

```text
guard-system/
├── build/
│   ├── guard-system
│   └── libguard_api.so
│
├── cert/
│   ├── server.crt
│   └── server.key
│
├── configs/
│   ├── settings.json
│   └── detection.enabled
│
├── docs/
│   └── thermal_tests/
│
├── html/
│   ├── index.html
│   └── live/
│       └── latest.jpg
│
├── include/
│   ├── api/
│   ├── application/
│   ├── camera/
│   ├── common/
│   ├── history/
│   ├── shared_memory/
│   ├── utils/
│   └── web/
│
├── models/
│   └── mobilenet-ssd/
│
├── python/
│   ├── api/
│   │   └── main.py
│   └── vision/
│       ├── camera.py
│       ├── detector.py
│       ├── jpeg_writer.py
│       ├── main.py
│       └── shared_memory.py
│
├── src/
│   ├── api/
│   ├── application/
│   ├── camera/
│   ├── common/
│   ├── history/
│   ├── shared_memory/
│   ├── utils/
│   └── web/
│
├── third_party/
│   ├── cJSON/
│   └── mongoose/
│
├── CMakeLists.txt
├── Makefile
└── README.md
```

---

# Build

From the project directory:

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

The main executable is generated as:

```text
build/guard-system
```

and the C API shared library as:

```text
build/libguard_api.so
```

---

# Running the System

The main application can be started manually with:

```bash
./build/guard-system
```

The production deployment uses systemd for automatic startup and recovery.

Check service status:

```bash
systemctl status guard-webserver.service
```

Restart:

```bash
systemctl restart guard-webserver.service
```

View logs:

```bash
journalctl -u guard-webserver.service -f
```

Boot-time behavior can be examined using:

```bash
systemd-analyze
systemd-analyze blame
```

---

# Testing

Part 2 includes dedicated evaluation tests covering system behavior and performance.

## Test 2-1 — CPU Temperature

Temperature is measured every 30 seconds under:

1. Idle operation
2. Streaming only
3. Streaming + detection

The collected data is used to evaluate the thermal behavior of the system under increasing workloads.

**Result:**
*Test results and temperature graph will be inserted here.*

```text
[PLACEHOLDER — TEST 2-1 GRAPH]
```

---

## Test 2-2 — Memory Consumption

RSS and virtual memory usage are monitored during continuous streaming.

The purpose is to identify any increasing memory consumption that could indicate a memory leak.

**Result:**
*Memory usage graph and analysis will be inserted here.*

```text
[PLACEHOLDER — TEST 2-2 GRAPH]
```

---

## Test 2-3 — Concurrent API Load

Multiple concurrent requests are sent to:

```text
/API/V1/TELEMETRY
```

The test measures the system's behavior under burst API traffic.

Telemetry collected during the test includes:

* CPU temperature
* CPU usage
* Free memory

Request latency is also recorded for later analysis.

**Result:**
*Latency/telemetry graphs and analysis will be inserted here.*

```text
[PLACEHOLDER — TEST 2-3 GRAPH]
```

---

## Test 2-4 — Network Failure and Recovery

The network connection is intentionally interrupted while the system is operating.

The test evaluates:

* Camera/stream behavior
* API availability
* Application stability
* Recovery after reconnection
* systemd service behavior

**Result:**
*Network failure/recovery logs and screenshots will be inserted here.*

```text
[PLACEHOLDER — TEST 2-4 LOG SCREENSHOT]
```

---

## Test 2-5 — Security Validation

The security test evaluates:

* Command injection rejection
* Command validation
* Process privilege
* Absence of embedded credentials
* HTTP → HTTPS enforcement

Malicious command inputs are tested against the command endpoint to verify that unsupported commands are rejected.

**Result:**
*Security test screenshots, command responses, and analysis will be inserted here.*

```text
[PLACEHOLDER — TEST 2-5 SECURITY SCREENSHOT]
```

---

# Security

The system incorporates several security mechanisms:

* HTTPS communication using TLS
* HTTP → HTTPS redirection
* Controlled command interface
* Input validation for API commands
* No direct shell execution for arbitrary user commands through the API
* Generic API error responses
* Separation of C system functionality from the API presentation layer
* systemd-based service management
* SSH security configuration inherited from Part 1

Security validation is documented in Test 2-5.

---

# Development Approach

The project follows a modular architecture:

```text
Hardware
   │
   ▼
Camera Acquisition
   │
   ▼
Shared Memory
   │
   ▼
Vision Processing
   │
   ▼
Detection Results
   │
   ├───────────────┐
   ▼               ▼
C API          Web Server
   │               │
   └───────┬───────┘
           ▼
      FastAPI / Swagger
           │
           ▼
        HTTPS API
```

C is responsible for the core embedded functionality, including camera acquisition, shared-memory management, telemetry, history management, and the native web/API interface. Python is used for the computational vision pipeline and for the FastAPI documentation/routing layer.

---

# Result

The completed system provides an embedded Linux guard platform with:

* Live USB camera acquisition
* Human detection
* Shared-memory communication
* MJPEG video streaming
* System telemetry
* Persistent detection history
* REST-style API endpoints
* Interactive Swagger documentation
* HTTPS communication
* HTTP → HTTPS redirection
* Modular C architecture
* Python-based vision processing
* systemd-based automatic startup and recovery
* Performance and security validation

The architecture is designed to keep hardware and system-level functionality close to the C layer while providing a practical Python-based interface for computer vision and API documentation.
