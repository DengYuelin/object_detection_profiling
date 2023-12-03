import numpy as np
import cv2
import onnxruntime
import torch
from utils.general import non_max_suppression


class Detection:
    def __init__(self, x, y, w, h, conf, id):
        self.class_id = int(id)
        self.confidence = conf
        self.box_upper_left = (x, y)
        self.box_lower_right = (x + w, y + h)

    @classmethod
    def draw(cls, img, colormap):
        return cv2.rectangle(
            img, cls.box_upper_left, cls.box_lower_right, colormap[cls.class_id]
        )


def main():
    input = "data/mario.mp4"
    classes = "network/classes_mario.txt"
    model = "network/yolov5s_mario.onnx"

    # set model parameters
    conf_thres = 0.4  # NMS confidence threshold
    iou_thres = 0.4  # NMS IoU threshold

    # load input
    cap = cv2.VideoCapture(input)
    all_frames = np.array([])
    n_frames = 0
    if cap.isOpened():
        n_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
        ret, frame = cap.read()
        if ret is False:
            raise RuntimeError("Input video is empty.")
        all_frames.resize([n_frames, frame.shape[0], frame.shape[1], frame.shape[2]])
        all_frames[0] = frame
    else:
        raise RuntimeError("Input cannot be opened.")
    for n in range(1, n_frames):
        # vid_capture.read() methods returns a tuple, first element is a bool
        # and the second is frame
        ret, frame = cap.read()
        if ret is True:
            all_frames[n] = frame
        else:
            break

    # load model
    session = onnxruntime.InferenceSession(model, None)
    input_name = session.get_inputs()[0].name
    output_name = session.get_outputs()[0].name

    # load classes
    classes_list = []
    with open(classes, "r") as f:
        classes_list = f.readlines()

    for n in range(0, n_frames):
        # pre-processing
        max_dim = np.max(all_frames[n].shape)
        nn_input_frame = all_frames[n]
        nn_input_frame.resize(max_dim, max_dim, 3)  # zero-padding
        nn_input_frame = cv2.resize(nn_input_frame, dsize=(640, 640))  # scaling
        nn_input_frame = nn_input_frame.transpose()[-1:-4:-1]  # dim switching
        nn_input_frame = [nn_input_frame.astype(np.float32) / 255.0]  # normalizing

        # main inference
        frame_result = session.run([output_name], {input_name: nn_input_frame})[0]

        # post-processing
        pred = non_max_suppression(torch.from_numpy(frame_result), conf_thres, iou_thres)
        scale_factor = max_dim / 640
        detection_list = []
        for detection in pred[0]:
            x = detection[0]
            y = detection[1]
            w = detection[2]
            h = detection[3]
            left = (x - 0.5 * w) * scale_factor
            upper = (y - 0.5 * w) * scale_factor
            w *= scale_factor
            h *= scale_factor
            detection_list.append(Detection(left, upper, w, h, detection[4], detection[5]))

        print(str(len(detection_list)) + " identified objects in frame " + str(n))


if __name__ == "__main__":
    main()
