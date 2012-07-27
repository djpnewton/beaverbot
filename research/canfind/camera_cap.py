import time
import cv
import dancanfind

hunt = True

cap = cv.CaptureFromCAM(-1)
# set the width, height and exposure
cv.SetCaptureProperty(cap, cv.CV_CAP_PROP_FRAME_WIDTH, 320);
cv.SetCaptureProperty(cap, cv.CV_CAP_PROP_FRAME_HEIGHT, 240);
#cv.SetCaptureProperty(cap, cv.CV_CAP_PROP_AUTO_EXPOSURE, 0);
cv.SetCaptureProperty(cap, cv.CV_CAP_PROP_EXPOSURE, 1);

image_buffers = None
while True:
    image = cv.QueryFrame(cap)

    rect, image_buffers = dancanfind.find_can(dancanfind.INDIGO_LASER, image, image_buffers=image_buffers)

    if hunt:
        #todo
        print time.time(), rect
    else:
        key = cv.WaitKey(0)
        key &= 0xff
        print key
        if key == 27:
            break

