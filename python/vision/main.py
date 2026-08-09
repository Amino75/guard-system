import time

from config import Config
from detector import PersonDetector
from shared_memory import SharedMemoryReader
from pathlib import Path

DETECTION_DISABLE_FILE = Path(
    "/tmp/guard-system-detection.disabled"
)

def main():

    print("========================================")
    print("Guard System Vision")
    print("========================================")


    config = Config(
        "configs/settings.json"
    )


    detection = config.detection()


    print(
        f"Detection enabled: {detection.enabled}"
    )

    print(
        f"Detection interval: "
        f"{detection.interval} frames"
    )

    print(
        f"Detection confidence: "
        f"{detection.confidence:.2f}"
    )


    shm = SharedMemoryReader()


    try:

        shm.open()


        detector = None


        if detection.enabled:

            detector = PersonDetector(detection.confidence)

        else:

            print(
                "Person detection is DISABLED"
            )


        last_sequence = None

        frame_count = 0

        detection_count = 0

        person_count = 0

        start_time = time.time()


        print(
            "Vision system started"
        )


        while True:

            frame, sequence = shm.read_frame()


            if frame is None:

                time.sleep(0.005)

                continue


            # Ignore the same camera frame.
            if sequence == last_sequence:

                time.sleep(0.002)

                continue


            last_sequence = sequence

            frame_count += 1


            detection_runtime_enabled = (
                detection.enabled
                and not DETECTION_DISABLE_FILE.exists()
            )

            if frame_count % 30 == 0:

                if detection_runtime_enabled:
                    print("Detection state: ENABLED")
                else:
                    print("Detection state: DISABLED")

            if (
                detection_runtime_enabled
                and detector is not None
                and frame_count % detection.interval == 0
            ):

                start = time.time()


                person_count = detector.detect(
                    frame
                )


                end = time.time()


                detection_count += 1


                shm.set_person_count(
                    person_count
                )


                print(
                    f"MobileNet-SSD inference: "
                    f"{(end - start) * 1000:.1f} ms | "
                    f"Persons: {person_count}"
                )


            elif not detection_runtime_enabled:

                person_count = 0

                shm.set_person_count(
                    0
                )


            if frame_count % 30 == 0:

                elapsed = (
                    time.time() - start_time
                )

                fps = (
                    frame_count / elapsed
                    if elapsed > 0
                    else 0
                )


                print(
                    f"Vision FPS: {fps:.2f} | "
                    f"Persons: {person_count}"
                )


    except KeyboardInterrupt:

        print(
            "\nStopping vision system..."
        )


    finally:

        shm.close()


if __name__ == "__main__":

    main()
