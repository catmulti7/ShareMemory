#ifndef _SHAREMEMORY_H
#define _SHAREMEMORY_H

#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include <sys/sem.h>
#include<atomic>
#include<iostream>

///#include <ros/ros.h>

const int infoLen=2;

using namespace std;

//enum LockStatus
//{
//    readed,
//    reading,
//    writing,
//    written
//};

int readed_=0;
int reading_=1;
int writing_=2;
int written_=3;

class ShareMemWrt
{
public:
    int shmid;//share memory id
    void* p;
    int imgNum; //number of data blocks
    int imgRows, imgCols; //used to calc size of data blocks
    int wrtIdx;
    atomic_int* nwrtIdx;
    atomic_int *pp;//数据区首地址，*pp==99表明已经成功获得共享内存
    atomic_int **lock;//图片状态标志位 0：已读 1：正在读取 2：正在写入 3：写入完毕待读取
    unsigned char *databegin;//数据区开始位置
    unsigned char **shmdata;//共享内存数据块

 
    ShareMemWrt(int shmkey);//写进程构造函数
    ~ShareMemWrt();

 
    unsigned char* requiredata();//内存要求函数，成功则返回可写内存块指针
    void updateWrtLock();//数据更新，等待被读程序读入



};

ShareMemWrt::ShareMemWrt(int shmkey)
{
    //get params from rosparam server
//    node.param("imgNum", imgNum, 8);
//    node.param("image_width", imgCols, 640);
//    node.param("image_height", imgRows, 480);

    imgNum=2;
    imgCols=640;
    imgRows=480;

    //计算一张图片所占内存大小
    //only for Mat which type is CV_8UC3(depth==1,channels==3)
    int dataLen=imgRows*imgCols*3;

    // 生成一个key
    key_t key = ftok("./../..", shmkey);
    // 创建共享内存，返回一个id
    // 第二个参数为共享内存大小，前面四个值分别记录共享内存循环队列的块大小，总块数，写指针索引,读指针索引与退出标志
    shmid = shmget(key, sizeof(atomic_int)*(infoLen+imgNum)+dataLen*imgNum, IPC_CREAT|0666);//0666是文件权限，不写只有超级用户才能打开
    if(-1 == shmid)
    {
        perror("shmget failed");
        return;
    }
    // 映射共享内存，得到虚拟地址
    p = shmat(shmid, 0, 0);
    if((void*)-1 == p)
    {
        perror("shmat failed");
        return;
    }
    shmdata=new unsigned char*[imgNum];//指针数据
    lock=new atomic_int*[imgNum];
    pp = (atomic_int*)p;// 共享内存对象映射

    //声明共享内存已经创建成功
    *pp=99;

    //记录写入位置
    nwrtIdx=pp+1;

    //每张图片的锁初始化为0
    for(int i=0;i<imgNum;i++)
    {
        lock[i]=pp+i+infoLen;
        *lock[i]=readed_;
    }
    
    databegin=(unsigned char*)(pp+infoLen+imgNum);
    for(int i=0;i<imgNum;i++)
    {
        shmdata[i]=databegin+i*(dataLen);
    }
}


void ShareMemWrt::updateWrtLock()
{
    //解锁
    *lock[wrtIdx] = written_;
    *nwrtIdx=wrtIdx;
}
 
// 内存要求函数，成功则返回可写内存块指针，失败返回Null
unsigned char* ShareMemWrt::requiredata()
{
    for(int i=0;i<imgNum;i++)
    {
        if(lock[i]->compare_exchange_strong(readed_, writing_))
        {
            wrtIdx=i;
            return shmdata[i];
        }
        else
        {
            readed_ = 0;
        }
    }
    for(int i=0;i<imgNum;i++)
    {
        if(lock[i]->compare_exchange_strong(written_, writing_))
        {
            wrtIdx=i;
            return shmdata[i];
        }
        else
        {
            written_ = 3;
        }
    }
    return nullptr;
}

