from detector import PersonDetector
import numpy as np

detector = PersonDetector()

dummy = np.zeros((480, 640, 3), dtype=np.uint8)

outputs = detector.session.run(
    None,
    {
        detector.input_name: np.expand_dims(
            np.transpose(
                dummy.astype(np.float32) / 255.0,
                (2, 0, 1)
            ),
            axis=0
        )
    }
)

print(type(outputs))
print(len(outputs))

for i, out in enumerate(outputs):
    print(i, out.shape)