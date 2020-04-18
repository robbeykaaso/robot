#include "brain.h"
#include <iostream>
#include <QScreen>
#include <QDir>
#include <QJsonDocument>

/*HHOOK g_hook;
bool goahead = false;
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam){
//    std::cout << "hello" << std::endl;
    if (wParam == 39)
        goahead = true;
    else
        goahead = false;
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}*/

//copy form: https://blog.csdn.net/dancing_night/article/details/51545524
QImage cvMat2QImage(const cv::Mat& mat)
{
    if (mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        image.setColorCount(256);
        for (int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        uchar *pSrc = mat.data;
        for (int row = 0; row < mat.rows; row++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        // cv::Mat dst;
        // cv::cvtColor(mat, dst, cv::COLOR_GRAY2RGB);
        //  return QImage(dst.data, dst.cols, dst.rows, dst.step, QImage::Format_RGB888).copy();
        //return image.convertToFormat(QImage::Format_RGB888);
        return image;
    }
    else if (mat.type() == CV_8UC3)
    {
        // const uchar *pSrc = (const uchar*)mat.data;
        cv::Mat dst;
        cv::cvtColor(mat, dst, cv::COLOR_BGR2RGB);
        auto image = QImage(dst.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888).copy();
        return image.rgbSwapped();
    }
    else if (mat.type() == CV_8UC4)
    {
        const uchar *pSrc = (const uchar*)mat.data;
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        return QImage();
    }
}

cv::Mat QImage2cvMat(QImage image)
{
    cv::Mat mat;
    //    qDebug() << image.format();
    switch(image.format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
        break;
    case QImage::Format_Indexed8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
        break;
    }
    return mat;
}

void robotBrain::calcScene(const QImage& aImage){

};

void robotBrain::updateModel(){

}

bool robotBrain::calcOperation(){
    QJsonObject ret;
    ret.insert("type", "drag");
    HWND pWind = FindWindow("Notepad", NULL);
    if (pWind){
        RECT rect;
        GetWindowRect(pWind, &rect);
        POINT pt = {rect.right, rect.top}, pt2 = {rect.left, rect.bottom};
        ret.insert("org", dst::JArray(pt2.x + 115, pt.y + 25));
        if (m_tick++ % 2 == 0)
            ret.insert("del", dst::JArray(400, 0));
        else
            ret.insert("del", dst::JArray(- 400, 0));
    }
    TRIG("controlWorld", STMJSON(ret))
    return true;
}

void robotBrain::testCalc(const QImage& aImage){
    if (!m_tick++){
    cv::Mat src = QImage2cvMat(aImage);
    cv::cvtColor(src, src, cv::COLOR_RGB2GRAY);
    cv::Mat tmp = imread("config_/2.png", cv::IMREAD_GRAYSCALE);
    cv::Mat ret;
    cv::matchTemplate(src, tmp, ret, 0);
    normalize(ret, ret, 0, 1, cv::NORM_MINMAX);
    double minValue, maxValue;
    cv::Point minLocation, maxLocation;
    cv::Point matchLocation;
    minMaxLoc(ret, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());
    //cv::rectangle(ret, minLocation, cv::Point(minLocation.x + tmp.cols, minLocation.y + tmp.rows), cv::Scalar(0,255,0), 2, 8);
    cv::imshow("匹配后的图像", ret);
    //cv::Mat ret;
    //threshold(src, ret, 125, 255, cv::THRESH_BINARY);
    //imshow("test", ret);
    }
}

robotBrain::~robotBrain(){

}

robotBrain::robotBrain() : configObject(QJsonObject()){

    dst::streamManager::instance()->registerEvent("commandExportLog", "mdybrain", [this](std::shared_ptr<dst::streamData> aInput){
        QFile fl("log.txt");
        if (fl.open(QFile::WriteOnly)){
            fl.write(m_logs.toStdString().data());
            fl.close();
        }
        return aInput;
    });

    dst::streamManager::instance()->registerEvent("commandClearLog", "mdybrain", [this](std::shared_ptr<dst::streamData> aInput){
        m_logs = "";
        return aInput;
    });

    dst::streamManager::instance()->registerEvent("addLogRecord", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
        auto cfg = reinterpret_cast<dst::streamJson*>(aInput.get())->getData();
        m_logs += cfg->value("log_msg").toString() + "\n";
        return aInput;
    });

    dst::streamManager::instance()->registerEvent("setImageTransform", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
        auto cfg = reinterpret_cast<dst::ImageBoard::streamTransform*>(aInput.get());
        m_trans = cfg->getTransform();
        return aInput;
    });

    dst::streamManager::instance()->registerEvent("boardMousePress", "ndybrain",  [this](std::shared_ptr<dst::streamData> aInput){
        auto cfg = reinterpret_cast<dst::imageObject::streamImage*>(aInput.get());
        POINT w_pt;
        GetCursorPos(&w_pt);
        auto test = w_pt;
        return aInput;
    });

    dst::streamManager::instance()->registerEvent("boardKeyPress", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
        auto cfg = reinterpret_cast<dst::imageObject::streamImage*>(aInput.get())->getData();
        if (cfg->value("key") == 16777236){
            m_go = true;
            //TRIG("controlWorld", STMJSON(dst::Json("type", "drag",
            //                                       "org", dst::JArray(751, 916),
            //                                       "del", dst::JArray(478, - 428))))
        }
        else
            m_go = false;
        return aInput;
    });

    dst::streamManager::instance()->registerEvent("handleImage", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
        auto cfg = reinterpret_cast<dst::imageObject::streamImage*>(aInput.get());
        auto img = cfg->getImage()->getImage();
        if (m_go){
            calcScene(img);
            updateModel();
            calcOperation();
        }
        return aInput;
    }, "", "", 1);

    dst::streamManager::instance()->registerEvent("writeImageConfig", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
        aInput->callback(nullptr);
        return aInput;
    }, "", "", 1);

    dst::streamManager::instance()->registerEvent("commandCrop", "mdybrain", [this](std::shared_ptr<dst::streamData> aInput){
        QScreen *screen = QGuiApplication::primaryScreen();
        auto test = m_screen;
        auto img = screen->grabWindow(0).toImage();
        if (m_screen->getShapeList()->size() > 0){
            auto shp = m_screen->getShapeList()->begin().value();
            auto rect = shp->getActBound();
            auto pt1 = m_trans.map(QPoint(rect.left(), rect.top())), pt2 = m_trans.map(QPoint(rect.right(), rect.bottom()));
            QDir dir("config_");
            QString fl_nm = "config_/" + QString::number(dir.entryList().size());
            img.copy(pt1.x() * m_ratiox, pt1.y() * m_ratioy, (pt2.x() - pt1.x()) * m_ratiox, (pt2.y() - pt1.y()) * m_ratioy).save(fl_nm + ".png");
            QFile fl(fl_nm + ".json");
            fl.open(QFile::WriteOnly);
            fl.write(QJsonDocument(dst::JArray(pt1.x() * m_ratiox, pt1.y() * m_ratioy, pt2.x() * m_ratiox, pt2.y() * m_ratioy)).toJson());
            fl.close();
        }
        return aInput;
    }, "mdyGUI", "mdyGUI2");

    QScreen *screen = QGuiApplication::primaryScreen();
    auto img0 = screen->grabWindow(0).toImage();
    auto sz = screen->size();
    m_ratiox = img0.width() / sz.width(), m_ratioy = img0.height() / sz.height();
    QImage img(sz.width(), sz.height(), QImage::Format_ARGB32);
    img.fill("transparent");
    auto cfg = dst::Json("shapes", dst::Json("crop", dst::Json("type", "rectangle", "points", dst::JArray(100, 100, 200, 200))));
    cfg.insert("current_shape", "crop");
    m_screen = dst::cacheObject::createObject<dst::imageObject>(cfg, std::make_shared<dst::imageObject::imageAdditionalParameter>(img));

    auto stm = std::make_shared<dst::imageObject::streamImage>(dst::Json("board", "panel"));
    stm->setImage(m_screen);
    TRIG("showImage", stm);

   // HWND pWind = FindWindow("Notepad", NULL);

   // HINSTANCE tmp = HINSTANCE(GetWindowLongA(pWind, 0));
    //auto tmp = GetModuleHandle("kernel32.dll");
    //g_hook = SetWindowsHook(WH_KEYBOARD, KeyboardProc);
}


REGISTERPipe(collectShapeCommand, mdydnn, [](std::shared_ptr<dst::streamData> aInput){
    QJsonObject items;
    items.insert("crop", "commandCrop");
    items.insert("train", "commandTrainGem");
    items.insert("dnntest", "commandDnnTest");
    items.insert("exportLog", "commandExportLog");
    items.insert("clearLog", "commandClearLog");
    aInput->callback(&items);
    return aInput;
}, 0);

REGISTERPipe(collectShapeCommand, mdywindow, [](std::shared_ptr<dst::streamData> aInput){
    QJsonObject items;
    items.insert("window", "setWindowStyle");
    aInput->callback(&items);
    return aInput;
}, 0);

REGISTERPipe(brain, mdybak, [](std::shared_ptr<dst::streamData> aInput){
    aInput->callback(new robotBrain());
    return aInput;
}, 0);


REGISTERPipe(initializeBackend, inistg, [](std::shared_ptr<dst::streamData> aInput){
    dst::document::instance()->findModel("hearthStone");
    dst::document::instance()->findModel("eye");
    dst::document::instance()->findModel("hand");
    return aInput;
}, 0);
