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


        count = 0


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

                count += 1


        return count
