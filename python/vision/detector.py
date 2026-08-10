import cv2


class PersonDetector:

    def __init__(
        self,
        confidence=0.5
    ):

        model_path = (
            "/opt/guard-system/models/"
            "mobilenet-ssd/"
            "mobilenet_iter_73000.caffemodel"
        )

        config_path = (
            "/opt/guard-system/models/"
            "mobilenet-ssd/"
            "MobileNetSSD_deploy.prototxt"
        )

        print(
            "Loading MobileNet-SSD..."
        )

        self.net = cv2.dnn.readNet(
            model_path,
            config_path,
            "Caffe"
        )

        self.confidence = float(
            confidence
        )

        # MobileNet-SSD VOC class ID
        # person = 15
        self.person_class = 15

        print(
            "MobileNet-SSD loaded successfully"
        )

    def detect(self, frame):

        height, width = frame.shape[:2]

        blob = cv2.dnn.blobFromImage(
            cv2.resize(
                frame,
                (300, 300)
            ),
            0.007843,
            (300, 300),
            127.5
        )

        self.net.setInput(
            blob
        )

        detections = self.net.forward()

        persons = []

        for i in range(
            detections.shape[2]
        ):

            confidence = float(
                detections[0, 0, i, 2]
            )

            class_id = int(
                detections[0, 0, i, 1]
            )

            if (
                class_id == self.person_class
                and confidence >= self.confidence
            ):

                x1 = int(
                    detections[0, 0, i, 3] * width
                )

                y1 = int(
                    detections[0, 0, i, 4] * height
                )

                x2 = int(
                    detections[0, 0, i, 5] * width
                )

                y2 = int(
                    detections[0, 0, i, 6] * height
                )

                # Clamp coordinates to the image.
                x1 = max(
                    0,
                    min(x1, width - 1)
                )

                y1 = max(
                    0,
                    min(y1, height - 1)
                )

                x2 = max(
                    0,
                    min(x2, width - 1)
                )

                y2 = max(
                    0,
                    min(y2, height - 1)
                )

                box_width = x2 - x1
                box_height = y2 - y1

                if (
                    box_width <= 0
                    or box_height <= 0
                ):
                    continue

                persons.append(
                    {
                        "x": x1,
                        "y": y1,
                        "width": box_width,
                        "height": box_height,
                        "confidence": confidence
                    }
                )

        return persons
