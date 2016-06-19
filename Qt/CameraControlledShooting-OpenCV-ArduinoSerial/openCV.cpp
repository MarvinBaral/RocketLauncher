#include "openCV.hpp"


OpenCV::OpenCV(ServoControl *pServoControl) {
    std::cout << "BGR color model!!!" << std::endl;

    servoControl = pServoControl;

    paramCam[EXTERNAL][MINIMUM_CTR] = 350;
    paramCam[EXTERNAL][ANGLE_OF_VIEW_X] = 2 * 30;
    paramCam[EXTERNAL][ANGLE_OF_VIEW_Y] = 2 * 20;
    paramCam[EXTERNAL][WIDTH] = 640;
    paramCam[EXTERNAL][HEIGHT] = 480;

    paramCam[INTERNAL][MINIMUM_CTR] = 100;
    paramCam[INTERNAL][ANGLE_OF_VIEW_X] = 2 * 30;
    paramCam[INTERNAL][ANGLE_OF_VIEW_Y] = 2 * 20;
    paramCam[INTERNAL][WIDTH] = 640;
    paramCam[INTERNAL][HEIGHT] = 480;

    usedCam = EXTERNAL;
    invertXAxis = true;
    minimumRelationTrigger = 0.5;
    minimumAbsoluteRedValue = 200;
    interestingColor = 2;
    repeationsUntilShot = 30;
    distanceBetweenCamAndCannon = 0.1; //m
    realSize = 0.25; //m

    pixelMarkColor[0] = 255;
    pixelMarkColor[1] = 0;
    pixelMarkColor[2] = 0;
    positionMarkColor[0] = 0;
    positionMarkColor[1] = 255;
    positionMarkColor[2] = 0;

    cap = new cv::VideoCapture(usedCam);
    if (!cap->isOpened()) {
        std::cout << "Cannot open the video cam" << std::endl;
    }

    displayWindow = true;
    windowTitle = "Abschusskamera";
    if (displayWindow) {
        cv::namedWindow(windowTitle, CV_WINDOW_AUTOSIZE);
    }
}

int OpenCV::getByte(cv::Mat frame, int x, int y, int byte) {
    return *(frame.data + frame.step[0] * y + frame.step[1] * x + byte); //http://docs.opencv.org/2.4/modules/core/doc/basic_structures.html#mat, BGR color model!
}

void OpenCV::writeByte(cv::Mat frame, int x, int y, int byte, int value) {
    *(frame.data + frame.step[0] * y + frame.step[1] * x + byte) = value;
}

float OpenCV::getRelation(cv::Mat frame, int x, int y, int byte) {
    float sum = (getByte(frame, x, y, 0) + getByte(frame, x, y, 1) + getByte(frame, x, y, 2));
    float single = getByte(frame, x, y, byte);
    if (sum == 0) {
        sum = 1;
    }
    return single/sum;
}

short OpenCV::getHighestColor(cv::Mat frame, int x, int y) {
    signed short winner = -1;
    short highestValue = 0;
    short value = 0;
    for (short i = 0; i < 3; i++) {
        value = getByte(frame, x, y, i);
        if (value > highestValue) {
            highestValue = value;
            winner = i;
        }
    }
    return winner;
}

short OpenCV::getAverage(cv::Mat frame, int x, int y) {
    int sum = (getByte(frame, x, y, 0) + getByte(frame, x, y, 1) + getByte(frame, x, y, 2));
    return sum / 3.f;
}

void OpenCV::markPixel(cv::Mat frame, int posx, int posy) {
    for (int i = 0; i < 3; i++) {
        writeByte(frame, posx, posy, i, pixelMarkColor[i]);
    }
}

void OpenCV::showColorOfCenteredPixel() {
    int baseColor[3];
    for (int color = 0; color < 3; color++){
        baseColor[color] = *(frame.data + frame.step[0] * (frame.rows / 2) + frame.step[1] * (frame.cols / 2) + color);
        std::cout << baseColor[color] << "|";
    }
    std::cout << std::endl << "y: " << frame.rows / 2 << " x: " << frame.cols / 2 << std::endl;
}

void OpenCV::markPosition(int posx, int posy) {
    int size = 5;
    for (int y = posy - size; y < posy + size; y++) {
        for (int x = posx - size; x < posx + size; x++){
            for (int i = 0; i < 3; i++) {
                if (x > 0 && y > 0){
                    writeByte(frame, x, y, i, positionMarkColor[i]);
                }
            }
        }
    }
}