ShareMemWrt::~ShareMemWrt()
{
    // 解除映射
    if(-1 == shmdt(p))
    {
        perror("shmdt failed");
        exit(3);
    }
    if(-1 == shmctl(shmid, IPC_RMID, NULL))
    {
        perror("shmctl failed");
        exit(4);
    }
    // 销毁开辟内存
    if(shmdata != nullptr)
    {
        delete []shmdata;
        shmdata = nullptr;
        for(int i=0;i<imgNum;i++)
        {
            delete lock[i];
            lock[i] = nullptr;
        }
        delete []lock;
        lock = nullptr;
    }

}



//读进程
class ShareMemRed
{
public:
    int shmid;//共享内存ID
    void *p;//共享内存首地址
    int imgNum;
    int imgRows, imgCols;
    int redIdx;
    atomic_int *pp;//数据区首地址
    atomic_int *nwrtIdx;
    atomic_int **lock;//图片状态标志位 0：已读 1：正在读取 2：已经读取 3：正在写入 4：写入完毕待读取
    unsigned char *databegin;//数据区开始位置
    unsigned char **shmdata;//共享内存数据块

 
    ShareMemRed(int shmkey);//写进程构造函数
    ~ShareMemRed();

 
    void writerelease();//写数据对象释放
    void readrelease();//读数据对象释放
 
    unsigned char* requiredata();//内存要求函数，成功则返回可写内存块指针
    
 
    unsigned char* getdataforread();//内存要求，成功则返回可读内存指针
    void updateRedLock();//数据更新，解锁

};

ShareMemRed::ShareMemRed(int shmkey)
{
    //get params from rosparam server
//    node.param("imgNum", imgNum, 8);
//    node.param("image_width", imgCols, 640);
//    node.param("image_height", imgRows, 480);
    imgNum=2;
    imgCols=640;
    imgRows=480;

    //计算一张图片所占内存大小
    //only for Mat which type is CV_8UC3(depth==1,channels==3)
    int dataLen=imgRows*imgCols*9;

    // 生成一个key
    key_t key = ftok("./../..", shmkey);
    // 创建共享内存，返回一个id
    // 第二个参数为共享内存大小，前面四个值分别记录共享内存循环队列的块大小，总块数，写指针索引,读指针索引与退出标志
    shmid = shmget(key, 0, 0);//0666是文件权限，不写只有超级用户才能打开
    if(-1 == shmid)
    {
        perror("shmget failed");
        return;
    }
    // 映射共享内存，得到虚拟地址
    p = shmat(shmid, 0, 0);
    if((void*)-1 == p)
    {
        perror("shmat failed");
        return;
    }
    shmdata=new unsigned char*[imgNum];//指针数据
    lock=new atomic_int*[imgNum];
    pp = (atomic_int*)p;// 共享内存对象映射
    nwrtIdx=pp+1;

    for(int i=0;i<imgNum;i++)
    {
        lock[i]=pp+i+infoLen;
        *lock[i]=0;
    }
    databegin=(unsigned char*)(pp+infoLen+imgNum);
    for(int i=0;i<imgNum;i++)
    {
        shmdata[i]=databegin+i*(dataLen);
    }
}

unsigned char* ShareMemRed::getdataforread()
{
    while(true)
    {
        if(lock[*nwrtIdx]->compare_exchange_strong(written_, reading_))
        {
            redIdx=*nwrtIdx;
            return shmdata[*nwrtIdx];
        }
        else
        {
            written_ = 3;
        }
    }
    return nullptr;
}

void ShareMemRed::updateRedLock()
{
    *lock[redIdx]=readed_;
}

ShareMemRed::~ShareMemRed()
{
    // 解除映射
    if(-1 == shmdt(p))
    {
        perror("shmdt failed");
        exit(3);
    }
    // 销毁开辟内存
    if(shmdata != nullptr)
    {
        delete []shmdata;
        shmdata = nullptr;
        for(int i=0;i<imgNum;i++)
        {
            delete lock[i];
            lock[i] = nullptr;
        }
        delete []lock;
        lock = nullptr;
    }
}

#endif
