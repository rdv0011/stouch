import cv2
import numpy as np

def nothing(x):
    pass

cv2.namedWindow('mask')
cv2.namedWindow('info')

# create trackbars for color change
# A HSV range for a red color - 90, 100, 100 - 255, 255, 255
# one of the variants [ 90 ,  113 ,  81 ] - [ 255 ,  255 ,  255 ]
cv2.createTrackbar('H-Low','mask', 90, 255, nothing)
cv2.createTrackbar('H-High','mask', 255, 255, nothing)
cv2.createTrackbar('S-Low','mask', 113, 255, nothing)
cv2.createTrackbar('S-High','mask', 255, 255, nothing)
cv2.createTrackbar('L-Low','mask', 81, 255, nothing)
cv2.createTrackbar('L-High','mask', 255, 255, nothing)
filename = 'stouch-video-rgb-mapped.jpg'
img = cv2.imread(filename)
hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

infoImage = np.zeros([480,640,3])

while(1):
    h_low = cv2.getTrackbarPos('H-Low','mask')
    h_high = cv2.getTrackbarPos('H-High','mask')
    s_low = cv2.getTrackbarPos('S-Low','mask')
    s_high = cv2.getTrackbarPos('S-High','mask')
    l_low = cv2.getTrackbarPos('L-Low','mask')
    l_high = cv2.getTrackbarPos('L-High','mask')

    hsv_lower = np.array([h_low, s_low, l_low])
    hsv_upper = np.array([h_high, s_high, l_high])
    
    mask = cv2.inRange(hsv, hsv_lower, hsv_upper)
    
    lower_format = str((h_low,s_low,l_low))
    upper_format = str((h_high,s_high,s_high))
    hsv_range = ''.join(["[",lower_format," - ",upper_format,"]"])
    cv2.putText(infoImage, hsv_range, (20,50), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0,0,255) , 2)
    print hsv_range

    cv2.imshow("info", infoImage)
    cv2.imshow("mask", mask)
    
    #im2, contours, hierarchy  = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    #cnt = contours[0]
    #epsilon = 0.1*cv2.arcLength(cnt,True)
    #approx = cv2.approxPolyDP(cnt,epsilon,True)

    #x,y,w,h = cv2.boundingRect(cnt)
    #cv2.rectangle(cntImg,(x,y),(x+w,y+h),(0,255,0),2)
    #rect = cv2.minAreaRect(cnt)
    #box = cv2.boxPoints(rect)
    #box = np.int0(box)
    #cv2.drawContours(cntImg,[box],0,(0,0,255),2)
    #cv2.imshow("contour", cntImg)

    k = cv2.waitKey(1) & 0xFF
    if k == 27:
        break

cv2.destroyAllWindows()
