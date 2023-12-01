import cv2 as cv
import numpy as np
from matplotlib import pyplot as plt
import sys
import os

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

# total arguments
argc = len(sys.argv)
if argc < 3:
    print("Not enough parameters")
    print("Usage:")
    print(sys.argv[0] + " <image_name> <template_name> [<mask_name>] [threshold]")
    sys.exit()

img_src = sys.argv[1] 
tmp_src = sys.argv[2]
method = sys.argv[3] if argc > 3 else 'TM_CCOEFF_NORMED'
threshold = float(sys.argv[4]) if argc > 4 else 0.9

# Source image
img_rgb = cv.imread(img_src)
assert img_rgb is not None, "file could not be read, check with os.path.exists()"
img_gray = cv.cvtColor(img_rgb, cv.COLOR_BGR2GRAY)

# Template image
template = cv.imread(tmp_src, cv.IMREAD_GRAYSCALE)
assert template is not None, "file could not be read, check with os.path.exists()"
w, h = template.shape[::-1]

# Template matching
res = cv.matchTemplate(img_gray,template, methods_dict[method])

# Apply threshold to detect multiple objects
if method in ['TM_SQDIFF', 'TM_SQDIFF_NORMED']:
    loc = np.where( res <= threshold)
else:
    loc = np.where( res >= threshold)

# Draw bounding box
for pt in zip(*loc[::-1]):
    cv.rectangle(img_rgb, pt, (pt[0] + w, pt[1] + h), (0,0,255), 2)

print(str(len(loc[0]))+" objects detected using method " + method + " with a threshold of " + str(threshold))
os.makedirs("runs/", exist_ok=True)
cv.imwrite('runs/tm_result.png',img_rgb)
print("Result saved to runs/tm_result.png")