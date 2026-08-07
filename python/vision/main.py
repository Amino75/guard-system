import time

from config import Config
from camera import Camera
from jpeg_writer import JpegWriter
from detector import PersonDetector


def main():

    config = Config("configs/settings.json")

    camera = Camera(config.camera())

    vision = config.vision()

    detector = PersonDetector()

    writer = JpegWriter(
        vision.jpeg_path,
        vision.jpeg_quality
    )


    camera.open()

    print("Camera opened successfully")

    frame_count = 0
    start_time = time.time()

    person_count = 0

    DETECTION_INTERVAL = 10


    try:

        while True:

            frame = camera.read()

            frame_count += 1


        # Run YOLO only every N frames
            if frame_count % DETECTION_INTERVAL == 0:

                start = time.time()

                person_count = detector.detect(frame)

                end = time.time()

                print(
                    f"MobileNet-SSD inference: {(end-start)*1000:.1f} ms | "
                    f"Persons: {person_count}"
                )


        # Always update image
            writer.save(frame)



            if frame_count % 30 == 0:

                elapsed = time.time() - start_time

                fps = frame_count / elapsed

                print(
                    f"Camera FPS: {fps:.2f} | "
                    f"Persons: {person_count}"
                )


    except KeyboardInterrupt:

        print("\nStopping...")


    finally:

        camera.release()

        print("Camera released")


if __name__ == "__main__":
    main()