float OpenCV::calcDistance(std::vector<int> point1, std::vector<int> point2){
    float distance = std::sqrt((point1[0] - point2[0]) * (point1[0] - point2[0]) + (point1[1] - point2[1]) * (point1[1] - point2[1]));
    return distance;
}

void OpenCV::detectBallByAverage() {
    std::vector<std::vector<int>> pixels;
    int ctr = 0, ypos = 0, xpos = 0;
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols; x++) {
            if (getRelation(frame, x, y, interestingColor) >= minimumRelationTrigger  && getByte(frame, x, y, interestingColor) >= minimumAbsoluteRedValue && getByte(frame, x, y, 0) + getByte(frame, x, y, 1) + getByte(frame, x, y, 2) >= minimumAbsoluteRedValue) {
                pixels.push_back({x, y});
                ctr++;
                ypos += y;
                xpos += x;
                markPixel(frame, x, y);
            }
        }
    }
    //get position
    if (ctr == 0) {
        ctr = 1;
    }
    ypos /= ctr;
    xpos /= ctr;

    //get size
    int width = 0;
    int height = 0;
    int sizeLocal = 0;
    int extremes[2][2] = {{paramCam[usedCam][WIDTH], 0},{paramCam[usedCam][HEIGHT], 0}}; //x,y min,max

    for (unsigned int i = 0; i < pixels.size(); i++) {
        for (int xy = 0; xy < 2; xy++) {
            if (pixels[i][xy] < extremes[xy][0]) {
                extremes[xy][0] = pixels[i][xy];
            }
            if (pixels[i][xy] > extremes[xy][1]) {
                extremes[xy][1] = pixels[i][xy];
            }
        }
    }
    width = extremes[0][1] - extremes[0][0];
    height = extremes[1][1] - extremes[1][0];
    markPosition(extremes[0][0], extremes[1][0]);
    markPosition(extremes[0][0], extremes[1][1]);
    markPosition(extremes[0][1], extremes[1][0]);
    markPosition(extremes[0][1], extremes[1][1]);

    sizeLocal = std::round((width + height) * 0.5);
    size = sizeLocal;
    for (unsigned int i = 0; i < contacts.size(); i++) {
        size = std::min(size, contacts[i][0]);
//        std::cout << "compared" << std::endl;
    }
    std::cout << "size: " << size << std::endl;

    //get distance
    const float PI = 3.14159265359;
    float distance = 0;
    float coordY;
    if (size > 0) {
        float alpha = (paramCam[usedCam][ANGLE_OF_VIEW_Y] / (paramCam[usedCam][HEIGHT] * 1.f)) * size; //ganzzahldivision
        std::cout << "angle: " << alpha << std::endl;
        alpha =  alpha / 180.f * PI;  //conversion from degree to radiant
        distance = (realSize / 2.f) / (std::tan(alpha / 2.f));
        distance -= distanceBetweenCamAndCannon;

        std::cout << "distance: " << distance << std::endl;
    //get Height
        float angleY;
        angleY = paramCam[usedCam][ANGLE_OF_VIEW_Y] / (paramCam[usedCam][HEIGHT] * 1.f) * ((paramCam[usedCam][HEIGHT] - ypos) - paramCam[usedCam][HEIGHT] / 2.f);
        angleY = angleY / 180.f * PI;
        coordY = std::sin(angleY);

        std::cout << "height: " << coordY << std::endl;
    }

    //get position and calc shooting angles
    float degrees[2] = {0, 0};


    if (xpos > 0 && ypos > 0) {
        std::cout << "Position x: " << xpos << " y: " << ypos << " ctr: " << ctr << std::endl;
        this->markPosition(xpos, ypos);
        if (ctr > paramCam[usedCam][MINIMUM_CTR]) {
            contacts.push_back({sizeLocal, 0});
            int xysize[2] = {paramCam[usedCam][WIDTH], paramCam[usedCam][HEIGHT]};
            int xypos[2] = {xpos, ypos};
            for (int i = 0; i < 2; i ++) {
                float degree = paramCam[usedCam][ANGLE_OF_VIEW_X + i] * 0.5 - ((xypos[i] * (1.0f / xysize[i])) * paramCam[usedCam][ANGLE_OF_VIEW_X + i]);
                if (i == 1) {
                    //degree += 20;
                }
                if (i == 0) {
                    if (invertXAxis) {
                        degree = -degree;
                    }
                }
                //std::cout << "degree: " << degree << std::endl;
                degrees[i] = degree;
            }
            float v0 = 3;
            float a = 0;
            float g = 9.81;
            float y0 = 0;
            float x = distance, y;
            for (int i = 0; i < 80; i++) {
                a = i;
                a = a / 180.f * PI; //convert to radiant
                float t = (x / v0 * std::cos(a));
                y = y0 + v0 * std::sin(a) * t - 0.5 * g * t * t;

                if (y >= coordY) {
                    degrees[1] = i;
                    break;
                }
            }
            degrees[0] += std::pow(1.07, degrees[1] - 18) + 8; //regression: y=1.07^(x-18)+8, more: https://docs.google.com/spreadsheets/d/1m2OmglEK80_FfIZ42FL04EmCf1KAKzufZCY5AwhhgKE/edit?usp=sharing
            for (int i = 0; i < 2; i ++) {
                servoControl->setServo(i, degrees[i]);
            }
            if (contacts.size() >= repeationsUntilShot) {
                servoControl->shoot();
                contacts.clear();
            }
        } else {
            contacts.clear();
        }
    }
    //    std::cout << "contineous contacts: " << contacts.size() << std::endl;
    pixels.clear();
}

