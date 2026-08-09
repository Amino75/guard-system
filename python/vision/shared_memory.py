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

        print("Shared memory closed")

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

    def get_person_count(self):

        return self._read_u32(
            PERSON_COUNT_OFFSET
        )

    def set_person_count(self, count):

        self._write_u32(
            PERSON_COUNT_OFFSET,
            count
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

            # Frame changed while we were copying it.
            if sequence_before != sequence_after:
                continue

            if sequence_after & 1:
                continue

            frame = np.frombuffer(
                raw,
                dtype=np.uint8
            ).copy()

            expected_size = width * height * 2

            if frame.size != expected_size:
                return None, sequence_after

            frame = frame.reshape(
                (height, width, 2)
            )

            # Convert C camera's YUYV frame to BGR
            bgr = cv2.cvtColor(
                frame,
                cv2.COLOR_YUV2BGR_YUY2
            )

            return bgr, sequence_after

        return None, None
