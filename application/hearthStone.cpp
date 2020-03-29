#include "brain.h"
#include "../framework/util.h"
#include "decimal_infer.h"
#include <QJsonDocument>
#include <opencv2/opencv.hpp>

class card{
public:
    card(int aIndex, int aCost) {
        m_index = aIndex;
        m_cost = aCost;
    }
    int getCost() {return m_cost;}
private:
    int m_index;
    int m_cost;
    cv::Rect m_pos;
};

class cardsModel{
public:
    void addCard(std::shared_ptr<card> aCard){
        m_cards[aCard->getCost()].insert(aCard);
    }
    void reset(){
       for (auto i : m_cards)
           i.clear();
    }
private:
    std::set<std::shared_ptr<card>> m_cards[10];
};

class scene{
public:
    virtual ~scene(){}
    virtual double isCurrentScene(const cv::Mat& aScreen) = 0;
    virtual void updateModel(std::shared_ptr<cardsModel> aCards) = 0;
    virtual QJsonObject calcOperation() = 0;
protected:
    double calcFeatureIOU(const cv::Mat& aBackground, const cv::Mat& aFeature, const cv::Rect& aPos, cv::Rect& aRetPos){
        auto src = aBackground(cv::Rect(aPos.x - 5, aPos.y - 5, aPos.width + 5, aPos.height + 5));

        cv::Mat ret;
        cv::matchTemplate(src, aFeature, ret, cv::TemplateMatchModes::TM_SQDIFF_NORMED);

        double minValue, maxValue;
        cv::Point minLocation, maxLocation;
        cv::Point matchLocation;
        minMaxLoc(ret, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());

        if (minValue < 0.05){
            cv::Rect target(minLocation.x, minLocation.y, aPos.width, aPos.height), origin(5, 5, aPos.width, aPos.height);
            aRetPos = cv::Rect(minLocation.x - 5 + aPos.x, minLocation.y - 5 + aPos.y, aPos.width, aPos.height);
            return (target & origin).area() * 1.0 / (target | origin).area();
        }else
            return 0;
        //cv::rectangle(src, minLocation, cv::Point(minLocation.x + m_ready_button.cols, minLocation.y + m_ready_button.rows), cv::Scalar(0,255,0), 2, 8);
        //cv::imshow("匹配后的图像", src);
    }
    void loadFeature(const QString& aName, cv::Mat& aFeature, cv::Rect& aPos){
        QFile fl("config_/hearthStone/" + aName + ".json");
        if (fl.open(QFile::ReadOnly)){
            auto rect = QJsonDocument::fromJson(fl.readAll()).array();
            aPos.x = rect[0].toInt();
            aPos.y = rect[1].toInt();
            aPos.width = rect[2].toInt() - rect[0].toInt();
            aPos.height = rect[3].toInt() - rect[1].toInt();
            fl.close();
        }

        QImage img("config_/hearthStone/" + aName + ".png");
        if (!img.isNull()){
            auto cv_img = QImage2cvMat(img);
            cv::cvtColor(cv_img, cv_img, cv::COLOR_RGB2GRAY);
            normalize(cv_img, aFeature, 0, 1, cv::NORM_MINMAX);
        }
    }
    bool m_valid = false;
};

class readyScene : public scene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
public:
    readyScene() : scene(){
        loadFeature("ready", m_button, m_loc);
    }
    double isCurrentScene(const cv::Mat& aScreen) override{
        return calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        aCards.reset();
    }
    QJsonObject calcOperation() override{
        return dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5));
    }
};

class selectScene : public scene{
private:
    cv::Rect m_loc_3[3];
    cv::Rect m_loc_4[4];
protected:
    std::vector<std::shared_ptr<card>> m_cards;
private:
    bool isValidGem(const cv::Rect& aPos, const cv::Mat& aBackground, int &aNumber){
        auto roi = aBackground(cv::Rect(aPos.x, aPos.y, aPos.width, aPos.height));
        threshold(roi, roi, 20, 255, cv::THRESH_BINARY);
        aNumber = recognizeNumber(roi);
        return false;
    };
public:
    selectScene() : scene(){
        cv::Mat mt;
        for (int i = 0; i < 3; ++i)
            loadFeature("select3_" + QString::number(i), mt, m_loc_3[i]);
        for (int i = 0; i < 4; ++i)
            loadFeature("select4_" + QString::number(i), mt, m_loc_4[i]);
    }
    double isCurrentScene(const cv::Mat& aScreen) override{
        int cost[4];

        int valid = 0;
        for (int i = 0; i < 3; ++i)
            valid = isValidGem(m_loc_3[i], aScreen, cost[i]) ? valid + 1 : valid;
        if (valid == 3){
            for (int i = 0; i < 3; ++i)
                m_cards.push_back(std::make_shared<card>(i, cost[i]));
            return 1;
        }

        valid = 0;
        for (int i = 0; i < 4; ++i)
            valid = isValidGem(m_loc_4[i], aScreen, cost[i]) ? valid + 1 : valid;
        if (valid == 4){
            for (int i = 0; i < 4; ++i)
                m_cards.push_back(std::make_shared<card>(i, cost[i]));
            return 1;
        }

        return 0;
    }
};

class firstSelectScene : public selectScene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
public:
    firstSelectScene() : selectScene(){
        loadFeature("firstSelect", m_button, m_loc);
    }
    double isCurrentScene(const cv::Mat& aScreen) override{
        selectScene::isCurrentScene(aScreen);
        return calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        for (auto i : m_cards)
            aCards->addCard(i);
    }
    QJsonObject calcOperation() override{
        return dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5));
    }
};

class myTurnScene : public scene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
public:
    myTurnScene() : scene(){
        loadFeature("myTurn", m_button, m_loc);
    }
    double isCurrentScene(const cv::Mat &aScreen) override{
        auto ret = calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
        if (ret == 1.0){

        }
        return ret;
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{

    }
    QJsonObject calcOperation() override{
        return QJsonObject();
    }
};

class EnemyTurnScene : public scene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
public:
    EnemyTurnScene() : scene(){
        loadFeature("enemyTurn", m_button, m_loc);
    }
    double isCurrentScene(const cv::Mat &aScreen) override{
        cv::Rect mt;
        return calcFeatureIOU(aScreen, m_button, m_loc, mt);
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{

    }
    QJsonObject calcOperation() override{
        return QJsonObject();
    }
};

class gameOverScene : public scene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
public:
    gameOverScene() : scene(){
        loadFeature("gameOver", m_button, m_loc);
    }
    double isCurrentScene(const cv::Mat& aScreen) override{
        return calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        aCards.reset();
    }
    QJsonObject calcOperation() override{
        return dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5));
    }
};

class hearthStoneBrain : public robotBrain{
public:
    hearthStoneBrain() : robotBrain(){
        m_cards = std::make_shared<cardsModel>();
        m_scenes.push_back(std::make_shared<readyScene>());
        m_scenes.push_back(std::make_shared<firstSelectScene>());
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
        if (m_current_scene)
            m_current_scene->updateModel(m_cards);
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
    std::shared_ptr<cardsModel> m_cards;
};

REGISTERPipe(hearthStone, mdybak, [](std::shared_ptr<dst::streamData> aInput){
    aInput->callback(new hearthStoneBrain());
    return aInput;
}, 0);

