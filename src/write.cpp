#include "ShareMemory.hpp"
#include <unistd.h>
#include <opencv2/opencv.hpp>

using namespace cv;
int main()
{
    VideoCapture cap;
    cap.open(0);
    //Mat img(480, 640, CV_8UC3, Scalar(0, 255, 0));
    Mat img;
    //img= imread("/home/catmulti7/图片/cam1.png",IMREAD_ANYCOLOR );
    //int step = img.step;
    //cout<<"step is"<<step;

//    Mat imgsh(480, 640, CV_8UC3);
//    imgsh.data= img.data;
//    imshow("imgsh",imgsh);
 //   imshow("img",img);
//    waitKey(0);
    //cvtColor(img,img,COLOR_BGR2RGB);


    ShareMemWrt write(1);
    unsigned char *p;
    while (cap.isOpened())
    {
        cap>>img;
        resize(img,img,Size(640,480));
        p=write.requiredata();
        if(p!=nullptr)
        {
            memcpy(p,img.data,640*480*3);
            write.updateWrtLock();
        }
        //sleep(1);

    }

}