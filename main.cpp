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

#include "tiny_dnn/tiny_dnn.h"

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
    /*tiny_dnn::network<tiny_dnn::sequential> nn;
    nn << tiny_dnn::layers::conv(3, 1, 3, 1, 1, 1, tiny_dnn::padding::valid, true, 1, 1, 1, 1);
    //nn.load("Gem-LeNet-model.json", tiny_dnn::content_type::weights_and_model, tiny_dnn::file_format::json);
    nn.save("Gem-LeNet-model2.json", tiny_dnn::content_type::weights, tiny_dnn::file_format::json);
    tiny_dnn::vec_t data;
    data.push_back(1);
    data.push_back(1);
    data.push_back(1);
    auto ret = nn.predict(data);
    auto test = ret;*/
    auto img = cv::imread("F:/ttt/Hearthstone Screenshot 03-30-20 21.41.19.png");
    cv::cvtColor(img, img, cv::COLOR_RGB2GRAY);
    img = img(cv::Rect(470, 540, 950, 100));
    cv::resize(img, img, cv::Size(512, 100));
    //img = 255 - img;
    auto bk = (cv::sum(img.colRange(511, 512)) + cv::sum(img.colRange(0, 1))) / 200.0;
    img = abs(img - bk);
    //cv::threshold(img, img, 160, 255, cv::THRESH_TRUNC);
    cv::imshow("hello", img);
    //img = 255 - img;
    cv::imwrite("test2.png", img);

    normalize(img, img, 0, 1, cv::NORM_MINMAX);
    std::vector<int> integration(512);
    for (int i = 0; i < 512; ++i){
        auto tmp = img.colRange(i, i + 1);
        auto test = cv::sum(tmp);
        integration[i] = test.val[0];
    }

    img = img;
    //cv::ellipse(img, cv::Point(100, 100), cv::Size(200, 150), 0, 0, 360, cv::Scalar(0, 0, 255), - 1, 8);
}

void testCVAlg3() {
    tiny_dnn::network<tiny_dnn::sequential> nn;
    using fc = tiny_dnn::layers::fc;
    using conv = tiny_dnn::layers::conv;
    using max_pool = tiny_dnn::layers::max_pool;
    using relu = tiny_dnn::activation::relu;
    using padding = tiny_dnn::padding;
    using softmax = tiny_dnn::softmax_layer;

    nn << conv(512, 1, 2, 1, 1, 2, padding::same, true, 1, 1, 1, 1) //C1, 1@512*1-in, 2@512*1-out
       << relu()
       << max_pool(512, 1, 2, 2, 2, false) //S2, 2@512*1-in, 2@256*1-out
       << conv(256, 1, 4, 1, 2, 4, padding::same, true, 1, 1, 1, 1) //C3, 2@256*1-in, 4@256*1-out
       << relu()
       << max_pool(256, 1, 4, 4, 4, false) //S4, 4@256*1-in, 4@64*1-out
       << conv(64, 1, 4, 1, 4, 8, padding::same, true, 1, 1, 1, 1) //C5, 4@64*1-in, 8@64*1-out
       << relu()
       << max_pool(64, 1, 8, 4, 4, false) //S6, 8@64*1-in, 8@16*1-out
       << conv(16, 1, 8, 1, 8, 16, padding::same, true, 1, 1, 1, 1) //C7, 8@16*1-in, 16@16*1-out
       << relu()
       << max_pool(16, 1, 16, 4, 4, false)  //S8, 16@16*1-in, 16@4*1-in
       << conv(4, 1, 4, 1, 16, 32, padding::valid, true, 1, 1, 1, 1) //C7, 16@4*1-in, 32@1*1-out
       << relu()
       << fc(32, 8, true) //F8, 16-in, 8-out
       << relu()
       << softmax();
}

int main(int argc, char *argv[])
{
    //testCVAlg();
    //testCVAlg2();
    //testCVAlg3();
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
