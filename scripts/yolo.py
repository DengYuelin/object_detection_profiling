import cv2
import time
import numpy as np
import sys
import os


def load_classes():
    class_list = []
    with open("network/classes_mario.txt", "r") as f:
        class_list = [cname.strip() for cname in f.readlines()]
    return class_list


def build_model(is_cuda):
    net = cv2.dnn.readNet("network/yolov5s_mario.onnx")
    if is_cuda:
        print("Attempty to use CUDA")
        net.setPreferableBackend(cv2.dnn.DNN_BACKEND_CUDA)
        net.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA_FP16)
    else:
        print("Running on CPU")
        net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
        net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)
    return net


colors = [(255, 255, 0), (0, 255, 0), (0, 255, 255), (255, 0, 0)]

INPUT_WIDTH = 640
INPUT_HEIGHT = 640
SCORE_THRESHOLD = 0.2
NMS_THRESHOLD = 0.4
CONFIDENCE_THRESHOLD = 0.4


def detect(image, net):
    # --------------------Pre-process--------------------
    blob = cv2.dnn.blobFromImage(
        image, 1/255.0, (INPUT_WIDTH, INPUT_HEIGHT), swapRB=True, crop=False)
    net.setInput(blob)
    # --------------------Main Inference-----------------
    preds = net.forward()
    return preds


def load_capture():
    capture = cv2.VideoCapture("data/mario.mp4")
    return capture


class_list = load_classes()


def wrap_detection(input_image, output_data):
    # --------------------Post-process-------------------
    class_ids = []
    confidences = []
    boxes = []

    rows = output_data.shape[0]

    image_width, image_height, _ = input_image.shape

    x_factor = image_width / INPUT_WIDTH
    y_factor = image_height / INPUT_HEIGHT

    for r in range(rows):
        row = output_data[r]
        confidence = row[4]
        # Discard bad detections and continue.
        if confidence >= 0.4:
            classes_scores = row[5:]
            _, _, _, max_indx = cv2.minMaxLoc(classes_scores)
            # Perform minMaxLoc and acquire index of best class score.
            class_id = max_indx[1]
            # Continue if the class score is above the threshold.
            if (classes_scores[class_id] > .25):
                # Store class ID and confidence in the pre-defined respective vectors.
                confidences.append(confidence)

                class_ids.append(class_id)

                x, y, w, h = row[0].item(), row[1].item(
                ), row[2].item(), row[3].item()
                left = int((x - 0.5 * w) * x_factor)
                top = int((y - 0.5 * h) * y_factor)
                width = int(w * x_factor)
                height = int(h * y_factor)
                # Store good detections in the boxes vector.
                box = np.array([left, top, width, height])
                boxes.append(box)

    # --------------------Non Maximum Suppression--------------------
    indexes = cv2.dnn.NMSBoxes(boxes, confidences, 0.25, 0.45)

    result_class_ids = []
    result_confidences = []
    result_boxes = []

    for i in indexes:
        result_confidences.append(confidences[i])
        result_class_ids.append(class_ids[i])
        result_boxes.append(boxes[i])

    return result_class_ids, result_confidences, result_boxes


def format_yolov5(frame):

    row, col, _ = frame.shape
    _max = max(col, row)
    result = np.zeros((_max, _max, 3), np.uint8)
    result[0:row, 0:col] = frame
    return result


# is_cuda = len(sys.argv) > 1 and sys.argv[1] == "cuda"

net = build_model(False)
capture = load_capture()
total_frames = int(capture.get(cv2.CAP_PROP_FRAME_COUNT))

start = time.time_ns()
current_frame = 0

# store data
frame_time_hist = np.zeros(shape=(total_frames, 1))
objects_detected = np.zeros(shape=(total_frames, 1))

while True:

    _, frame = capture.read()
    if frame is None:
        print("End of stream")
        break

    inputImage = format_yolov5(frame)
    outs = detect(inputImage, net)

    class_ids, confidences, boxes = wrap_detection(inputImage, outs[0])

    current_frame += 1

    print("total " + str(len(boxes)) +
          " identified objects in frame " + str(current_frame))

    # TODO: measure and store the execution time and object count of each frame.
    frame_time = 0
    frame_time_hist[current_frame - 1] = frame_time
    objects_detected[current_frame - 1] = [len(boxes)]

    # Uncomment to visualize the result on a graphical system

    # for (classid, confidence, box) in zip(class_ids, confidences, boxes):
    #     color = colors[int(classid) % len(colors)]
    #     cv2.rectangle(frame, box, color, 2)
    #     cv2.rectangle(frame, (box[0], box[1] - 20),
    #                   (box[0] + box[2], box[1]), color, -1)
    #     cv2.putText(frame, class_list[classid], (box[0],
    #                 box[1] - 10), cv2.FONT_HERSHEY_SIMPLEX, .5, (0, 0, 0))

    # cv2.imshow("output", frame)

    # if cv2.waitKey(1) > -1:
    #     print("finished by user")
    #     break

os.makedirs("runs/", exist_ok=True)
np.save('runs/frame_time.npy', frame_time_hist)
np.save('runs/objects_count.npy', objects_detected)
print("Completed processing, total frames: " + str(total_frames))
