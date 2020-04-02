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
    int getIndex() {return m_index;}
    void setIndex(int aIndex) {m_index = aIndex;}
   // cv::Rect getPos() {return m_pos;}
private:
    int m_index;
    int m_cost;
   // cv::Rect m_pos;
};

class cardsModel;

class scene{
public:
    virtual ~scene(){}
    virtual double isCurrentScene(const cv::Mat& aScreen) = 0;
    virtual void updateModel(std::shared_ptr<cardsModel> aCards) = 0;
    virtual QJsonObject calcOperation() = 0;
    static void loadFeaturePos(const QString& aName, cv::Rect& aPos){
        QFile fl("config_/hearthStone/" + aName + ".json");
        if (fl.open(QFile::ReadOnly)){
            auto rect = QJsonDocument::fromJson(fl.readAll()).array();
            aPos.x = rect[0].toInt();
            aPos.y = rect[1].toInt();
            aPos.width = rect[2].toInt() - rect[0].toInt();
            aPos.height = rect[3].toInt() - rect[1].toInt();
            fl.close();
        }
    }
    static void loadFeatureImage(const QString& aName, cv::Mat& aFeature){
        QImage img("config_/hearthStone/" + aName + ".png");
        if (!img.isNull()){
            auto cv_img = QImage2cvMat(img);
            cv::cvtColor(cv_img, cv_img, cv::COLOR_RGB2GRAY);
            normalize(cv_img, aFeature, 0, 1, cv::NORM_MINMAX);
        }
    }
protected:
    double calcFeatureIOU(const cv::Mat& aBackground, const cv::Mat& aFeature, const cv::Rect& aPos, cv::Rect& aRetPos){
        if (aFeature.cols == 0 || aFeature.rows == 0 || aPos.width == 0 || aPos.height == 0)
            return 0;
        auto src = aBackground(cv::Rect(aPos.x - 5, aPos.y - 5, aPos.width + 5, aPos.height + 5));
        normalize(src, src, 0, 1, cv::NORM_MINMAX);

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
    bool m_valid = false;
};

class cardsModel{
public:
    cardsModel(){
        for (int i = 0; i < 10; ++i)
            for (int j = 0; j < i + 1; ++j){
                scene::loadFeaturePos("cards_pos/" + QString::number(i) + "_" + QString::number(j), m_cards_pos[i][j]);
            }
    }
    void addCard(std::shared_ptr<card> aCard){
        m_cards[aCard->getCost()].insert(aCard);
        ++m_cards_count;
    }
    int getCardsCount() {return m_cards_count;}
    cv::Rect getCardPos(int aIndex){
        return m_cards_pos[m_cards_count - 1][aIndex];
    }
    void supplyCard(std::shared_ptr<card> aCard){
        if (m_cards_count > 9)
            return;
        addCard(aCard);
    }
    void placeCard(int aIndex) {
        --m_cards_count;
        for (int i = 0; i < 10; ++i)
            for (auto j : m_cards[i]){
                auto idx = j->getIndex();
                if (idx > aIndex)
                    j->setIndex(idx - 1);
            }
    }
    void reset(){
        for (auto i : m_cards)
            i.clear();
        m_gem_count = 0;
        m_cards_count = 0;
    }
    std::set<std::shared_ptr<card>> getCards(int aCost) {return m_cards[aCost];}
    void setGemCount(int aCount) { m_gem_count = aCount;}
    int getGemCount() {return m_gem_count;}
private:
    int m_cards_count;
    int m_gem_count;
    std::set<std::shared_ptr<card>> m_cards[10];
    cv::Rect m_cards_pos[10][10];
};

class readyScene : public scene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
public:
    readyScene() : scene(){
        loadFeaturePos("ready", m_loc);
        loadFeatureImage("ready", m_button);
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
    int m_tick = 0;
private:
    bool isValidGem(const cv::Rect& aPos, const cv::Mat& aBackground, int &aNumber){
        if (aPos.width > 0 && aPos.height > 0){
            auto roi = aBackground(cv::Rect(aPos.x, aPos.y, aPos.width, aPos.height));
            cv::Mat roi2;
            cv::resize(roi, roi2, cv::Size(32, 32));
            if (!m_tick++){
                cv::imshow("匹配后的图像", roi2);
            }
            aNumber = recognizeNumber(roi2);
            return aNumber >= 0;
        }else
            return false;
    }
public:
    selectScene() : scene(){
        for (int i = 0; i < 3; ++i)
            loadFeaturePos("select3_" + QString::number(i), m_loc_3[i]);
        for (int i = 0; i < 4; ++i)
            loadFeaturePos("select4_" + QString::number(i), m_loc_4[i]);
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
        loadFeatureImage("firstSelect", m_button);
        loadFeaturePos("firstSelect", m_loc);
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
    std::shared_ptr<cardsModel> m_cards_model;
    cv::Point m_card_place;
private:
    void attackEnemy(){

    }
    void placeCards(){
        int gem_count = m_cards_model->getGemCount();

        auto higher_cost = std::min(gem_count, 10);
        for (int i = higher_cost; i >= 0;){
            auto cards = m_cards_model->getCards(i);
            if (cards.size() > 0){
                auto card = *cards.begin();
                cards.erase(cards.begin());
                gem_count -= card->getCost();
                i -= card->getCost();
                auto st_pos = m_cards_model->getCardPos(card->getIndex());
                auto st_x = st_pos.x + st_pos.width * 0.5, st_y = st_pos.y + st_pos.height * 0.5;
                TRIG("controlWorld", STMJSON(dst::Json("type", "drag",
                                                       "org", dst::JArray(st_x, st_y),
                                                       "del", dst::JArray(m_card_place.x - st_x, m_card_place.y - st_y))));
                m_cards_model->placeCard(card->getIndex());
                continue;
            }else
                --i;
        }
    }
    cv::Mat m_screen;
public:
    myTurnScene() : scene(){
        loadFeatureImage("myTurn", m_button);
        loadFeaturePos("myTurn", m_loc);
    }
    double isCurrentScene(const cv::Mat &aScreen) override{
        auto ret = calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
        if (ret == 1.0)
            m_screen = aScreen;
        return ret;
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        m_cards_model = aCards;

        int card_count = m_cards_model->getCardsCount();
        if (card_count < 10){
            auto pos = m_cards_model->getCardPos(card_count);
            auto roi = m_screen(pos);
            auto cost = recognizeNumber(roi);
            m_cards_model->supplyCard(std::make_shared<card>(card_count, cost));
        }
    }
    QJsonObject calcOperation() override{
        attackEnemy();
        placeCards();
        return dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5));
    }
};

class enemyTurnScene : public scene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
public:
    enemyTurnScene() : scene(){
        loadFeatureImage("enemyTurn", m_button);
        loadFeaturePos("enemyTurn", m_loc);
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
        loadFeatureImage("gameOver", m_button);
        loadFeaturePos("gameOver", m_loc);
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
        m_scenes.push_back(std::make_shared<myTurnScene>());
        m_scenes.push_back(std::make_shared<enemyTurnScene>());
        m_scenes.push_back(std::make_shared<gameOverScene>());

        dst::streamManager::instance()->registerEvent("commandTrainGem", "mdyHearthStone", [this](std::shared_ptr<dst::streamData> aInput){
            trainGemModel();
            return aInput;
        });

        dst::streamManager::instance()->registerEvent("commandDnnTest", "mdyHearthStone", [this](std::shared_ptr<dst::streamData> aInput){
            dnnTest();
            return aInput;
        });
    }
protected:
    void calcScene(const QImage& aImage) override{
        m_current_scene = nullptr;
        double score = - 1;

        auto cv_img = QImage2cvMat(aImage);
        cv::Mat dst;
        cv::cvtColor(cv_img, dst, cv::COLOR_RGB2GRAY);

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

