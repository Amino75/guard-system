import cv2
import time

from detector import PersonDetector


IMAGE = "html/live/latest.jpg"


print("Loading image...")

frame = cv2.imread(IMAGE)

if frame is None:
    raise RuntimeError(
        f"Could not load image: {IMAGE}"
    )


height, width = frame.shape[:2]

print(
    f"Image: {width} x {height}"
)


detector = PersonDetector()


print("Running MobileNet-SSD...")
print()


start = time.time()

person_count = detector.detect(frame)

end = time.time()


inference_ms = (
    end - start
) * 1000.0


print(
    f"Inference time: {inference_ms:.1f} ms"
)

print(
    f"Persons detected: {person_count}"
)
