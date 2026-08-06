from pathlib import Path
import os

import cv2


class JpegWriter:
    """
    Continuously writes the latest frame as a JPEG image.
    """

    def __init__(self, output_path: str, quality: int = 90):

        self.output_path = Path(output_path)
        self.quality = quality

        self.output_path.parent.mkdir(
            parents=True,
            exist_ok=True
        )

    def save(self, frame):

        temp_path = self.output_path.with_name(
            self.output_path.stem + ".tmp.jpg"
        )

        success = cv2.imwrite(
            str(temp_path),
            frame,
            [
                cv2.IMWRITE_JPEG_QUALITY,
                self.quality
            ]
        )

        if not success:
            raise RuntimeError(
                f"Unable to save image: {temp_path}"
            )

        os.replace(
            temp_path,
            self.output_path
        )