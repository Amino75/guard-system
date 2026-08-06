import cv2
import numpy as np
import onnxruntime as ort


class PersonDetector:

    def __init__(
        self,
        model_path="../../models/yolov8n.onnx",
        confidence=0.5
    ):

        self.confidence = confidence

        self.session = ort.InferenceSession(
            model_path,
            providers=[
                "CPUExecutionProvider"
            ]
        )

        self.input_name = (
            self.session
            .get_inputs()[0]
            .name
        )

        self.input_size = 640

        print("YOLO model loaded")


    def detect(self, frame):

        input_image = cv2.resize(
            frame,
            (
                self.input_size,
                self.input_size
            )
        )

        input_image = cv2.cvtColor(
            input_image,
            cv2.COLOR_BGR2RGB
        )

        input_image = (
            input_image.astype(np.float32)
            / 255.0
        )

        input_image = np.transpose(
            input_image,
            (2, 0, 1)
        )

        input_image = np.expand_dims(
            input_image,
            axis=0
        )


        outputs = self.session.run(
            None,
            {
                self.input_name:
                input_image
            }
        )


        return self.count_persons(
            outputs[0]
        )


    def count_persons(self, output):

        count = 0

        predictions = output[0].T


        for detection in predictions:

            class_scores = detection[4:]

            class_id = np.argmax(
                class_scores
            )

            score = class_scores[class_id]


            if (
                class_id == 0
                and score > self.confidence
            ):
                count += 1


        return count