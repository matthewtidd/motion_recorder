// compile with gcc -ggdb `pkg-config --cflags --libs opencv` opencvvideo_test.c -o opencvvideo_test
#include <stdio.h>
#include "highgui.h"
#include "cv.h"


int main(int argc, char** argv)
{
  IplImage *frame;
  IplImage *camera_image;
  IplImage *color_image;
  IplImage *display_image;
  IplImage *grey_image;
  CvMat *running_average_image;
  IplImage *running_average_in_display_color_depth;
  IplImage *difference;
  int      key;
  int counter = 0;

  /* supply the AVI file to play */
  //assert( argc == 2 );

  /* load the AVI file */
  CvCapture *capture = cvCreateFileCapture("stream_fifo") ;//cvCaptureFromAVI( argv[1] );

  /* always check */
  if (!capture) {
    return 1;
  }

  /* get fps, needed to set the delay */
  int fps = 5;

  int frameH = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
  int frameW = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);

  /* display video */
  cvNamedWindow( "video", CV_WINDOW_AUTOSIZE );

  while (key != 'q')
  {
    double t1=(double)cvGetTickCount();
    /* get a frame */
    camera_image = cvQueryFrame( capture );
    double t2=(double)cvGetTickCount();
    //printf("time: %gms  fps: %.2g\n",(t2-t1)/(cvGetTickFrequency()*1000.), 1000./((t2-t1)/(cvGetTickFrequency()*1000.)));

    /* always check */
    if (!frame) {
      break;
    }

    cvFlip(camera_image, 0, -1);

    display_image = cvCloneImage(camera_image);
    color_image = cvCloneImage(display_image);
    cvSmooth(color_image, color_image, CV_GAUSSIAN, 19, 0);

    if (counter == 0) {
      running_average_image = cvCreateMat(camera_image->height, camera_image->width, CV_32FC1);
      cvConvert(display_image, running_average_image);
    }

    //cvRunningAvg(color_image, running_average_image, 0.320, 0);
    //cvConvertScale(running_average_image, running_average_in_display_color_depth, 1.0f, 0.0f);
    //cvAbsDiff(color_image, running_average_in_display_color_depth, difference);
    //cvCvtColor(difference, grey_image, CV_RGB2GRAY);
    //cvThreshold(grey_image, grey_image, 2, 255, CV_THRESH_BINARY);
    //cvSmooth(grey_image, grey_image, CV_GAUSSIAN, 19, 0);
    //cvThreshold(grey_image, grey_image, 240, 255, CV_THRESH_BINARY);


    /* display frame */
    cvShowImage( "video", color_image );

    /* quit if user press 'q' */
    key = cvWaitKey( 1000 / fps );
    counter += 1;
  }

  /* free memory */
  cvReleaseCapture( &capture );
  cvDestroyWindow( "video" );

  return 0;
}
