# -*- coding: utf-8 -*-
"""

@author: othane
"""

import cv
from pylab import imread, imshow, gray, mean
import pylab
from matplotlib.pyplot import *
import subprocess
import glob
import re

def update_hist(a, b ,c , axs):
    '''
    Plot 3 histograms into ax for the 3 channel input image
    Also return a model of that histogram that includes 70% of the input image
    '''
    # Gather stats for the whole image
    stats = [[], [], []]
    (X,Y) = cv.GetSize(r)        
    for y in range(0, Y):
        for x in range(0, X):
            stats[0].append(cv.Get2D(a, y, x)[0])
            stats[1].append(cv.Get2D(b, y, x)[0])
            stats[2].append(cv.Get2D(c, y, x)[0])
    # work out metrics
    a_range = [1.2*min(stats[0]), 0.9*max(stats[0])]
    b_range = [1.2*min(stats[1]), 0.9*max(stats[1])]
    c_range = [1.2*min(stats[2]), 0.9*max(stats[2])]
    model = [a_range, b_range, c_range]
    # show stats
    axs[0].hist(stats[0])
    axs[0].set_ylim((0, 200000))
    axs[1].hist(stats[1])
    axs[1].set_ylim((0, 200000))
    axs[2].hist(stats[2])
    axs[2].set_ylim((0, 200000))
    return (model)   
            
def cannyseg(image):
    global contour, bbox, bbox2   
    # get edges via canny operator
    r = cv.CreateImage(cv.GetSize(image), 8, 1)
    g = cv.CreateImage(cv.GetSize(image), 8, 1)
    b = cv.CreateImage(cv.GetSize(image), 8, 1)
    cv.Split(image, r, g, b, None)
    hsv = cv.CreateImage(cv.GetSize(image), 8, 3)
    cv.CvtColor(image, hsv, cv.CV_BGR2HSV)
    v = cv.CreateImage(cv.GetSize(image), 8, 1)
    s = cv.CreateImage(cv.GetSize(image), 8, 1)
    h = cv.CreateImage(cv.GetSize(image), 8, 1)    
    cv.Split(hsv, h, s, v, None)    
    canny = cv.CreateImage(cv.GetSize(image), 8, 1)
    cv.Zero(canny)
    
    t = 64
    cv.Canny(s, s, t, 54, 3)
    cv.Canny(v, v, t, 32, 3)
    cv.Canny(r, r, 200, 16, 3)
    cv.Canny(b, b, 200, 8, 3)
    cv.Or(r, b, canny)
    
    cv.ShowImage('s', s)
    cv.ShowImage('v', v)
    cv.ShowImage('r', r)
    cv.ShowImage('b', b)
    
    # find the contours of the edges and filter them based on can like properties
    storage = cv.CreateMemStorage(0)
    segs = cv.CreateImage(cv.GetSize(image), 8, 3)
    cv.Copy(image, segs)    
    contour = cv.FindContours(canny, storage, mode=cv.CV_RETR_CCOMP, method=cv.CV_CHAIN_APPROX_SIMPLE)
    cv.DrawContours(segs, contour, cv.CV_RGB(0, 255, 0), cv.CV_RGB(0, 128, 128), 1,thickness=2)
    while contour:
        bbox = cv.BoundingRect(list(contour))
        bbox2 = cv.MinAreaRect2(list(contour))
        contour = contour.h_next()

        pts = cv.BoxPoints(bbox2)
        ptslist = []
        for p in pts:
            ptslist.append((int(p[0]), int(p[1])))

        pt1 = (bbox[0], bbox[1])
        pt2 = (bbox[0] + bbox[2], bbox[1] + bbox[3])
        #cv.Rectangle(segs, pt1, pt2, cv.CV_RGB(255,255,255), 1)
        
        w = float(bbox[2])
        h = float(bbox[3])
        ar = h / w
        wt = 66.0
        ht = 121.0
        if w*h < 1000:
            # chuck small cans for now
            continue
        cv.PolyLine(segs, [ptslist], True, cv.CV_RGB(255,255,255))                    
        if (ar < 0.90*ht/wt or ar > 1.1*ht/wt) and \
           (ar < 0.90*wt/ht or ar > 1.1*wt/ht):
            continue
        cv.Rectangle(segs, pt1, pt2, cv.CV_RGB(0, 0, 255), 1)        
    return (canny, segs)
    
