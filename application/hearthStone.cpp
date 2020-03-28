#include "brain.h"
#include <QJsonDocument>
#include <opencv2/opencv.hpp>

class scene{
public:
    virtual ~scene(){}
    virtual double isCurrentScene(const cv::Mat& aScreen) = 0;
    virtual QJsonObject calcOperation() = 0;
protected:
    bool m_valid = false;
};

class readyScene : public scene{
private:
    cv::Mat m_ready_button;
    cv::Rect m_loc;
    cv::Rect m_actloc;
public:
    readyScene() : scene(){
        QFile fl("config_/hearthStone/tag_ready.json");
        if (fl.open(QFile::ReadOnly)){
            auto rect = QJsonDocument::fromJson(fl.readAll()).array();
            m_loc.x = rect[0].toInt();
            m_loc.y = rect[1].toInt();
            m_loc.width = rect[2].toInt() - rect[0].toInt();
            m_loc.height = rect[3].toInt() - rect[1].toInt();
            fl.close();
        }

        QImage img("config_/hearthStone/tag_ready.png");
        if (!img.isNull()){
            auto cv_img = QImage2cvMat(img);;
            cv::cvtColor(cv_img, m_ready_button, cv::COLOR_RGB2GRAY);
            normalize(m_ready_button, m_ready_button, 0, 1, cv::NORM_MINMAX);
        }

    }
    ~readyScene() override{}

    double isCurrentScene(const cv::Mat& aScreen) override{
        auto src = aScreen(cv::Rect(m_loc.x - 5, m_loc.y - 5, m_loc.width + 5, m_loc.height + 5));

        cv::Mat ret;
        cv::matchTemplate(src, m_ready_button, ret, cv::TemplateMatchModes::TM_SQDIFF_NORMED);

        double minValue, maxValue;
        cv::Point minLocation, maxLocation;
        cv::Point matchLocation;
        minMaxLoc(ret, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());

        if (minValue < 0.05){
            cv::Rect target(minLocation.x, minLocation.y, m_loc.width, m_loc.height), origin(5, 5, m_loc.width, m_loc.height);
            m_actloc = cv::Rect(minLocation.x - 5 + m_loc.x, minLocation.y - 5 + m_loc.y, m_loc.width, m_loc.height);
            return (target & origin).area() * 1.0 / (target | origin).area();
        }else
            return 0;
        //cv::rectangle(src, minLocation, cv::Point(minLocation.x + m_ready_button.cols, minLocation.y + m_ready_button.rows), cv::Scalar(0,255,0), 2, 8);
        //cv::imshow("匹配后的图像", src);
    }
    QJsonObject calcOperation() override{
        return dst::Json("type", "click", "org", dst::JArray(m_actloc.x + m_actloc.width * 0.5, m_actloc.y + m_actloc.height * 0.5));
    }
};

class hearthStoneBrain : public robotBrain{
public:
    hearthStoneBrain() : robotBrain(){
        m_scenes.push_back(std::make_shared<readyScene>());
    }
protected:
    void calcScene(const QImage& aImage) override{
        m_current_scene = nullptr;
        double score = - 1;

        auto cv_img = QImage2cvMat(aImage);
        cv::Mat dst;
        cv::cvtColor(cv_img, dst, cv::COLOR_RGB2GRAY);
        normalize(dst, dst, 0, 1, cv::NORM_MINMAX);

        for (auto i : m_scenes){
            auto scr = i->isCurrentScene(dst);
            if (scr > score && scr > 0.5){
                m_current_scene = i;
            }
        }
    }
    void updateModel() override{

    }
    QJsonObject calcOperation() override {
        if (m_current_scene)
            return m_current_scene->calcOperation();
        else
            return QJsonObject();
    }
private:
    std::shared_ptr<scene> m_current_scene;
    std::vector<std::shared_ptr<scene>> m_scenes; //ready, selectCards, playing_me, playing_rival, gameover

};

REGISTERPipe(hearthStone, mdybak, [](std::shared_ptr<dst::streamData> aInput){
    aInput->callback(new hearthStoneBrain());
    return aInput;
}, 0);

