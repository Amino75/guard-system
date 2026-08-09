import json

from dataclasses import dataclass
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]


@dataclass
class CameraConfig:

    device: int
    width: int
    height: int
    fps: int


@dataclass
class VisionConfig:

    jpeg_path: Path
    jpeg_quality: int


@dataclass
class DetectionConfig:

    enabled: bool
    interval: int
    confidence: float


class Config:

    def __init__(self, config_path: str):

        config_file = PROJECT_ROOT / config_path

        if not config_file.exists():

            raise FileNotFoundError(
                f"Configuration file not found: {config_file}"
            )

        with open(config_file, "r") as f:

            self.data = json.load(f)


    def camera(self) -> CameraConfig:

        camera = self.data["camera"]

        return CameraConfig(
            device=camera["device"],
            width=camera["width"],
            height=camera["height"],
            fps=camera["fps"],
        )


    def vision(self) -> VisionConfig:

        vision = self.data["vision"]

        return VisionConfig(
            jpeg_path=PROJECT_ROOT / vision["jpeg_path"],
            jpeg_quality=vision["jpeg_quality"],
        )


    def detection(self) -> DetectionConfig:

        detection = self.data["detection"]

        return DetectionConfig(
            enabled=bool(
                detection.get("enabled", True)
            ),

            interval=max(
                1,
                int(
                    detection.get("interval", 10)
                )
            ),

            confidence=float(
                detection.get("confidence", 0.5)
            ),
        )