int OpenCV::moveWhileSameColor(int starty, int startx, int directiony, int directionx) {
    int length = 0;
    int posy = starty, posx = startx;
    bool colorOK = true;
    while (colorOK) {
        posy += directiony;
        posx += directionx;
        length++; //will not work in 45° angles
        for (int i = 0; i < 3; i++) {
            if (getRelation(frame, posx, posy, 2) < minimumRelationTrigger) {
                colorOK = false;
            }
        }
    }
    return length; //minimum return value is 1
}

void OpenCV::detectBallWithLines() {
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols; x++) {
            if (getRelation(frame, x, y, 2) < minimumRelationTrigger) {
                int initHeight = this->moveWhileSameColor(y, x, 1, 0); //go from first point down
                int iniWidthRight = this->moveWhileSameColor(y + initHeight * 0.5, x, 0, 1); //go at half right
                int iniWidthLeft = this->moveWhileSameColor(y + initHeight * 0.5, x, 0, -1); //go at half left
                int iniWidth = iniWidthLeft + iniWidthRight;
                if (initHeight > 50 && iniWidth > 50) {
                    std::cout << "pos x: " << x << " y: " << y + initHeight * 0.5 << std::endl;
                    this->markPosition(x, y + initHeight * 0.5);
                    return;
                }
            }
        }
    }
}

void OpenCV::showAvgBGR() {
    long int avgRGB[3] = {0, 0, 0}; //2^32 is far greater than e.g. 640 * 480 * 255
    for (int y = 0; y < frame.rows; y += 1) {
        for (int x = 0; x < frame.cols; x += 1) {
            for (int color = 0; color < 3; color++){
                avgRGB[color] += *(frame.data + frame.step[0] * y + frame.step[1] * x + color); //http://docs.opencv.org/2.4/modules/core/doc/basic_structures.html#mat; the 4th byte ist the first byte of the next image
            }
        }
    }
    float divisor = frame.rows * frame.cols; //640 * 480;
    std::cout << "Average BGR-values: ";
    for (int ctr = 0; ctr < 3; ctr++) {
        avgRGB[ctr] /= divisor;
        std::cout << avgRGB[ctr] << "|";
    }
    std::cout << std::endl;
}

void OpenCV::printApropriateSign(int value) {
    char sign = 'Y';
    if (value > 255) {
        sign = '#';
    } else if (value > 200) {
        sign = 'X';
    } else if (value > 150) {
        sign = 'x';
    } else if (value > 100) {
        sign = 'O';
    } else if (value > 50) {
        sign = 'o';
    } else if (value > 5) {
        sign = '-';
    } else {
        sign = '.';
    }
    std::cout << sign << "|";
}

