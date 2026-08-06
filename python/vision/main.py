import time

from config import Config
from camera import Camera
from jpeg_writer import JpegWriter
from detector import PersonDetector


def main():

    config = Config("configs/settings.json")

    camera = Camera(config.camera())

    vision = config.vision()

    writer = JpegWriter(
        vision.jpeg_path,
        vision.jpeg_quality
    )

    detector = PersonDetector()

    camera.open()

    print("Camera opened successfully")

    frame_count = 0
    start_time = time.time()

    try:

        while True:

            frame = camera.read()

            person_count = detector.detect(frame)

            

            writer.save(frame)

            frame_count += 1

            if frame_count % 30 == 0:

                elapsed = time.time() - start_time
                fps = frame_count / elapsed

                print(
                    f"Frames: {frame_count:5d} | "
                    f"FPS: {fps:.2f}"
                    f"Persons: {person_count}"
                )
                

    except KeyboardInterrupt:

        print("\nStopping...")

    finally:

        camera.release()
        print("Camera released")


if __name__ == "__main__":
    main()