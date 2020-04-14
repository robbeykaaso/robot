#ifndef APPLICATION_BRAIN_H_
#define APPLICATION_BRAIN_H_

#include "../framework/rxmanager.h"
#include "../framework/document.h"
#include "../framework/imageModel.h"
#include "../framework/ImageBoard.h"
#include <opencv2/opencv.hpp>

QImage cvMat2QImage(const cv::Mat& mat);
cv::Mat QImage2cvMat(QImage image);

class robotBrain : public dst::configObject{
protected:
    virtual void calcScene(const QImage& aImage);
    virtual void updateModel();
    virtual bool calcOperation();
private:
    void testCalc(const QImage& aImage);
    std::shared_ptr<dst::imageObject> m_screen;
    QTransform m_trans;
    double m_ratiox, m_ratioy;
public:
    robotBrain();
    ~robotBrain();
        // UnhookWindowsHookEx(g_hook);
private:
    QString m_logs = "";
    int m_tick = 0;
    bool m_go = false;
};

#endif
