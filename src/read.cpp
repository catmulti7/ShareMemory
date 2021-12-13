#include"ShareMemory.hpp"
#include <opencv2/opencv.hpp>

using namespace cv;
int main()
{
    Mat img(480, 640, CV_8UC3);
    ShareMemRed read(1);
    unsigned char *p;
    while(true)
    {
        p=read.getdataforread();
        //memcpy(img.data,p,640*480*3);
        img.data=p;
        imshow("read",img);
        waitKey(1);
        read.updateRedLock();

    }
}