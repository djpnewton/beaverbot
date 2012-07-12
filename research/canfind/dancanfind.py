#!/usr/bin/python

import cv
import glob
import math

RED = 0
ADAPT = 1
INDIGO_LASER = 2
mode = INDIGO_LASER

if __name__ == "__main__":

    # get list of image files
    if mode == RED:
        clist = glob.glob("coke*.png")
    elif mode == ADAPT:
        clist = glob.glob("coke*.png") + glob.glob("white*.jpg")
    elif mode == INDIGO_LASER:
        clist = glob.glob("indigo_laser*.jpg")

    # create image buffers
    size = 640, 480
    if mode == RED or mode == INDIGO_LASER:
        hsv_frame = cv.CreateImage(size, cv.IPL_DEPTH_8U, 3)
        thresholded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
        thresholded2 = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
        dilate_eroded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)

        if mode == RED:
            # create some color stuff
            sat_min, sat_max = 210, 255
            bri_min, bri_max = 210, 255
            hsv_min = cv.Scalar(175, sat_min, bri_min, 0);
            hsv_max = cv.Scalar(180, sat_max, bri_max, 0);
            hsv_min2 = cv.Scalar(0, sat_min, bri_min, 0);
            hsv_max2 = cv.Scalar(5, sat_max, bri_max, 0);
        else:
            # create some color stuff
            hsv_min = cv.Scalar(120, 50, 50, 0);
            hsv_max = cv.Scalar(150, 250, 200, 0);
            hsv_min2 = cv.Scalar(150, 100, 100, 100);
            hsv_max2 = cv.Scalar(151, 101, 101, 101);
    elif mode == ADAPT:
        bw = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
        thresholded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
        dilate_eroded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)

    for f in clist:
        # load and show image
        image = cv.LoadImage(f, cv.CV_LOAD_IMAGE_COLOR)
        cv.ShowImage('cam', image)

        if mode == RED or mode == INDIGO_LASER:
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

            # smooth image
            #cv.Smooth(dilate_eroded, dilate_eroded, cv.CV_GAUSSIAN, 9, 9)

            # show dilate and eroded image
            cv.ShowImage('dilate and erode', dilate_eroded)

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

            final = dilate_eroded

        elif mode == ADAPT:
            # adaptive threshold
            cv.CvtColor(image, bw, cv.CV_RGB2GRAY)
            cv.AdaptiveThreshold(bw, thresholded, 255, cv.CV_ADAPTIVE_THRESH_GAUSSIAN_C, cv.CV_THRESH_BINARY, 31, -10)

            # show thresholded image
            cv.ShowImage('thresh', thresholded)

            # smooth image
            cv.Smooth(thresholded, thresholded, cv.CV_GAUSSIAN, 9, 9)

            # show smoothed
            cv.ShowImage('smoothed', thresholded)

            final = thresholded

        # contours + largest area bounding rect
        storage = cv.CreateMemStorage(0)
        contours = cv.FindContours(final, storage)
        biggest_contours = contours
        biggest_contours_area = -1
        while contours:
            cv.DrawContours(image, contours, cv.RGB(0, 0, 255), cv.RGB(0, 0, 0), 1, 2)
            area = cv.ContourArea(contours)
            if area > biggest_contours_area:
                biggest_contours = contours
                biggest_contours_area = area
            contours = contours.h_next()
        rect = cv.BoundingRect(biggest_contours)
        cv.Rectangle(image, (rect[0], rect[1]), (rect[0] + rect[2], rect[1] + rect[3]), cv.RGB(0, 255, 0))
        cv.ShowImage('contours', image)
        
        # wait for key press before showing next image
        key = cv.WaitKey(-1)
        # if escape pressed then exit
        key &= 0xff
        if key == 27:
            break


