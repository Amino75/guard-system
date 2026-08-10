import ctypes
from ctypes import (
    c_float,
    c_uint32,
    c_uint64,
    c_char,
    POINTER,
    Structure,
    c_bool
)

import threading
import subprocess
import requests

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import (
    StreamingResponse,
    Response
)
from fastapi.openapi.docs import get_swagger_ui_html


# --------------------------------------------------
# Load C shared library
# --------------------------------------------------

LIB_PATH = "/opt/guard-system/build/libguard_api.so"

lib = ctypes.CDLL(LIB_PATH)


# --------------------------------------------------
# C structures
# --------------------------------------------------

class GuardTelemetry(Structure):
    _fields_ = [
        ("cpu_temperature", c_float),
        ("free_memory_mb", c_uint64),
        ("cpu_usage", c_float),
    ]


class HistoryEntry(Structure):
    _fields_ = [
        ("timestamp", c_char * 64),
        ("person_count", c_uint32),
        ("cpu_temperature", c_float),
    ]


# --------------------------------------------------
# C function prototypes
# --------------------------------------------------

lib.guard_api_get_telemetry.argtypes = [
    POINTER(GuardTelemetry)
]

lib.guard_api_get_telemetry.restype = ctypes.c_int


lib.guard_api_get_person_count.argtypes = [
    POINTER(c_uint32)
]

lib.guard_api_get_person_count.restype = ctypes.c_int


lib.guard_api_get_timestamp.argtypes = [
    POINTER(c_char),
    c_uint32
]

lib.guard_api_get_timestamp.restype = ctypes.c_int


lib.history_get.argtypes = [
    POINTER(HistoryEntry),
    POINTER(c_uint32)
]

lib.history_get.restype = ctypes.c_int


lib.guard_api_get_stream_path.argtypes = [
    POINTER(c_char),
    c_uint32
]

lib.guard_api_get_stream_path.restype = ctypes.c_int

lib.guard_api_set_guard_mode.argtypes = [
    c_bool
]

lib.guard_api_set_guard_mode.restype = ctypes.c_int

lib.guard_api_get_guard_mode.argtypes = [
    POINTER(c_bool)
]

lib.guard_api_get_guard_mode.restype = ctypes.c_int


# --------------------------------------------------
# FastAPI application
# --------------------------------------------------

app = FastAPI(
    title="Guard System API",
    version="1.0.0",
    description="REST API for the Guard System",
    docs_url=None
)


# --------------------------------------------------
# SWAGGER / OPENAPI DOCUMENTATION
# --------------------------------------------------

@app.get("/docs", include_in_schema=False)
def custom_swagger():
    return get_swagger_ui_html(
        openapi_url=app.openapi_url,
        title="Guard System API - Swagger UI"
    )


# --------------------------------------------------
# TELEMETRY
# --------------------------------------------------

@app.get(
    "/API/V1/TELEMETRY",
    summary="Get Telemetry"
)
def get_telemetry():

    data = GuardTelemetry()

    result = lib.guard_api_get_telemetry(
        ctypes.byref(data)
    )

    if result != 0:
        raise HTTPException(
            status_code=500,
            detail="Telemetry unavailable"
        )

    return {
        "cpu_temperature": round(
            data.cpu_temperature,
            2
        ),
        "free_memory_mb": data.free_memory_mb,
        "cpu_usage": round(
            data.cpu_usage,
            2
        )
    }


# --------------------------------------------------
# PERSON COUNT
# --------------------------------------------------

@app.get(
    "/API/V1/PERSONS",
    summary="Get Persons"
)
def get_persons():

    count = c_uint32(0)

    result = lib.guard_api_get_person_count(
        ctypes.byref(count)
    )

    if result != 0:
        raise HTTPException(
            status_code=500,
            detail="Person count unavailable"
        )

    timestamp_buffer = ctypes.create_string_buffer(64)

    result = lib.guard_api_get_timestamp(
        timestamp_buffer,
        64
    )

    if result != 0:
        raise HTTPException(
            status_code=500,
            detail="Timestamp unavailable"
        )

    return {
        "person_count": count.value,
        "timestamp": timestamp_buffer.value.decode()
    }


# --------------------------------------------------
# HISTORY
# --------------------------------------------------

@app.get(
    "/API/V1/HISTORY",
    summary="Get History"
)
def get_history():

    entries = (HistoryEntry * 5)()
    count = c_uint32(0)

    result = lib.history_get(
        entries,
        ctypes.byref(count)
    )

    if result != 0:
        raise HTTPException(
            status_code=500,
            detail="History unavailable"
        )

    result_list = []

    for i in range(count.value):

        result_list.append({
            "timestamp": entries[i].timestamp.decode(
                "utf-8",
                errors="replace"
            ),
            "person_count": entries[i].person_count,
            "cpu_temperature": round(
                entries[i].cpu_temperature,
                2
            )
        })

    return {
        "count": count.value,
        "history": result_list
    }


