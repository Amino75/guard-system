import cv2
import os


class JpegWriter:

    def __init__(self, path, quality=90):
        self.path = path
        self.quality = quality

        directory = os.path.dirname(self.path)

        if directory:
            os.makedirs(directory, exist_ok=True)


    def save(self, frame):

        temp_path = self.path + ".tmp"

        success = cv2.imwrite(
            temp_path,
            frame,
            [
                cv2.IMWRITE_JPEG_QUALITY,
                self.quality
            ]
        )

        if not success:
            raise RuntimeError("Failed to write JPEG")

        os.replace(
            temp_path,
            self.path
        )