def adptseg(image):
    global bbox, contour
    
    dst = cv.CreateImage(cv.GetSize(image), 8, 1)
    #if image.channels > 1:
    if True:
        cv.CvtColor(image, dst, cv.CV_BGR2GRAY)
        r = cv.CreateImage(cv.GetSize(image), 8, 1)
        g = cv.CreateImage(cv.GetSize(image), 8, 1)
        b = cv.CreateImage(cv.GetSize(image), 8, 1)
        cv.Split(image, r, g, b, None)
        hsv = cv.CreateImage(cv.GetSize(image), 8, 3)
        cv.CvtColor(image, hsv, cv.CV_BGR2HSV)
        v = cv.CreateImage(cv.GetSize(image), 8, 1)
        s = cv.CreateImage(cv.GetSize(image), 8, 1)
        h = cv.CreateImage(cv.GetSize(image), 8, 1)    
        cv.Split(hsv, h, s, v, None)    
    else:
        cv.Copy(image, dst)
    
    #cv.Copy(s, dst)
    #cv.Smooth(dst, dst, cv.CV_BLUR, 3, 3)
    cv.AdaptiveThreshold(dst, dst, 255, cv.CV_ADAPTIVE_THRESH_MEAN_C, cv.CV_THRESH_BINARY_INV, 7, 15)
    cv.Dilate(dst, dst, iterations=1)
    cv.Erode(dst, dst, iterations=1)
    
    storage = cv.CreateMemStorage(0)
    segs = cv.CreateImage(cv.GetSize(image), 8, 3)
    cv.Copy(image, segs) 
    cdst = cv.CreateImage(cv.GetSize(dst), 8, 1)
    cv.Copy(dst, cdst)
    contour = cv.FindContours(cdst, storage, mode=cv.CV_RETR_CCOMP, method=cv.CV_CHAIN_APPROX_NONE)
    cv.DrawContours(segs, contour, cv.CV_RGB(0, 255, 0), cv.CV_RGB(0, 0, 255), 1,thickness=1)
    while contour:
        bbox = cv.BoundingRect(list(contour))
        contour = contour.h_next()
        w = float(bbox[2])
        h = float(bbox[3])
        if w*h < 1000:
            # chuck small cans for now
            continue
        pt1 = (bbox[0], bbox[1])
        pt2 = (bbox[0] + bbox[2], bbox[1] + bbox[3])        
        cv.Rectangle(segs, pt1, pt2, cv.CV_RGB(255, 0, 255), 3)            
    return (dst, segs)

def cleanup():
    global cam
    cv.DestroyAllWindows()
    if cam != None:
        del(cam)
    close('all')

if __name__ == "__main__":
    global cam

    # color space model of a can 
    rgb_can_model = [ [200, 256], [0, 128], [14, 120] ]
    hsv_can_model = [ [128, 256], [150, 255], [120, 240] ]
    
    # in r&d mode load files from disk
    rad = True
    clist = glob.glob('coke*.png')
    c = 0
    
    # start looking for cans, or capturing cans data
    cam = None
    if not rad:
        #ret = subprocess.call("./awb_off.sh", shell=True)
        cam = cv.CaptureFromCAM(-1)
        image = cv.QueryFrame(cam)
    while True:
        
        # aquire new image
        if rad:
            # in r&d mode we look at the disk images
            image = cv.LoadImage(clist[c], cv.CV_LOAD_IMAGE_COLOR)
            c += 1
            if c >= len(clist): break
        else:
            # if not r&d the run use real images
            image = cv.QueryFrame(cam)
        cv.ShowImage('cam', image)

        # adaptive segment
        (adpt, asegs) = adptseg(image)
        cv.ShowImage('adaptive', adpt)
        cv.ShowImage('asegments', asegs)
        
        # segment by edges
        #(canny, csegs) = cannyseg(image)
        #cv.ShowImage('canny', canny)
        #cv.ShowImage('csegments', csegs)
        
        # other
        if rad:
            key = cv.WaitKey(-1)
        else:
            key = cv.WaitKey(2)
        key &= 0xff
        if key == 27:
            break    
        if key == ord('h'):
            if r != None or g != None or b != None:
                fig = figure()
                axs = (fig.add_subplot(311), fig.add_subplot(312), fig.add_subplot(313))
                axs[0].set_title('red'); axs[1].set_title('green'); axs[2].set_title('blue')
                update_hist(r, g, b, axs)
        if key == ord('H'):
            if h != None or s != None or v != None:
                fig = figure()
                axs = (fig.add_subplot(311), fig.add_subplot(312), fig.add_subplot(313))
                axs[0].set_title('hue'); axs[1].set_title('sat'); axs[2].set_title('val')
                update_hist(h, s, v, axs)            
        if key == ord('s'):
            # get the next value
            coke_img_files = glob.glob('coke*.png')
            cannum = -1
            for f in coke_img_files:
                val = re.match('coke([0-9]+).png', f)
                val = int(val.groups()[0])
                if val > cannum:
                    cannum = val
            fname = str(cannum + 1)
            cv.SaveImage('./coke' + fname + '.png', image)
            print "written " + fname
    
    # done so clean up any mess    
    cleanup()
    print "Done"
