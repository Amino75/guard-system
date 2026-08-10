import mmap
import struct
import time

import cv2
import numpy as np


SHM_PATH = "/dev/shm/guard_system"

FRAME_SEQUENCE_OFFSET = 340
FRAME_WIDTH_OFFSET = 344
FRAME_HEIGHT_OFFSET = 348
FRAME_SIZE_OFFSET = 352
FRAME_YUYV_OFFSET = 356

PERSON_COUNT_OFFSET = 4

MAX_FRAME_SIZE = 640 * 480 * 2

MAX_DETECTIONS = 10


DETECTION_SEQUENCE_OFFSET = (
    FRAME_YUYV_OFFSET + MAX_FRAME_SIZE
)

DETECTION_COUNT_OFFSET = (
    DETECTION_SEQUENCE_OFFSET + 4
)

DETECTIONS_OFFSET = (
    DETECTION_COUNT_OFFSET + 4
)

DETECTION_SIZE = 20


class SharedMemoryReader:

    def __init__(self, path=SHM_PATH):

        self.path = path
        self.fd = None
        self.mm = None

    def open(self):

        self.fd = open(
            self.path,
            "r+b",
            buffering=0
        )

        self.mm = mmap.mmap(
            self.fd.fileno(),
            0,
            access=mmap.ACCESS_WRITE
        )

        print(
            "Shared memory opened:",
            self.path
        )

    def close(self):

        if self.mm is not None:

            self.mm.close()

            self.mm = None

        if self.fd is not None:

            self.fd.close()

            self.fd = None

        print(
            "Shared memory closed"
        )

    def _read_u32(self, offset):

        return struct.unpack_from(
            "<I",
            self.mm,
            offset
        )[0]

    def _write_u32(self, offset, value):

        struct.pack_into(
            "<I",
            self.mm,
            offset,
            int(value)
        )

    def _write_float(self, offset, value):

        struct.pack_into(
            "<f",
            self.mm,
            offset,
            float(value)
        )

    def get_person_count(self):

        return self._read_u32(
            PERSON_COUNT_OFFSET
        )

    def set_person_count(self, count):

        self._write_u32(
            PERSON_COUNT_OFFSET,
            count
        )

    def set_detections(self, persons):

        if self.mm is None:

            raise RuntimeError(
                "Shared memory is not opened"
            )

       
        persons = persons[
            :MAX_DETECTIONS
        ]

        
        sequence = self._read_u32(
            DETECTION_SEQUENCE_OFFSET
        )

        if sequence & 1:

            sequence += 1

        self._write_u32(
            DETECTION_SEQUENCE_OFFSET,
            sequence + 1
        )

        
        self._write_u32(
            DETECTION_COUNT_OFFSET,
            len(persons)
        )

   
        for i, person in enumerate(persons):

            offset = (
                DETECTIONS_OFFSET
                + i * DETECTION_SIZE
            )

            self._write_u32(
                offset,
                person["x"]
            )

            self._write_u32(
                offset + 4,
                person["y"]
            )

            self._write_u32(
                offset + 8,
                person["width"]
            )

            self._write_u32(
                offset + 12,
                person["height"]
            )

            self._write_float(
                offset + 16,
                person["confidence"]
            )

        
        for i in range(
            len(persons),
            MAX_DETECTIONS
        ):

            offset = (
                DETECTIONS_OFFSET
                + i * DETECTION_SIZE
            )

            self._write_u32(
                offset,
                0
            )

            self._write_u32(
                offset + 4,
                0
            )

            self._write_u32(
                offset + 8,
                0
            )

            self._write_u32(
                offset + 12,
                0
            )

            self._write_float(
                offset + 16,
                0.0
            )

       
        self._write_u32(
            DETECTION_SEQUENCE_OFFSET,
            sequence + 2
        )

    def read_frame(self):

        if self.mm is None:

            raise RuntimeError(
                "Shared memory is not opened"
            )

        for _ in range(20):

            sequence_before = self._read_u32(
                FRAME_SEQUENCE_OFFSET
            )

            # Odd means C is currently writing.
            if sequence_before & 1:

                time.sleep(0.001)

                continue

            width = self._read_u32(
                FRAME_WIDTH_OFFSET
            )

            height = self._read_u32(
                FRAME_HEIGHT_OFFSET
            )

            size = self._read_u32(
                FRAME_SIZE_OFFSET
            )

            if (
                width <= 0
                or height <= 0
                or size <= 0
                or size > MAX_FRAME_SIZE
            ):

                return None, sequence_before

            raw = self.mm[
                FRAME_YUYV_OFFSET:
                FRAME_YUYV_OFFSET + size
            ]

            sequence_after = self._read_u32(
                FRAME_SEQUENCE_OFFSET
            )

            # Frame changed while copying.
            if sequence_before != sequence_after:

                continue

            if sequence_after & 1:

                continue

            frame = np.frombuffer(
                raw,
                dtype=np.uint8
            ).copy()

            expected_size = (
                width * height * 2
            )

            if frame.size != expected_size:

                return None, sequence_after

            frame = frame.reshape(
                (height, width, 2)
            )

            # Convert C camera YUYV → BGR.
            bgr = cv2.cvtColor(
                frame,
                cv2.COLOR_YUV2BGR_YUY2
            )

            return bgr, sequence_after

        return None, None
