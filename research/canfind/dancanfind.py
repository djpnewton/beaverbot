#!/usr/bin/python

import cv
import glob
import math

RED = 0
ADAPT = 1
INDIGO_LASER = 2

def find_can(mode, image, show_images=False, dilate_and_erode=False, image_buffers=None):
    # create image buffers
    size = image.width, image.height
    if mode == RED or mode == INDIGO_LASER:
        if image_buffers:
            # reuse image buffers
            hsv_frame, thresholded, thresholded2, dilate_eroded = image_buffers
        else:
            # create new image buffers
            hsv_frame = cv.CreateImage(size, cv.IPL_DEPTH_8U, 3)
            thresholded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
            if mode == RED:
                thresholded2 = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
            else:
                thresholded2 = None
            dilate_eroded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
            image_buffers = (hsv_frame, thresholded, thresholded2, dilate_eroded)

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
            hsv_max = cv.Scalar(160, 255, 255, 0);
    elif mode == ADAPT:
        if image_buffers:
            bw, thresholded, dilate_eroded = image_buffers
        else:
            bw = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
            thresholded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
            dilate_eroded = cv.CreateImage(size, cv.IPL_DEPTH_8U, 1)
            image_buffers = (bw, thresholded, dilate_eroded)

    # show image
    if show_images:
        cv.ShowImage('cam', image)

    if mode == RED or mode == INDIGO_LASER:
        # threshold based on HSV values
        cv.CvtColor(image, hsv_frame, cv.CV_BGR2HSV)
        cv.InRangeS(hsv_frame, hsv_min, hsv_max, thresholded)
        if mode == RED:
            cv.InRangeS(hsv_frame, hsv_min2, hsv_max2, thresholded2)
            cv.Or(thresholded, thresholded2, thresholded)

        # show thresholded image
        if show_images:
            cv.ShowImage('thresh', thresholded)

        # dilate and erode to try to get a solid region of interest
        if dilate_and_erode:
            cv.Dilate(thresholded, dilate_eroded, None, 20)
            cv.Erode(dilate_eroded, dilate_eroded, None, 20)

            # smooth image
            #cv.Smooth(dilate_eroded, dilate_eroded, cv.CV_GAUSSIAN, 9, 9)

            # show dilate and eroded image
            if show_images:
                cv.ShowImage('dilate and erode', dilate_eroded)
        else:
            dilate_eroded = thresholded

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
        #if show_images:
        #    cv.ShowImage('lines', image)

        final = dilate_eroded

    elif mode == ADAPT:
        # adaptive threshold
        cv.CvtColor(image, bw, cv.CV_RGB2GRAY)
        cv.AdaptiveThreshold(bw, thresholded, 255, cv.CV_ADAPTIVE_THRESH_GAUSSIAN_C, cv.CV_THRESH_BINARY, 31, -10)

        # show thresholded image
        if show_images:
            cv.ShowImage('thresh', thresholded)

        # smooth image
        cv.Smooth(thresholded, thresholded, cv.CV_GAUSSIAN, 9, 9)

        # show smoothed
        if show_images:
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
    if biggest_contours:
        rect = cv.BoundingRect(biggest_contours)
        cv.Rectangle(image, (rect[0], rect[1]), (rect[0] + rect[2], rect[1] + rect[3]), cv.RGB(0, 255, 0))
        if show_images:
            cv.ShowImage('contours', image)
        return rect, image_buffers
    return None, image_buffers

if __name__ == "__main__":

    # set mode
    mode = INDIGO_LASER

    # get list of image files
    if mode == RED:
        clist = glob.glob("coke*.png")
    elif mode == ADAPT:
        clist = glob.glob("coke*.png") + glob.glob("white*.jpg")
    elif mode == INDIGO_LASER:
        clist = glob.glob("indigo_laser*.jpg")

    for f in clist:
        # load image
        image = cv.LoadImage(f, cv.CV_LOAD_IMAGE_COLOR)
        # process image
        find_can(mode, image, show_images=True, dilate_and_erode=True);
        # wait for key press before showing next image
        key = cv.WaitKey(-1)
        # if escape pressed then exit
        key &= 0xff
        if key == 27:
            break


