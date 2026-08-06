import cv2

from config import CameraConfig


class Camera:
    """
    USB camera wrapper.

    Responsibilities:
        - Open camera
        - Configure resolution
        - Configure FPS
        - Read frames
        - Release resources
    """

    def __init__(self, config: CameraConfig):

        self._config = config
        self._capture = None

    def open(self):

        self._capture = cv2.VideoCapture(
            self._config.device,
            cv2.CAP_V4L2
        )

        if not self._capture.isOpened():
            raise RuntimeError(
                f"Cannot open camera device {self._config.device}"
            )

        self._capture.set(
            cv2.CAP_PROP_FRAME_WIDTH,
            self._config.width
        )

        self._capture.set(
            cv2.CAP_PROP_FRAME_HEIGHT,
            self._config.height
        )

        self._capture.set(
            cv2.CAP_PROP_FPS,
            self._config.fps
        )

    def read(self):

        if self._capture is None:
            raise RuntimeError("Camera is not opened")

        success, frame = self._capture.read()

        if not success:
            raise RuntimeError("Failed to capture frame")

        return frame

    def release(self):

        if self._capture is not None:
            self._capture.release()

    def actual_width(self):
        return int(
            self._capture.get(cv2.CAP_PROP_FRAME_WIDTH)
        )

    def actual_height(self):
        return int(
            self._capture.get(cv2.CAP_PROP_FRAME_HEIGHT)
        )

    def actual_fps(self):
        return self._capture.get(cv2.CAP_PROP_FPS)