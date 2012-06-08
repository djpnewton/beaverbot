#!/usr/bin/python

import cv
import glob
import math

if __name__ == "__main__":
    # get list of image files
    clist = glob.glob("coke*.png")

    size = 640, 480
    hsv_frame = cv.CreateImage(size, cv.IPL_DEPTH_8U, 3)
    thresholded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
    thresholded2 = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
    dilate_eroded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)

    # create some color stuff
    sat_min, sat_max = 210, 255
    bri_min, bri_max = 210, 255
    hsv_min = cv.Scalar(175, sat_min, bri_min, 0);
    hsv_max = cv.Scalar(180, sat_max, bri_max, 0);
    hsv_min2 = cv.Scalar(0, sat_min, bri_min, 0);
    hsv_max2 = cv.Scalar(5, sat_max, bri_max, 0);

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

        # dilate and erode to try to get a solid region of interest
        cv.Dilate(thresholded, dilate_eroded, None, 20)
        cv.Erode(dilate_eroded, dilate_eroded, None, 20)

        # show dilate and eroded image
        cv.ShowImage('dilate and erode', dilate_eroded)

        # smooth thresholded image
        #cv.Smooth(thresholded, thresholded, cv.CV_GAUSSIAN, 9, 9)

        # hough lines 
        #storage = cv.CreateMemStorage(0)
        #lines = cv.HoughLines2(thresholded, storage, cv.CV_HOUGH_STANDARD, 1, cv.CV_PI/180, 100, 0, 0)
        #for (rho, theta) in lines:
        #    a = math.cos(theta)
        #    b = math.sin(theta)
        #    x0 = a * rho
        #    y0 = b * rho
        #    pt1 = (cv.Round(x0 + 1000*(-b)), cv.Round(y0 + 1000*(a)))
        #    pt2 = (cv.Round(x0 - 1000*(-b)), cv.Round(y0 - 1000*(a)))
        #    cv.Line(image, pt1, pt2, cv.RGB(255, 0, 0), 3, 8)
        #cv.ShowImage('lines', image)

        # contours
        storage = cv.CreateMemStorage(0)
        contours = cv.FindContours(dilate_eroded, storage)
        cv.DrawContours(image, contours, cv.RGB(0, 0, 255), cv.RGB(0, 0, 0), 1, 2)
        cv.ShowImage('contours', image)
        
        # wait for key press before showing next image
        key = cv.WaitKey(-1)
        # if escape pressed then exit
        key &= 0xff
        if key == 27:
            break


