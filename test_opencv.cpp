#include <stdio.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <time.h>
#include <string>

using namespace cv;
using namespace std;

int main( int argc, char** argv )
{
	IplImage *input = 0;
	Mat frame;
	Mat foreground;
	Mat background;
	Mat frameGray;
	Mat runningAverage;
	Mat contoursMat;
	bool blurify = true;

	int recordingLimit = 5;
	bool recording = false;
	time_t timer;
	time_t diff;
	struct tm *time_ptr;

	FILE *outputFile = 0;
	string outputFileName;

	int frameNumber = 0;

	cvNamedWindow("video", 1);
	//cvNamedWindow("bounds",1);
	//cvNamedWindow("foreground",1);
	cvMoveWindow("video", 30, 0);
	//cvMoveWindow("bounds", 360, 0);
	//cvMoveWindow("foreground", 690, 0);

	const char* file = "stream_fifo";
	//const char* file = "test_video3.avi";

	CvCapture *capture = 0;
	capture = cvCaptureFromFile(file);
	if (!capture) {
		fprintf(stderr, "Can not open file %s\n", file);
		return -2;
	}

	for (;;) {
		input = cvQueryFrame(capture);
		frame = input;

		if (frame.empty()) {
			fprintf(stderr, "empty frame\n");
			break;
		}

		// camera is upside down, flip both axis
		flip(frame, frame, -1);
		frameNumber++;

		if(frameNumber == 1) {
			background = Mat(frame);
			frame.convertTo(runningAverage, CV_32F);
			foreground = Mat(frame);

		} else {
			runningAverage.convertTo(background, CV_8U);

			// diff the background and the frame, store results in the foreground
			absdiff(background, frame, foreground);

			// convert the foreground to gray and use a threshold to convert it to a binary image
			cvtColor(foreground, foreground, CV_BGR2GRAY);
			threshold(foreground, foreground, 80, 255.0, CV_THRESH_BINARY);

			// add the frame to the running average
			accumulateWeighted(frame, runningAverage, 0.005);

			vector<vector<Point> > contours;
			vector<Vec4i> hierarchy;

			// create a new Mat for storing the bounding rectangles in binary format
			contoursMat = Mat::zeros(frame.size(), CV_8UC3);

			// find all of the contours in the binary image
			findContours(foreground, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
			for (int i = 0; i < contours.size(); i++) {
				vector<Point> contour = contours.at(i);
				double size = contourArea(contour);

				// only use contours which have some size to them
				if (size > 200) {
					Rect bounds = boundingRect(Mat(contour));

					// only use contours within our specific viewing area
					if ((bounds.x > 60 || bounds.y > 200) && bounds.width > 27) {
						if (bounds.x < 200) {

							// draw the bounding rectangle in the original frame and the bounding rect binary image
							rectangle(contoursMat, bounds.tl(), bounds.br(), CV_RGB(255, 255, 255), -1, 8, 0);
							rectangle(frame, bounds.tl(), bounds.br(), CV_RGB(255, 50, 50), 1, 8, 0);

							timer = time(NULL);

							// start recording if we weren't
							if (!recording) {
								recording = true;
								diff = time(NULL);

								time_t tm = time(NULL);
								struct tm *tm_ptr = localtime(&tm);
								char time_stamp[15];
								strftime(time_stamp, 18, "%Y%m%d-%H%M%S", tm_ptr);
								outputFileName = string(time_stamp);

								string command = "ffmpeg -y -f image2pipe -vcodec mjpeg -r 3 -i - -vcodec mpeg4 -qscale 5 -r 3 captures/" + outputFileName + ".avi";
								printf("%s\n", command.c_str());
								outputFile = popen(command.c_str(), "w");
							}
						}
					}
				}
			}

			// desaturate the frame onto a gray Mat
			frame.copyTo(frameGray);
			cvtColor(frame, frameGray, CV_BGR2GRAY);
			cvtColor(frameGray, frameGray, CV_GRAY2BGR);
			if (blurify) {
				GaussianBlur(frameGray, frameGray, Size(5, 5), 0, 0);
			}

			// copy the frame onto the black and white image using the bounding rects Mat as a mask
			frame.copyTo(frameGray, contoursMat);

			// add the time stamp to the frame
			time_t tm = time(NULL);
			struct tm *tm_ptr = localtime(&tm);
			char time_stamp[18];
			strftime(time_stamp, 18, "%Y%m%d %H:%M:%S", tm_ptr);
			putText(frame, time_stamp, Point(157, 235), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(165, 165, 165), 1, 8);
			putText(frameGray, time_stamp, Point(157, 235), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(165, 165, 165), 1, 8);

			imshow("video", frameGray);
			//imshow("bounds", contoursMat);
			//imshow("foreground", foreground);

			// send the recorded image to ffmpeg
			if (recording) {
				vector<uchar> data_buffer;
				imencode(".jpg", frame, data_buffer);
				uchar* d = data_buffer.data();
				fwrite(d, 1, data_buffer.size() * sizeof(uchar), outputFile);
				fflush(outputFile);
				diff = time(NULL);
				printf("%.f second passed \n", difftime(diff, timer));
				//char buf[80];
				//strftime(buf, 80, "%Y%m%d%H%M%S", time_ptr);
				//printf("%s timer\n", buf);
				int time_diff = difftime(diff, timer);

				// trigger for stopping recording
				if (time_diff >= recordingLimit) {
					recording = false;
					printf("done recording!!\n");
					pclose(outputFile);
				}
			}

			int key = cvWaitKey(1);
			if (key >= 0) {
				if (key == 1048674) {
					blurify = !blurify;
				} else {
					printf("key pressed = %i\n", key);
					break;
				}
			}
		}
	}

	cvDestroyWindow("video");
	//cvDestroyWindow("background");
	//cvDestroyWindow("foreground");

	return 0;
}
