//#include "framework/uimanager.h"
#include "../framework/document.h"
//#include "glrender.h"
//#include "../framework/util.h"
//#include "../framework/ImageBoard.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QApplication>
#include <QResource>
#include <QTranslator>
#include <QIcon>
#include <iostream>
#include <QJsonArray>

#include <opencv2/opencv.hpp>

void testCVAlg(){
    auto img = cv::imread("F:/ttt/Hearthstone Screenshot 03-30-20 20.57.10.png");
    cv::cvtColor(img, img, cv::COLOR_RGB2GRAY);
    //normalize(img, img, 0, 1, cv::NORM_MINMAX);
    //threshold(img, img, 85, 255, cv::THRESH_BINARY);
    //cv::erode(img, img, )
    //img = 255 - img;
    //cv::imshow("hello1", img);
    //cv::Mat img2;
    //equalizeHist(img, img);
    //cv::imshow("hello2", img2);
    //threshold(img2, img2, 120, 255, cv::THRESH_BINARY);
    auto img2 = img(cv::Rect(696, 474, 130, 180)).clone();
   // cv::rectangle(img2, cv::Point(16, 20), cv::Point(36, 40), cv::Scalar(0, 0, 0));
   // cv::rectangle(img2, cv::Point(100, 20), cv::Point(120, 40), cv::Scalar(0, 0, 0));
   // cv::rectangle(img2, cv::Point(55, 150), cv::Point(75, 170), cv::Scalar(0, 0, 0));
    /*int ch[] = {0};
    int histsize[] = {256};
    float range[] = { 0, 255 };
    const float* fHistRanges[] = {range};
    cv::Mat hist;
    cv::calcHist(&img2, 1, ch, cv::Mat(), hist, 1, histsize, fHistRanges);
    QJsonArray ret;
    for (int i = 0; i < hist.rows; ++i)
        ret.push_back(hist.at<float>(i));*/


    cv::Mat mask0 = img2(cv::Rect(16, 20, 20, 20)),
            mask1 = img2(cv::Rect(100, 20, 20, 20)),
            mask2 = img2(cv::Rect(55, 150, 20, 20));
    cv::Mat mask(img2.rows, img2.cols, CV_8UC1, cv::Scalar(0, 0, 0));
    mask0.clone().copyTo(mask(cv::Rect(16, 20, 20, 20)));
    mask1.clone().copyTo(mask(cv::Rect(100, 20, 20, 20)));
    mask2.clone().copyTo(mask(cv::Rect(55, 150, 20, 20)));
    //cv::namedWindow("hello2",0);
    //cv::moveWindow("hello2", 300, 0);

    //cv::Mat maskn;
    //threshold(img2, maskn, 250, 255, cv::THRESH_TOZERO_INV);
    //img2 -= maskn;
    cv::imshow("hello", img2);

    cv::imshow("hello2", mask);

    cv::Mat got0(180, 130, CV_8UC1, cv::Scalar(255, 255, 255));
//    got0.copyTo(img(cv::Rect(696, 474, 130, 180)));
//    got0.copyTo(img(cv::Rect(1088, 474, 130, 180)));
//    got0.copyTo(img(cv::Rect(1023, 298, 130, 180)));
//    got0.copyTo(img(cv::Rect(957, 474, 130, 180)));

    cv::Mat ret;
    cv::matchTemplate(img, mask, ret, cv::TemplateMatchModes::TM_CCORR_NORMED, mask);

    double minValue, maxValue;
    cv::Point minLocation, maxLocation;
    cv::Point matchLocation;
    minMaxLoc(ret, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());

    cv::rectangle(img, maxLocation, cv::Point(maxLocation.x + mask.cols, maxLocation.y + mask.rows), cv::Scalar(0,255,0), 2, 8);
    cv::imshow("匹配后的图像", img);
}

void testCVAlg2(){
    auto img = cv::imread("E:/test.png");
    cv::cvtColor(img, img, cv::COLOR_RGB2GRAY);
    //cv::resize(img, img, cv::Size(384, 203));
    threshold(img, img, 45, 255, cv::THRESH_BINARY);
    cv::imshow("hello", img);
    //cv::ellipse(img, cv::Point(100, 100), cv::Size(200, 150), 0, 0, 360, cv::Scalar(0, 0, 255), - 1, 8);
}

int main(int argc, char *argv[])
{
    //testCVAlg();
    //testCVAlg2();
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(dst::getCWD("/favicon.png")));
    app.setOrganizationName("somename");
    app.setOrganizationDomain("somename");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("applicationDirPath", QGuiApplication::applicationDirPath()); //https://recalll.co/ask/v/topic/qt-QML%3A-how-to-specify-image-file-path-relative-to-application-folder/55928bae7d3563c7088b7db1
    dst::streamManager::instance()->engine = &engine;
    TRIG("registerQMLClass", std::make_shared<dst::streamData>());

    QJsonObject lan;
    dst::configObject::loadJsonFileConfig("lanInfo", lan);
    QTranslator translator;
    QString ln = lan.value("name").toString();
        translator.load(ln);
    QCoreApplication::installTranslator(&translator);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    //dst::streamManager::instance()->emitSignal("setFocus", STMJSON(dst::Json("board", "panel0")));
    return app.exec();
}