# --------------------------------------------------
# API/V1/STREAM
#
# GET
#
# Normal browser/client:
#     continuous MJPEG stream
#
# Swagger Execute:
#     one JPEG snapshot
# --------------------------------------------------

@app.get(
    "/API/V1/STREAM",
    summary="Live Camera Stream",
    description=(
        "Live MJPEG camera stream. "
        "Swagger Execute returns one JPEG snapshot."
    ),
    responses={
        200: {
            "description": (
                "JPEG snapshot for Swagger "
                "or live MJPEG stream"
            ),
            "content": {
                "image/jpeg": {},
                "multipart/x-mixed-replace": {}
            }
        }
    }
)
def get_stream(request: Request):

    upstream_url = "https://127.0.0.1:8443/stream"

    # --------------------------------------------------
    # Swagger UI sends:
    #
    # Accept: application/json
    #
    # Therefore return ONE JPEG frame instead of
    # keeping the Swagger request open forever.
    # --------------------------------------------------

    accept = request.headers.get(
        "accept",
        ""
    )

    if "application/json" in accept:

        response = requests.get(
            upstream_url,
            verify=False,
            stream=True,
            timeout=10
        )

        response.raise_for_status()

        try:

            buffer = b""

            for chunk in response.iter_content(
                chunk_size=4096
            ):

                if not chunk:
                    continue

                buffer += chunk

                start = buffer.find(
                    b"\xff\xd8"
                )

                end = buffer.find(
                    b"\xff\xd9"
                )

                if (
                    start != -1
                    and end != -1
                    and end > start
                ):

                    jpeg = buffer[
                        start:end + 2
                    ]

                    return Response(
                        content=jpeg,
                        media_type="image/jpeg"
                    )

        finally:

            response.close()

        raise HTTPException(
            status_code=500,
            detail="Unable to obtain camera snapshot"
        )

    # --------------------------------------------------
    # Normal API/client request
    #
    # Return continuous MJPEG
    # --------------------------------------------------

    def stream_generator():

        response = requests.get(
            upstream_url,
            verify=False,
            stream=True,
            timeout=10
        )

        response.raise_for_status()

        try:

            for chunk in response.iter_content(
                chunk_size=4096
            ):

                if chunk:
                    yield chunk

        finally:

            response.close()

    return StreamingResponse(
        stream_generator(),
        media_type=(
            "multipart/x-mixed-replace;"
            " boundary=frame"
        )
    )

# --------------------------------------------------
# COMMAND
# --------------------------------------------------

reboot_lock = threading.Lock()
reboot_scheduled = False

@app.post(
    "/API/V1/COMMAND",
    summary="Execute Command"
)
def execute_command(command: dict):

    global reboot_scheduled

    cmd = command.get("cmd")

    # --------------------------------------------------
    # GUARD MODE ON
    # --------------------------------------------------

    if cmd == "guard_on":

        result = lib.guard_api_set_guard_mode(
            True
        )

        if result != 0:
            raise HTTPException(
                status_code=500,
                detail="Unable to enable Guard Mode"
            )

        return {
            "command": "guard_on",
            "guard_mode": True,
            "status": "accepted"
        }

    # --------------------------------------------------
    # GUARD MODE OFF
    # --------------------------------------------------

    if cmd == "guard_off":

        result = lib.guard_api_set_guard_mode(
            False
        )

        if result != 0:
            raise HTTPException(
                status_code=500,
                detail="Unable to disable Guard Mode"
            )

        return {
            "command": "guard_off",
            "guard_mode": False,
            "status": "accepted"
        }

    # --------------------------------------------------
    # REBOOT
    # --------------------------------------------------

    if cmd == "reboot":

        # Prevent duplicate reboot scheduling

        with reboot_lock:

            if reboot_scheduled:

                raise HTTPException(
                    status_code=409,
                    detail="Reboot already scheduled"
                )

            reboot_scheduled = True

        # Give FastAPI enough time to send
        # the response before rebooting.

        subprocess.Popen(
            [
                "/bin/sh",
                "-c",
                "sleep 2; /bin/systemctl reboot"
            ],
            start_new_session=True
        )

        return {
            "command": "reboot",
            "status": "accepted"
        }

    # --------------------------------------------------
    # UNKNOWN COMMAND
    # --------------------------------------------------

    raise HTTPException(
        status_code=400,
        detail="Unsupported command"
    )

# --------------------------------------------------
# GUARD MODE STATUS
# --------------------------------------------------

@app.get(
    "/API/V1/GUARD",
    summary="Get Guard Mode"
)
def get_guard_mode():

    enabled = c_bool(False)

    result = lib.guard_api_get_guard_mode(
        ctypes.byref(enabled)
    )

    if result != 0:
        raise HTTPException(
            status_code=500,
            detail="Guard Mode unavailable"
        )

    return {
        "guard_mode": bool(enabled.value)
    }