void OpenCV::showOnCMD() {
    int cols = 13;
    int rows = 30;
    int ysteps = frame.rows/rows;
    int xsteps = frame.cols/cols;
    std::cout << "Frame: " << cols << " x " << rows << " pixel" << std::endl;
    for (int y = 0; y < frame.rows; y += ysteps) {
        for (int x = 0; x < frame.cols; x += xsteps) {
            std::cout << "|";
            for (int color = 0; color < 3; color++){
                this->printApropriateSign(*(frame.data + frame.step[0] * y + frame.step[1] * x + color)); //http://docs.opencv.org/2.4/modules/core/doc/basic_structures.html#mat; the 4th byte ist the first byte of the next image
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void OpenCV::printParams(cv::VideoCapture cap) {
    std::vector<double> propId =
    {
        CV_CAP_PROP_POS_MSEC,
        CV_CAP_PROP_POS_FRAMES,
        CV_CAP_PROP_POS_AVI_RATIO,
        CV_CAP_PROP_FRAME_WIDTH,
        CV_CAP_PROP_FRAME_HEIGHT,
        CV_CAP_PROP_FPS,
        CV_CAP_PROP_FOURCC,
        CV_CAP_PROP_FRAME_COUNT,
        CV_CAP_PROP_FORMAT,
        CV_CAP_PROP_MODE,
        CV_CAP_PROP_BRIGHTNESS,
        CV_CAP_PROP_CONTRAST,
        CV_CAP_PROP_SATURATION,
        CV_CAP_PROP_HUE,
        CV_CAP_PROP_GAIN,
        CV_CAP_PROP_EXPOSURE,
        CV_CAP_PROP_CONVERT_RGB,
        CV_CAP_PROP_WHITE_BALANCE_BLUE_U,
        CV_CAP_PROP_WHITE_BALANCE_RED_V,
        CV_CAP_PROP_RECTIFICATION,
        CV_CAP_PROP_ISO_SPEED,
        CV_CAP_PROP_BUFFERSIZE
    };
    for (unsigned int i = 0; i < propId.size(); i++) {
        std::cout << i << "(" << propId.at(i) << "): " << std::to_string(cap.get(propId.at(i))) << std::endl;
    }
}

/*
    CV_CAP_PROP_POS_MSEC Current position of the video file in milliseconds or video capture timestamp.
    CV_CAP_PROP_POS_FRAMES 0-based index of the frame to be decoded/captured next.
    CV_CAP_PROP_POS_AVI_RATIO Relative position of the video file: 0 - start of the film, 1 - end of the film.
    CV_CAP_PROP_FRAME_WIDTH Width of the frames in the video stream.
    CV_CAP_PROP_FRAME_HEIGHT Height of the frames in the video stream.
    CV_CAP_PROP_FPS Frame rate.
    CV_CAP_PROP_FOURCC 4-character code of codec.
    CV_CAP_PROP_FRAME_COUNT Number of frames in the video file.
    CV_CAP_PROP_FORMAT Format of the Mat objects returned by retrieve() .
    CV_CAP_PROP_MODE Backend-specific value indicating the current capture mode.
    CV_CAP_PROP_BRIGHTNESS Brightness of the image (only for cameras).
    CV_CAP_PROP_CONTRAST Contrast of the image (only for cameras).
    CV_CAP_PROP_SATURATION Saturation of the image (only for cameras).
    CV_CAP_PROP_HUE Hue of the image (only for cameras).
    CV_CAP_PROP_GAIN Gain of the image (only for cameras).
    CV_CAP_PROP_EXPOSURE Exposure (only for cameras).
    CV_CAP_PROP_CONVERT_RGB Boolean flags indicating whether images should be converted to RGB.
    CV_CAP_PROP_WHITE_BALANCE_U The U value of the whitebalance setting (note: only supported by DC1394 v 2.x backend currently)
    CV_CAP_PROP_WHITE_BALANCE_V The V value of the whitebalance setting (note: only supported by DC1394 v 2.x backend currently)
    CV_CAP_PROP_RECTIFICATION Rectification flag for stereo cameras (note: only supported by DC1394 v 2.x backend currently)
    CV_CAP_PROP_ISO_SPEED The ISO speed of the camera (note: only supported by DC1394 v 2.x backend currently)
    CV_CAP_PROP_BUFFERSIZE Amount of frames stored in internal buffer memory (note: only supported by DC1394 v 2.x backend currently)
*/

OpenCV::~OpenCV() {
    delete cap;
}
