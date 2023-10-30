#pragma warning( disable : 4996 )
#include <iostream>
#include <fstream>
#include <vector>
#include <windows.h>
#include <direct.h>
#include <conio.h>
#include <ctime>
#include <thread>
#include <mutex>
#include <chrono>


//opencv
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/video/video.hpp>

// add by matthew
#include <opencv2/imgproc/types_c.h>
#include <opencv2/imgproc/imgproc_c.h>

//pylon
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/usb/BaslerUsbInstantCamera.h>
typedef Pylon::CBaslerUsbInstantCamera Camera_t;
//typedef Pylon::CBaslerUsbCamera Camera_t;
//#include "ConfigurationEventPrinter.h"
//#include "ImageEventPrinter.h"


using namespace std;
using namespace cv;
using namespace Pylon;
using namespace GenApi;
using namespace Basler_UsbCameraParams;
//using namespace Basler_UsbStreamParams;

int diffThreshold = 5;
int areaThreshold = 15;
int width = 640, height = 480;
int framerate = 500;
int exposureTime = 1000;
int recordTime = 1;
int pauseTime = 9;
int timeout = 50;
bool threadRun = false;
int saveOriginalImg = framerate;
int saveProcessedImg = framerate;
int algorithmType = 1;
bool isThreadOff = true;
bool toDetect = false;
bool toShow = true;
vector<Mat> vmats;
vector<string> fileNames;
//vector<string> resultNames;
//vector<Mat> resultImgDiffs;
//vector<Mat> resultImgBins;
vector<int> compressionParams; //initial in main function
mutex fmutex;

void grabThread();
//void algorithmType1Thread(int start, int end);
void cmdGetxy(int& x, int& y);
void cmdGotoxy(int x, int y);
void getTimeStr(string& str);
//int learn_MAT();

void saveConfig() {
    ofstream ofile("setting.config");
    ofile << "-areaThreshold " << areaThreshold << endl;
    ofile << "-diffThreshold " << diffThreshold << endl;
    ofile << "-framerate " << framerate << endl;
    ofile << "-exposureTime " << exposureTime << endl;
    ofile << "-recordTime " << recordTime << endl;
    ofile << "-pauseTime " << pauseTime << endl;
    ofile << "-timeout " << timeout << endl;
    ofile << "-saveOriginalImg " << saveOriginalImg << endl;
    ofile << "-saveProcessedImg " << saveProcessedImg << endl;
    ofile << "-algorithmType " << algorithmType << endl;
    ofile.close();
}

