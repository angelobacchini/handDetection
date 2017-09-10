# handDetection
Hand/gesture detection with OpenCV and Qt.

![alt text](https://github.com/adam-p/markdown-here/raw/master/src/common/images/icon48.png "./handProfile.png")

[link to Google!](http://google.com)

See full description at [angelobacchini.github.io] 
 (https://angelobacchini.github.io/software%20projects/hand-gesture-detection-with-qt-and-opencv)
---
### build
Builds with Qt 5.9.1 and OpenCV 3.3.0-2
Make sure openCV path in handDetection.pro is correct, then build with 

        qmake handProject.pro
        make

or just open the project in QtCreator.

## runtime slider settings
| Sliders        | Are           |
| ------------- |-------------|
| **mirror** | set to 1 to flip the image horizzontally |
| **channel** | output channel (HSV) selector |
| **output** | output modes selector - see list below |
| **blend** | blends the processed image with the original|
| **sigma** | sigma of the gaussian activation function |
| **huePicker** | sets hue level of the reference skin tone |
| **satPicker** | sets saturation level of the reference skin tone |
| **median**  | median filter kerlen size |
| **threshold** | binary thresholding level |

### ouptut modes
0. Original image (single channel, selected through channel slider)
1. Hue distance from the reference skin tone
2. Saturaion distance from the reference skin ton
3. Post gaussian activation and median filtering
4. Binary image
4. Distance transform
5. Distance transform (fingers region only)
6. Split screen

Enjoy
