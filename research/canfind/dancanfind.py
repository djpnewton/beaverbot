#!/usr/bin/python

import cv
import glob

if __name__ == "__main__":
    # get list of image files
    clist = glob.glob("coke*.png")

    size = 640, 480
    hsv_frame = cv.CreateImage(size, cv.IPL_DEPTH_8U, 3)
    thresholded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
    thresholded2 = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)

    # create some color stuff
    sat_min, sat_max = 200, 255
    bri_min, bri_max = 200, 255
    hsv_min = cv.Scalar(170, sat_min, bri_min, 0);
    hsv_max = cv.Scalar(180, sat_max, bri_max, 0);
    hsv_min2 = cv.Scalar(0, sat_min, bri_min, 0);
    hsv_max2 = cv.Scalar(10, sat_max, bri_max, 0);

    for f in clist:
        # load and show image
        image = cv.LoadImage(f, cv.CV_LOAD_IMAGE_COLOR)
        cv.ShowImage('cam', image)

        # threshold based on HSV values
        cv.CvtColor(image, hsv_frame, cv.CV_BGR2HSV)
        cv.InRangeS(hsv_frame, hsv_min, hsv_max, thresholded)
        cv.InRangeS(hsv_frame, hsv_min2, hsv_max2, thresholded2)
        cv.Or(thresholded, thresholded2, thresholded)

        # show thresholded image
        cv.ShowImage('thresh', thresholded)

        
        # wait for key press before showing next image
        key = cv.WaitKey(-1)
        # if escape pressed then exit
        key &= 0xff
        if key == 27:
            break