int main() {
    string flag, param;
    //init png compression params.
    compressionParams.push_back(IMWRITE_PNG_BILEVEL);
    compressionParams.push_back(1);
    compressionParams.push_back(IMWRITE_PNG_COMPRESSION);
    compressionParams.push_back(9);
      // https://blog.csdn.net/mail_cm/article/details/7765786
      // 如果函數成功，返回值包含文件或目錄的屬性。如果函數失敗，返回值是INVALID_FILE_ATTRIBUTES。
    DWORD ftyp = GetFileAttributesA(".//detect");
    if (ftyp == INVALID_FILE_ATTRIBUTES) _mkdir(".//detect");

    //read setting.config
    ifstream ifile("setting.config");

    if (!ifile) {
        saveConfig();
        /*cerr << "need to set config \"setting.config\" and run again." << endl;
        system("pause");
        return -1;*/
    }
    else {
        while (ifile >> flag >> param) {
            if (flag == "-areaThreshold") areaThreshold = stoi(param);
            else if (flag == "-diffThreshold") diffThreshold = stoi(param);
            else if (flag == "-framerate") framerate = stoi(param);
            else if (flag == "-exposureTime") exposureTime = stoi(param);
            else if (flag == "-recordTime") recordTime = stoi(param);
            else if (flag == "-pauseTime") pauseTime = stoi(param);
            else if (flag == "-timeout") timeout = stoi(param);
            else if (flag == "-saveOriginalImg") saveOriginalImg = stoi(param);
            else if (flag == "-saveProcessedImg") saveProcessedImg = stoi(param);
            else if (flag == "-algorithmType") algorithmType = stoi(param);
        }
    }

    ifile.close();
    int cmdx, cmdy, strnum;

    while (true) {
        system("cls");
        cout << endl << " Command List:" << endl
            << "\t (1)camera start" << endl
            << "\t (2)camera stop" << endl
            << "\t (3)detect on" << endl
            << "\t (4)detect off" << endl
            << "\t (5)show on" << endl
            << "\t (6)show off" << endl
            << "\t (7)set config" << endl
            << "\t (0)exit" << endl
            << endl;
        cout << "Enter the command :                     ";
        cmdGetxy(cmdx, cmdy);
        cmdGotoxy(cmdx - 20, cmdy);
        cin >> strnum;

        switch (strnum) {
        default:
            cout << "!!wrong cmd!!!                        " << endl;
            break;
        case 0:
            threadRun = false;
            while (!isThreadOff) Sleep(100);
            cout << endl;
            system("pause");
            return 0;
        case 1:
        {
            //cout << strnum << endl;
            if (threadRun) continue;
            isThreadOff = false;
            thread hThread(grabThread);
            hThread.detach();
            Sleep(1000);
            break;
        }
        case 2:
            threadRun = toDetect = false;
            while (!isThreadOff) Sleep(100);
            break;
        case 3:
            toDetect = true;
            break;
        case 4:
            toDetect = false;
            break;
        case 5:
            toShow = true;
            break;
        case 6:
            toShow = false;
            break;
        case 7:
        {
            if (threadRun) continue;
            int setcof, cofval;
            do {
                system("cls");
                cout << endl << " Config List:" << endl
                    << "\t (1)areaThreshold = " << areaThreshold << endl
                    << "\t (2)diffThreshold = " << diffThreshold << endl
                    << "\t (3)framerate = " << framerate << endl
                    << "\t (4)exposureTime = " << exposureTime << endl
                    << "\t (5)recordTime = " << recordTime << endl
                    << "\t (6)pauseTime = " << pauseTime << endl
                    << "\t (7)timeout = " << timeout << endl
                    << "\t (8)saveOriginalImg(N or 0) = " << saveOriginalImg << endl
                    << "\t (9)saveProcessedImg(N or 0) = " << saveProcessedImg << endl
                    << "\t (10)algorithmType(1 or 2) = " << algorithmType << endl
                    << "\t (0)return" << endl
                    << endl;
                cout << " Choose the parameter : ";
                cin >> setcof;
                cout << " Set the value : ";
                if (setcof != 0)cin >> cofval;
                switch (setcof) {
                default:
                    setcof = -1;
                    break;
                case 0:
                    saveConfig();
                    break;
                case 1:
                    areaThreshold = cofval;
                    break;
                case 3:
                    framerate = cofval;
                    if (saveOriginalImg > framerate) saveOriginalImg = framerate;
                    if (saveProcessedImg > framerate) saveProcessedImg = framerate;
                    break;
                case 2:
                    diffThreshold = cofval;
                    break;
                case 4:
                    exposureTime = cofval;
                    break;
                case 5:
                    recordTime = cofval;
                    break;
                case 6:
                    pauseTime = cofval;
                    break;
                case 7:
                    timeout = cofval;
                    break;
                case 8:
                    saveOriginalImg = cofval < framerate ? cofval : framerate;
                    break;
                case 9:
                    saveProcessedImg = cofval < framerate ? cofval : framerate;
                    break;
                case 10:
                    algorithmType = cofval;
                    break;
                }
            } while (setcof != 0);
            break;
        }
        case 8:
            //learn_MAT();
            break;
        }
    }

    cout << endl;
    system("pause");
    return 0;
}

void cmdGetxy(int& x, int& y) {
    //https://learn.microsoft.com/zh-tw/windows/console/console-screen-buffer-info-str
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    x = csbi.dwCursorPosition.X;
    y = csbi.dwCursorPosition.Y;
}

void cmdGotoxy(int x, int y) {
    COORD point;
    point.X = x, point.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), point);
}

void getTimeStr(string& str) {
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%Y%m%d_%I%M%S_", timeinfo);
    str = string(buffer);
}


