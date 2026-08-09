import cv2
import os
import time

CAMERA = "/dev/video1"

MODEL = "models/mobilenet-ssd/mobilenet_iter_73000.caffemodel"
PROTO = "models/mobilenet-ssd/MobileNetSSD_deploy.prototxt"

OUTPUT = "html/live/mobilenet_test.jpg"

# MobileNet-SSD class labels
CLASSES = [
    "background",
    "aeroplane",
    "bicycle",
    "bird",
    "boat",
    "bottle",
    "bus",
    "car",
    "cat",
    "chair",
    "cow",
    "diningtable",
    "dog",
    "horse",
    "motorbike",
    "person",
    "pottedplant",
    "sheep",
    "sofa",
    "train",
    "tvmonitor"
]

PERSON_CLASS = 15
CONFIDENCE_THRESHOLD = 0.20


print("========================================")
print(" MobileNet-SSD Camera Test")
print("========================================")

print()
print("Loading MobileNet-SSD...")

net = cv2.dnn.readNet(
    MODEL,
    PROTO
)

net.setPreferableBackend(
    cv2.dnn.DNN_BACKEND_OPENCV
)

net.setPreferableTarget(
    cv2.dnn.DNN_TARGET_CPU
)

print("MobileNet-SSD loaded successfully")

print()
print("Opening camera:", CAMERA)

cap = cv2.VideoCapture(
    CAMERA,
    cv2.CAP_V4L2
)

if not cap.isOpened():
    print("ERROR: Could not open camera")
    raise SystemExit(1)

cap.set(
    cv2.CAP_PROP_FRAME_WIDTH,
    640
)

cap.set(
    cv2.CAP_PROP_FRAME_HEIGHT,
    480
)

cap.set(
    cv2.CAP_PROP_FPS,
    30
)

print("Camera opened successfully")

# Give the camera a moment to stabilize
for _ in range(10):
    ret, frame = cap.read()

if not ret:
    print("ERROR: Could not capture frame")
    cap.release()
    raise SystemExit(1)

print(
    "Captured frame:",
    frame.shape[1],
    "x",
    frame.shape[0]
)

print()
print("Running MobileNet-SSD...")
print()

start = time.time()

blob = cv2.dnn.blobFromImage(
    cv2.resize(frame, (300, 300)),
    0.007843,
    (300, 300),
    127.5
)

net.setInput(blob)

detections = net.forward()

elapsed = time.time() - start

print(
    "Inference time: %.1f ms"
    % (elapsed * 1000.0)
)

print()
print("ALL DETECTIONS")
print("----------------------------------------")

person_count = 0
detection_count = 0

height, width = frame.shape[:2]

for i in range(detections.shape[2]):

    confidence = float(
        detections[0, 0, i, 2]
    )

    class_id = int(
        detections[0, 0, i, 1]
    )

    if confidence < CONFIDENCE_THRESHOLD:
        continue

    detection_count += 1

    if class_id >= 0 and class_id < len(CLASSES):
        label = CLASSES[class_id]
    else:
        label = "unknown"

    print(
        "Detection %d: class=%d (%s), confidence=%.3f"
        % (
            detection_count,
            class_id,
            label,
            confidence
        )
    )

    if class_id == PERSON_CLASS:
        person_count += 1

        box = detections[0, 0, i, 3:7]

        x1 = int(box[0] * width)
        y1 = int(box[1] * height)
        x2 = int(box[2] * width)
        y2 = int(box[3] * height)

        x1 = max(0, min(x1, width - 1))
        y1 = max(0, min(y1, height - 1))
        x2 = max(0, min(x2, width - 1))
        y2 = max(0, min(y2, height - 1))

        cv2.rectangle(
            frame,
            (x1, y1),
            (x2, y2),
            (0, 255, 0),
            2
        )

        text = "PERSON %.1f%%" % (
            confidence * 100.0
        )

        cv2.putText(
            frame,
            text,
            (x1, max(20, y1 - 10)),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (0, 255, 0),
            2
        )

print("----------------------------------------")
print(
    "Detections above %.0f%%: %d"
    % (
        CONFIDENCE_THRESHOLD * 100,
        detection_count
    )
)

print(
    "Persons detected: %d"
    % person_count
)

os.makedirs(
    os.path.dirname(OUTPUT),
    exist_ok=True
)

cv2.imwrite(
    OUTPUT,
    frame
)

print()
print("Saved result:")
print(OUTPUT)

cap.release()

print()
print("Camera released")
print("========================================")
