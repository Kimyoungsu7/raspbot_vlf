
#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;
using namespace std; 

#define IMG_Width     640
#define IMG_Height    480

#define USE_DEBUG  0   // 1 Debug  사용
#define USE_CAMERA 0   // 1 CAMERA 사용  0 CAMERA 미사용

#define ROI_CENTER_Y  100
#define ROI_WIDTH     50

Mat Region_of_Interest(Mat image, Point *points)
{
  Mat img_mask =Mat::zeros(image.rows,image.cols,CV_8UC1);	 
  
  Scalar mask_color = Scalar(255,255,255);
  const Point* pt[1]={ points };	    
  int npt[] = { 4 };
  fillPoly(img_mask,pt,npt,1,Scalar(255,255,255),LINE_8);
  Mat masked_img;	
  bitwise_and(image,img_mask,masked_img);
  
  return masked_img;
}

Mat Region_of_Interest_crop(Mat image, Point *points)
{
   Mat img_roi_crop;	

   Rect bounds(0,0,image.cols,image.rows);	 
   Rect r(points[0].x,points[0].y,image.cols, points[2].y-points[0].y);  
   //printf("%d %d %d %d\n",points[0].x,points[0].y,points[2].x, points[2].y-points[0].y);
   //printf("%d  %d\n", image.cols, points[2].y-points[0].y);

   img_roi_crop = image(r & bounds);
   
   return img_roi_crop;
}


int main(void)
{
    /////////////////////////////////   영상 변수 선언  ////////////////////////////////////
    int img_width, img_height;
  
    Mat mat_image_org_color(IMG_Height,IMG_Width,CV_8UC3);
    Mat mat_image_org_gray;
    Mat mat_image_roi;
    
    Point points[4];
    
    img_width  = 640;
    img_height = 480;
	
	VideoCapture cap(0);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, img_width);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, img_height);
	
	
	if(!cap.isOpened())
	{
		cerr <<"Error , 카메라를 열 수 없습니다. \n";
		mat_image_org_color = imread("./img/line_1.jpg", IMREAD_COLOR); 
		img_height = mat_image_org_color.rows;
	    img_width  = mat_image_org_color.cols;
		//return -1;  
	}
	else
	{
		 printf("카메라가 잘 작동 됩니다.\n"); 
		 cap.read(mat_image_org_color);
	}
	
	
	if(USE_CAMERA == 0)  mat_image_org_color = imread("./img/line_1.jpg", IMREAD_COLOR); 
     
    
    if(mat_image_org_color.empty())
    {
       cerr << "image file error!";
    }
	
    Scalar GREEN(0,255,0);
    Scalar RED(0,0,255);
    Scalar BLUE(255,0,0);
    Scalar YELLOW(0,255,255);
    //////////////////////////////////////////////////////////////////////////////////////

    	
    printf("Image size[%3d,%3d]\n", img_width,img_height);
    
    namedWindow("Display Window", cv::WINDOW_NORMAL);
    resizeWindow("Display Window", img_width,img_height);
    moveWindow("Display Window", 10, 10);
    
    namedWindow("Gray Image Window", cv::WINDOW_NORMAL);
    resizeWindow("Gray Image Window", img_width,img_height);
    moveWindow("Gray Image Window", 700, 10);
    
    namedWindow("Gray ROI Image Window", cv::WINDOW_NORMAL);
    //resizeWindow("Gray ROI Image Window", img_width,img_height);
    moveWindow("Gray ROI Image Window", 10, 500);
   
    points[0] =  Point(0,ROI_CENTER_Y-ROI_WIDTH);
	points[1] =  Point(0,ROI_CENTER_Y+ROI_WIDTH);
	points[2] =  Point(640,ROI_CENTER_Y+ROI_WIDTH);
	points[3] =  Point(640,ROI_CENTER_Y-ROI_WIDTH);
	
	
    while(1)
    {
      
      if(USE_CAMERA == 1)  cap.read(mat_image_org_color);
      
      cvtColor(mat_image_org_color, mat_image_org_gray, CV_RGB2GRAY);        // color to gray conversion  
      mat_image_roi = Region_of_Interest_crop(mat_image_org_gray,points);    // ROI 영역을 추출함      
            
      
      imshow("Display Window", mat_image_org_color);
      imshow("Gray Image Window", mat_image_org_gray);
      imshow("Gray ROI Image Window",mat_image_roi);
      // ESC 키를 입력하면 루프가 종료됩니다.   
      if (waitKey(25) >= 0)
      {
            break;
      }
     }		             	
    
    if(USE_CAMERA == 1)  cap.release();    
    destroyAllWindows();

  
   return 0;	
}
