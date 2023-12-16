import cv2 as cv
import numpy as np
from matplotlib import pyplot as plt
import os
import time

# There are 6 comparison methods to choose from:
# TM_CCOEFF, TM_CCOEFF_NORMED, TM_CCORR, TM_CCORR_NORMED, TM_SQDIFF, TM_SQDIFF_NORMED
# You can see the differences at a glance here:
# https://docs.opencv.org/master/d4/dc6/tutorial_py_template_matching.html
methods_dict = dict([('TM_CCOEFF', cv.TM_CCOEFF),
                     ('TM_CCOEFF_NORMED', cv.TM_CCOEFF_NORMED),
                     ('TM_CCORR', cv.TM_CCORR),
                     ('TM_CCORR_NORMED', cv.TM_CCORR_NORMED),
                     ('TM_SQDIFF', cv.TM_SQDIFF),
                     ('TM_SQDIFF_NORMED', cv.TM_SQDIFF_NORMED)])


def detect(image, template, method, threshold):
    res = cv.matchTemplate(image, template, method)
    # Apply threshold to detect multiple objects
    if method in ['TM_SQDIFF', 'TM_SQDIFF_NORMED']:
        loc = np.where(res <= threshold)
    else:
        loc = np.where(res >= threshold)
    return loc


def load_capture():
    capture = cv.VideoCapture("data/mario.mp4")
    return capture


capture = load_capture()
total_frames = int(capture.get(cv.CAP_PROP_FRAME_COUNT))
current_frame = 0
start = time.time_ns()

# Load template for empty block
empty_block = cv.imread(
    'examples/templates/empty_block.png', cv.IMREAD_GRAYSCALE)
assert empty_block is not None, "file could not be read, check with os.path.exists()"

# store data
frame_time_hist = np.zeros(shape=(total_frames, 1))
objects_detected = np.zeros(shape=(total_frames, 4))


while True:
    # Load single frame from video
    _, frame = capture.read()
    if frame is None:
        print("End of stream")
        break
    frame_gray = cv.cvtColor(frame, cv.COLOR_BGR2GRAY)

    current_frame += 1

    # detect empty block
    empty_block_loc = detect(frame_gray, empty_block, cv.TM_CCOEFF_NORMED, 0.9)
    print(str(len(empty_block_loc[0])) +
          " empty block detected at frame " + str(current_frame))
    # TODO: detect brick_block, hard_block, and undestructible_block
    # Insert your code here

    # TODO: measure and store the execution time and object count of each frame.
    frame_time = 0
    frame_time_hist[current_frame - 1] = frame_time
    objects_detected[current_frame - 1] = [len(empty_block_loc[0]), 0, 0, 0]

os.makedirs("runs/", exist_ok=True)
np.save('runs/frame_time.npy', frame_time_hist)
np.save('runs/objects_count.npy', objects_detected)
print("Total frames: " + str(total_frames))
