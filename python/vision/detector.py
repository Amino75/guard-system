import cv2


class PersonDetector:

    def __init__(self):

        model_path = "../../models/MobileNetSSD_deploy.caffemodel"
        config_path = "../../models/MobileNetSSD_deploy.prototxt"

        self.net = cv2.dnn.readNetFromCaffe(
            config_path,
            model_path
        )

        self.confidence = 0.5

        # MobileNet-SSD class ID for person
        self.person_class = 15

        print("MobileNet SSD loaded")


    def detect(self, frame):

        blob = cv2.dnn.blobFromImage(
            cv2.resize(frame, (300, 300)),
            0.007843,
            (300, 300),
            127.5
        )

        self.net.setInput(blob)

        detections = self.net.forward()

        count = 0

        for i in range(detections.shape[2]):

            confidence = detections[0, 0, i, 2]

            class_id = int(
                detections[0, 0, i, 1]
            )

            if (
                class_id == self.person_class
                and confidence >= self.confidence
            ):
                count += 1

        return count
