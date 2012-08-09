#!/usr/bin/python

import sys
import cv
import math

# load image from command line
im_orig = cv.LoadImageM(sys.argv[1], cv.CV_LOAD_IMAGE_COLOR)

# create grayscale version of image
im_gray = cv.CreateImage((im_orig.width, im_orig.height), cv.IPL_DEPTH_8U, 1)
cv.CvtColor(im_orig, im_gray, cv.CV_RGB2GRAY)

# extract features from grayscale image
(keypoints, descriptors) = cv.ExtractSURF(im_gray, None, cv.CreateMemStorage(), (0, 5500, 3, 1))

# draw features on original image
color = cv.RGB(255, 0, 0)
for ((x, y), laplacian, size, dir, hessian) in keypoints:
    print "x=%d, y=%d, laplacian=%d, size=%d, dir=%f, hessian=%f" % (x, y, laplacian, size, dir, hessian)
    h = size / 2
    cv.Circle(im_orig, (int(x), int(y)), int(size/2), color, 1, cv.CV_AA)
    continue
    rad = math.radians(dir)
    x2 = math.cos(rad) * 0 - math.sin(rad) * h
    y2 = math.sin(rad) * 0 - math.cos(rad) * h
    x2 += x
    y2 += y
    cv.Line(im_orig, (int(x), int(y)), (int(x2), int(y2)), color, 1, cv.CV_AA)


# show result
cv.ShowImage("Features", im_orig)
cv.WaitKey(-1)
cv.WaitKey(-1)
cv.WaitKey(-1)