void grabThread() {
    threadRun = true;
    Size framesize = Size(width, height);
    int cmdx, cmdy;
    string timestr;
    // Before using any pylon methods, the pylon runtime must be initialized.
    PylonInitialize();

    try {
        Camera_t camera(CTlFactory::GetInstance().CreateFirstDevice());
        //std::cout << "Using device : " << camera.GetDeviceInfo().GetModelName() << endl;
        camera.Open();
        camera.PixelFormat.SetValue(PixelFormat_Mono8);
        //https://zh.docs.baslerweb.com/image-roi
        camera.OffsetX.SetValue(16);
        camera.OffsetY.SetValue(16);
        camera.Width.SetValue(width);
        camera.Height.SetValue(height);
        //https://docs.baslerweb.com/acquisition-frame-rate
        camera.AcquisitionFrameRateEnable.SetValue(true);
        camera.AcquisitionFrameRate.SetValue(framerate);
        camera.ExposureTime.SetValue(exposureTime);
        const size_t ImageSize = (size_t)(camera.PayloadSize.GetValue());
        camera.MaxNumBuffer = framerate;
        int countOfImagesToGrab = framerate;
        int count;
        int grabi = 0;
        int i;
        int cyclei = 0;
        // This smart pointer will receive the grab result data.
        CGrabResultPtr grabResult;
        int total = framerate * recordTime;
        int showrate = framerate / 100;
        if (showrate == 0) showrate = 1;
        vmats.clear();
        fileNames.clear();

        for (i = 0; i < total; i++) {
            vmats.push_back(Mat(framesize, CV_8UC1, new uint8_t[ImageSize]));
            fileNames.push_back("");
        }

        while (threadRun && (cyclei < timeout || timeout <= 0)) {
            getTimeStr(timestr);
            // Start the grabbing of numbers of images.
            camera.StartGrabbing(countOfImagesToGrab);
            count = 1;

            while (camera.IsGrabbing()) {
                // Wait for an image and then retrieve it. A timeout of 3000 ms is used.
                camera.RetrieveResult(3000, grabResult, TimeoutHandling_ThrowException);

                if (grabResult->GrabSucceeded() && grabi < total) {
                    if (toShow && grabi % showrate == 0) DisplayImage(1, grabResult);
                    memcpy(vmats[grabi].data, grabResult->GetBuffer(), ImageSize);
                    //輸出格式為png by matthew
                    fileNames[grabi] = timestr + to_string(count) + ".png";
                    grabi++;
                    count++;
                }
            }

            //https://docs.baslerweb.com/resulting-frame-rate
            framerate = camera.ResultingFrameRate.GetValue();
            cmdGetxy(cmdx, cmdy);
            cmdGotoxy(0, cmdy + 1);
            cout << "FrameRate : " << framerate << " TotalFrame :" << grabi << "    " << endl;
            cmdGotoxy(cmdx, cmdy);

            if (countOfImagesToGrab < framerate * 0.9 || countOfImagesToGrab > framerate * 1.1) {
                grabi = 0;
                countOfImagesToGrab = framerate;
                total = framerate * recordTime;
                showrate = framerate / 100;
                if (showrate == 0) showrate = 1;

                if (vmats.size() > total) while (vmats.size() != total) vmats.pop_back();
                else if (vmats.size() < total) while (vmats.size() != total)
                    vmats.push_back(Mat(framesize, CV_8UC1, new uint8_t[ImageSize]));
            }

            if (grabi >= total) {
                if (toDetect) {
                    clock_t tic = clock();
                    string str, str2;

                    if (saveOriginalImg > 0)
                        for (i = 0; i < vmats.size(); i++) {
                            if (i % framerate >= saveOriginalImg) continue;
                            str = "detect\\o_" + fileNames[i];
                            imwrite(str, vmats[i]);
                        }

                    switch (algorithmType) {
                    case 1:
                    default:
                    {
                        /*for (i = 0; i < maxThreads; i++)
                            tds[i] = thread(algorithmType1Thread, i * processedSize, (i + 1) * processedSize);
                        for (i = 0; i < maxThreads; i++) tds[i].join();*/

                        Mat imDiff, im_th, withborder, preimg;
                        //https://blog.csdn.net/Ahuuua/article/details/80593388
                        vector<vector<Point> > contours;
                        vector<Vec4i> hierarchy;
                        //vector<string> imgnames;
                        //vector<Mat> results, imDiffs;
                        int idx, i, sum, savei = 0;
                        string str, str2;
                        const int maxval = 255;
                        preimg = vmats[0];


                        for (idx = 0; idx < vmats.size(); idx++) {
                            if (idx % framerate >= saveProcessedImg && savei >= saveProcessedImg) {
                                if (idx % framerate == 0) savei = 0;
                                preimg = vmats[idx];
                                continue;
                            }
                            //image process  by matthew
                            medianBlur(vmats[idx] - preimg, imDiff, 5);
                            preimg = vmats[idx];
                              //二值化  https://blog.csdn.net/u012566751/article/details/77046445
                            threshold(imDiff, im_th, diffThreshold, maxval, THRESH_TOZERO);
                            withborder = im_th.clone(); //Mat.clone()
                              //檢測邊緣  https://blog.csdn.net/dcrmg/article/details/51987348
                            findContours(im_th, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

                            if (!contours.empty() && !hierarchy.empty())
                                for (i = 0; i < contours.size(); i++)
                                      //輪廓繪製 https://blog.csdn.net/Easen_Yu/article/details/89380578
                                    drawContours(withborder, contours, i, Scalar::all(maxval), CV_FILLED, 8);

                            sum = cv::sum(withborder)[0];

                            if (sum > areaThreshold) {
                                savei++;

                                if (saveProcessedImg > 0) {
                                    str = "detect\\D_" + fileNames[idx];
                                    str2 = "detect\\R_" + fileNames[idx];
                                    imwrite(str, imDiff);
                                    imwrite(str2, withborder);
                                }
                                /*imgnames.push_back(fileNames[idx]);
                                imDiffs.push_back(imDiff);
                                results.push_back(withborder);*/
                            }
                        }

                        /*if (saveProcessedImg > 0)
                            for (i = 0; i < imgnames.size(); i++) {
                                str = "detect\\D_" + imgnames[i];
                                str2 = "detect\\R_" + imgnames[i];
                                imwrite(str, imDiffs[i]);
                                imwrite(str2, results[i], compressionParams);
                            }*/

                            //resultNames.clear();
                            //resultImgDiffs.clear();
                            //resultImgBins.clear();
                        break;
                    }
                    case 2:
                    {
                        Mat avgImg;
                        avgImg.create(framesize, CV_32FC1);
                        for (i = 0; i < vmats.size(); i++) cv::accumulate(vmats[i], avgImg);
                        avgImg /= vmats.size();
                        avgImg.convertTo(avgImg, CV_8U);
                        string str = "detect\\R_" + timestr + "_avg.png";
                        imwrite(str, avgImg);

                        if (saveProcessedImg > 0)
                            for (i = 0; i < vmats.size(); i++) {
                                if (i % framerate >= saveProcessedImg) continue;
                                str = "detect\\R_" + fileNames[i];
                                imwrite(str, avgImg - vmats[i]);
                            }

                        /*for (i = 0; i < maxThreads; i++)
                            tds[i] = thread(algorithmType2Thread, avgImg, i * processedSize, (i + 1) * processedSize);
*/
                        break;
                    }
                    }

                    clock_t toc = clock();
                    double sec = (double)(toc - tic) / CLOCKS_PER_SEC;
                    double needSleep;
                    cyclei += recordTime;

                    if (sec < pauseTime) {
                        needSleep = pauseTime - sec;
                        //Sleep(needSleep * 1000);
                        this_thread::sleep_for(chrono::milliseconds((long)needSleep * 1000));
                    }
                    else needSleep = 0;

                    cmdGetxy(cmdx, cmdy);
                    cmdGotoxy(0, cmdy + 2);
                    cout << "SpendTime : " << sec
                        << " pauseTime : " << needSleep
                        << " time : " << cyclei << "    " << endl;
                    cmdGotoxy(cmdx, cmdy);
                }

                grabi = 0;
            }
        }

        /*resultNames.clear();
        resultImgDiffs.clear();
        resultImgBins.clear();*/
        camera.Close();
        cmdGetxy(cmdx, cmdy);
        cmdGotoxy(0, cmdy + 1);
        std::cout << "Grab is stoped.                     " << endl;
        cmdGotoxy(cmdx, cmdy);
        isThreadOff = true;
        toDetect = false;
    }
    //異常處理 https://www.runoob.com/cplusplus/cpp-exceptions-handling.html
    catch (const GenericException& e) {
        // Error handling.
        cmdGetxy(cmdx, cmdy);
        cmdGotoxy(0, cmdy + 1);
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        cmdGotoxy(cmdx, cmdy);
        isThreadOff = true;
    }
}
//
//void algorithmType1Thread(int start, int end) {
//  Mat imDiff, im_th, withborder, preimg;
//  vector<vector<Point> > contours;
//  vector<Vec4i> hierarchy;
//  vector<string> imgnames;
//  vector<Mat> results, imDiffs;
//  int idx, i, sum;
//  string str, str2;
//  int length = end - start;
//  const int maxval = 255;
//  if (start == 0) preimg = vmats[0];
//  else preimg = vmats[start - 1];
//
//  for (idx = 0; idx < length; idx++) {
//      medianBlur(vmats[idx] - preimg, imDiff, 5);
//      preimg = vmats[idx];
//      threshold(imDiff, im_th, diffThreshold, maxval, THRESH_TOZERO);
//      withborder = im_th.clone();
//      findContours(im_th, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
//
//      if (!contours.empty() && !hierarchy.empty())
//          for (i = 0; i < contours.size(); i++)
//              drawContours(withborder, contours, i, Scalar::all(maxval), CV_FILLED, 8);
//
//      sum = cv::sum(withborder)[0];
//
//      if (sum > areaThreshold) {
//          imgnames.push_back(fileNames[idx]);
//          imDiffs.push_back(imDiff);
//          results.push_back(withborder);
//      }
//  }
//
//  lock_guard<mutex> writelock(fmutex);
//
//  for (idx = 0; idx < imgnames.size(); idx++) {
//      resultNames.push_back(imgnames[idx]);
//      resultImgDiffs.push_back(imDiffs[idx].clone());
//      resultImgBins.push_back(results[idx].clone());
//  }
//}


//#include <opencv2\opencv.hpp>
//#include <iostream>
//
//using namespace std;
//using namespace cv;
//
//int main()
//{
//  Mat img;
//  img = imread("car.jpg");
//  if (img.empty())
//  {
//      cout << "check" << endl;
//      return 0;
//  }
//  imshow("test", img);
//  waitKey(0);
//  return 0;
//}
