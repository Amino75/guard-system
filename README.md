# Intelligent Guard System

An embedded intelligent guard system developed on **Orange Pi Zero Plus 2 H5**.
The system captures live video, processes frames for human detection, exposes a secure web interface, collects system telemetry, and supports automatic service management.

## Student

* **Name:** Amin Feizi
* **Student ID:** 404211106
* **Platform:** Orange Pi Zero Plus 2 H5
* **OS:** Ubuntu Linux
* **Main Language:** C
* **Image Processing:** Python
* **Web Server:** Mongoose
* **Vision Model:** YOLO / ONNX
* **Build System:** CMake

---

## System Architecture

```text
                    USB Camera
                        │
                        ▼
                ┌───────────────┐
                │  Camera / V4L2│
                │   camera.c    │
                └───────┬───────┘
                        │
                    JPEG Frame
                        │
                        ▼
                ┌───────────────┐
                │Image Processing│
                │  YOLO / ONNX  │
                └───────┬───────┘
                        │
                  person_count
                        │
                        ▼
                ┌───────────────┐
                │ Shared Memory  │
                │  telemetry_t  │
                └───────┬───────┘
                        │
          ┌─────────────┼──────────────┐
          ▼             ▼              ▼
     Web Server       MQTT          Logging
     HTTP/HTTPS       Client
          │             │
          ▼             ▼
      Web Browser    MQTT Broker
```

## Main Features

* USB camera capture through Linux **V4L2**
* `640x480 @ 30 FPS` camera configuration
* YUYV → RGB → JPEG conversion
* MJPEG live video streaming
* Frame ID tracking to avoid repeatedly sending old frames
* Human/person detection using YOLO and ONNX Runtime
* Shared memory for inter-module data exchange
* CPU temperature monitoring
* CPU usage monitoring
* Free memory monitoring
* REST-style telemetry/person APIs
* HTTP and HTTPS support
* Self-signed SSL certificate
* HTTP → HTTPS redirection
* Automatic startup using `systemd`
* Automatic crash recovery using `systemd`
* Configuration through JSON
* Modular C architecture

## Project Structure

```text
guard-system/
├── src/
│   ├── application/
│   ├── camera/
│   ├── web/
│   ├── shared_memory/
│   ├── utils/
│   └── ...
│
├── include/
│   ├── camera/
│   ├── web/
│   ├── shared_memory/
│   └── ...
│
├── configs/
│   └── settings.json
│
├── html/
│   ├── index.html
│   └── live/
│       └── latest.jpg
│
├── models/
├── cert/
├── third_party/
├── CMakeLists.txt
└── README.md
```

## Important Modules

### Camera

`camera.c` communicates directly with `/dev/video1` using V4L2.

It:

1. Opens the camera.
2. Configures resolution and FPS.
3. Allocates MMAP buffers.
4. Captures YUYV frames.
5. Converts frames to JPEG.
6. Stores the latest JPEG in memory.
7. Increments `frame_id` after each successful frame.

The public interface is provided by:

```c
camera_init();
camera_capture();
camera_get_jpeg();
camera_get_frame_id();
camera_stop();
```

### Web Server

`web_server.c` uses **Mongoose** to provide:

```text
/
 /stream
 /live/latest.jpg
 /telemetry
 /persons
```

The server supports both HTTP and HTTPS.

MJPEG streaming uses:

```text
multipart/x-mixed-replace
```

and continuously sends the newest available JPEG frame.

### Telemetry

System information is collected directly from Linux system interfaces, including:

```text
/sys/class/thermal/
 /proc/stat
 /proc/meminfo
```

The information is stored/shared through `telemetry_t`.

### Shared Memory

Shared memory provides communication between system components without requiring network sockets for local data exchange.

Typical shared data includes:

```text
person_count
cpu_temperature
cpu_usage
free_memory
timestamp
```

## Configuration

Main configuration file:

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

## Build

From the project directory:

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

The executable is generated as:

```text
build/guard-system
```

## Run Manually

```bash
./build/guard-system
```

## systemd Service

The main application is managed by:

```text
guard-webserver.service
```

Check status:

```bash
systemctl status guard-webserver.service
```

Start:

```bash
systemctl start guard-webserver.service
```

Stop:

```bash
systemctl stop guard-webserver.service
```

Restart:

```bash
systemctl restart guard-webserver.service
```

Enable automatic startup:

```bash
systemctl enable guard-webserver.service
```

View logs:

```bash
journalctl -u guard-webserver.service -f
```

## Testing

Boot time:

```bash
systemd-analyze
systemd-analyze blame
```

Crash recovery:

```bash
ps aux | grep guard-system
kill -9 <PID>
systemctl status guard-webserver.service
```

HTTPS:

```text
https://<ORANGE_PI_IP>/
```

The self-signed certificate uses:

```text
CN = 404211106
```

## Result

The final system provides an automatically starting embedded guard application with:

* Live camera streaming
* Human detection
* System telemetry
* Web-based monitoring
* HTTPS communication
* MQTT communication
* Shared-memory data exchange
* Automatic startup
* Automatic crash recovery

The system is designed as a modular embedded Linux application where camera capture, image processing, telemetry, networking, and service management are separated into independent components.
