#include <QSerialPort>
#include <QTime>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "servoControl.h"
#include "cameraControl.cpp"
#include <queue>

const unsigned short int STEP_DEGREE = 5;
const bool SHOW_RESPONSE_FROM_ARDUINO = false;
const QString PORT_NAME = "/dev/ttyACM0";
const bool SHOW_FPS = true;
bool automaticMode = true;
long unsigned int frameCount = 0;
unsigned int fpsCount = 0;
QTime startTime;
int keyPressed;
bool displayWindow = true;
std::string windowTitle = "Abschusskamera";
const short USB_CAM = 1;	//0 = first connected USB Cam on boot

int main() {

	cv::VideoCapture* capture = new cv::VideoCapture(USB_CAM);
	if (!capture->isOpened()) {
		std::cout << "Cannot open the video cam. Please connect the USB-Cam!" << std::endl;
	}
	if (displayWindow) {
		cv::namedWindow(windowTitle, CV_WINDOW_AUTOSIZE);
	}
    QSerialPort* serial = new QSerialPort();
    ServoControl* servoControl = new ServoControl(serial);
	CameraControl* cameraControl = new CameraControl(servoControl, capture, windowTitle);

    servoControl->initSerial(PORT_NAME);
    serial->open(QIODevice::ReadWrite);

    startTime = QTime::currentTime();
	do {
		cameraControl->readFrame();
		frameCount++;
        fpsCount++;

		if (automaticMode) {
			cameraControl->detectBallByAverage();
		}
		if (displayWindow) {
			cameraControl->showFrame();
		}
        keyPressed = cv::waitKey(1);
        switch (keyPressed) {
        case -1: break;
		case 97: //a = automatic mode
			automaticMode = true;
			break;
		case 109: //m = manual mode
			automaticMode = false;
				break;
		case 99: //c = clear
			cameraControl->allowedToShoot = true;
            break;
        case 107: //k
			cameraControl->showColorOfCenteredPixel();
            break;
        case 108: //l = lock
			cameraControl->allowedToShoot = false;
            break;
		case 81: //left
			if (!automaticMode)
				servoControl->updateServo(0, -STEP_DEGREE);
            break;
		case 83: //right
			if (!automaticMode)
				servoControl->updateServo(0, STEP_DEGREE);
            break;
		case 82: //up
			if (!automaticMode)
				servoControl->updateServo(1, -STEP_DEGREE);
            break;
		case 84: //down
			if (!automaticMode)
				servoControl->updateServo(1, STEP_DEGREE);
            break;
        case 10: //enter = shoot
            servoControl->shoot();
            break;
        case 114: //r = reset
            serial->close();
            serial->open(QIODevice::ReadWrite);
            break;
        default:
#ifdef DEBUG
			std::cout << "pressed " << keyPressed << std::endl;
#endif
			break;
        }

        if (SHOW_RESPONSE_FROM_ARDUINO) {
			serial->waitForReadyRead(10);
            QByteArray response = serial->readAll();
            if (!response.isEmpty() && !response.isNull()) {
                std::cout << response.toStdString();
            }
        }

		if (SHOW_FPS && startTime <= QTime::currentTime().addSecs(-1)) {
            startTime = QTime::currentTime();
            std::cout << "fps:" << fpsCount << std::endl;
            fpsCount = 0;
        }
    } while (keyPressed != 27);

    serial->close();
    std::cout << "esc key pressed - aborted" << std::endl;

	delete cameraControl;
    delete servoControl;
    delete serial;

    return 0;
}
