#include "brain.h"
#include "../framework/util.h"
#include "decimal_infer.h"
#include <QJsonDocument>
#include <QDir>
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
    virtual double isCurrentScene(const cv::Mat& aScreen, const QImage& aOrigin) = 0;
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
            cv::cvtColor(cv_img, aFeature, cv::COLOR_RGB2GRAY);
        }
    }
protected:
    double calcFeatureIOU(const cv::Mat& aBackground, const cv::Mat& aFeature, const cv::Rect& aPos, cv::Rect& aRetPos, const cv::Mat& aMask = cv::Mat()){
        if (aFeature.cols == 0 || aFeature.rows == 0 || aPos.width == 0 || aPos.height == 0)
            return 0;
        auto src = aBackground(cv::Rect(aPos.x - 5, aPos.y - 5, aPos.width + 5, aPos.height + 5));

        cv::Mat ret;
        if (aMask.cols == 0){
            normalize(src, src, 0, 1, cv::NORM_MINMAX);
            cv::Mat fe;
            normalize(aFeature, fe, 0, 1, cv::NORM_MINMAX);
            cv::matchTemplate(src, fe, ret, cv::TemplateMatchModes::TM_SQDIFF_NORMED);
        }else
            cv::matchTemplate(src, aFeature, ret, cv::TemplateMatchModes::TM_CCORR_NORMED, aMask);

        double minValue, maxValue;
        cv::Point minLocation, maxLocation;
        cv::Point matchLocation;
        minMaxLoc(ret, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());

        if ((aMask.cols == 0 && minValue < 0.05) ||
            (aMask.cols > 0 && maxValue > 0.95)){
            cv::Rect target(minLocation.x, minLocation.y, aPos.width, aPos.height), origin(5, 5, aPos.width, aPos.height);
            aRetPos = cv::Rect(minLocation.x - 5 + aPos.x, minLocation.y - 5 + aPos.y, aPos.width, aPos.height);
            return (target & origin).area() * 1.0 / (target | origin).area();
        }else
            return 0;
        //cv::rectangle(src, minLocation, cv::Point(minLocation.x + m_ready_button.cols, minLocation.y + m_ready_button.rows), cv::Scalar(0,255,0), 2, 8);
        //cv::imshow("匹配后的图像", src);
    }
    virtual QString savePredictResult(const QString& aDirectory, const QImage& aImage){
        auto id = dst::configObject::generateObjectID();
        QDir().mkdir("config_/" + aDirectory);
        QDir().mkdir("config_/" + aDirectory + "/" + id);
        aImage.save("config_/" + aDirectory + "/" + id + "/0.png");
        return id;
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
    void placeCard(std::shared_ptr<card> aCard) {
        getCards(aCard->getCost()).erase(aCard);
        --m_cards_count;
        for (int i = 0; i < 10; ++i)
            for (auto j : m_cards[i]){
                auto idx = j->getIndex();
                if (idx > aCard->getIndex())
                    j->setIndex(idx - 1);
            }
    }
    void resetModel(){
        for (auto i : m_cards)
            i.clear();
        m_gem_count = 0;
        m_cards_count = 0;
        TRIG("resetGame", STMJSON(QJsonObject()));
    }
    std::set<std::shared_ptr<card>> getCards(int aCost) {return m_cards[aCost];}
    void setGemCount(int aCount) { m_gem_count = aCount;}
    int getGemCount() {return m_gem_count;}
private:
    int m_cards_count;
    int m_gem_count;
    std::set<std::shared_ptr<card>> m_cards[11];
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
    double isCurrentScene(const cv::Mat& aScreen, const QImage& aOrigin) override{
        return calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        aCards->resetModel();
    }
    QJsonObject calcOperation() override{
        return dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5));
    }
};

class selectScene : public scene{
private:
    cv::Rect m_loc_3[3];
    cv::Rect m_loc_4[4];
    cv::Rect m_split;
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
           //     cv::imshow("匹配后的图像", roi2);
            }
            aNumber = trainingServer::instance()->recognizeNumber(roi2);
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
        loadFeaturePos("select_split", m_split);
    }
    double isCurrentScene(const cv::Mat& aScreen, const QImage& aOrigin) override{
        double minValue, maxValue;
        cv::Point minLocation, maxLocation;
        cv::Point matchLocation;
        auto split = aScreen(m_split);
        minMaxLoc(split, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());

        cv::Rect* tgt = m_loc_3;
        int sum = 3;
        if (maxValue > 55){
            tgt = m_loc_4;
            sum = 4;
        }
        for (int i = 0; i < sum; ++i){
            int cost;
            isValidGem(tgt[i], aScreen, cost);
            m_cards.push_back(std::make_shared<card>(i, cost));
        }
        if (sum == 4)
            m_cards.push_back(std::make_shared<card>(4, 0));
        //cv::imwrite("config_/src.png", aScreen);

        savePredictResult("firstSelect", aOrigin);

        return 1;
    }
protected:
    QString savePredictResult(const QString& aDirectory, const QImage& aImage) override{
        auto ret = scene::savePredictResult(aDirectory, aImage);
        QJsonObject cfg;
        cfg.insert("id", ret);
        cfg.insert("images", dst::JArray(ret + "/0.png"));
        QJsonObject shps;
        auto sum = m_cards.size();
        cv::Rect* tgt = m_loc_3;
        if (sum > 4){
            sum--;
            tgt = m_loc_4;
        }
        for (int i = 0; i < sum; ++i){
            shps.insert(dst::configObject::generateObjectID(),
                        dst::Json("label", QString::number(m_cards.at(i)->getCost()),
                                  "type", "rectangle",
                                  "points", dst::JArray(tgt[i].x, tgt[i].y, tgt[i].x + tgt[i].width, tgt[i].y + tgt[i].height)));
        }
        cfg.insert("shapes", shps);

        QFile fl("config_/" + aDirectory + "/" + ret + ".json");
        if (fl.open(QFile::WriteOnly)){
            fl.write(QJsonDocument(cfg).toJson());
            fl.close();
        }
        return ret;
    }
};

class firstSelectScene : public selectScene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
private:
    cv::Rect m_set_loc;
    cv::Rect m_giveup_loc;
public:
    firstSelectScene() : selectScene(){
        loadFeatureImage("firstSelect", m_button);
        loadFeaturePos("firstSelect", m_loc);
        loadFeaturePos("firstSelectSet", m_set_loc);
        loadFeaturePos("giveup", m_giveup_loc);
        dst::streamManager::instance()->registerEvent("resetGame", "mdyfstscn", [this](std::shared_ptr<dst::streamData> aInput){
            m_cards.clear();
            return aInput;
        });
    }
    double isCurrentScene(const cv::Mat& aScreen, const QImage& aOrigin) override{
        if (m_cards.size() > 0)
            return 0;
        auto ret = calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
        if (ret > 0)
            selectScene::isCurrentScene(aScreen, aOrigin);
        return ret;
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        for (auto i : m_cards)
            aCards->addCard(i);
    }
    QJsonObject calcOperation() override{
        /*TRIG("controlWorld", STMJSON(dst::Json("type", "click",
                                               "org", dst::JArray(m_set_loc.x + m_set_loc.width * 0.5, m_set_loc.y + m_set_loc.height * 0.5))));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        TRIG("controlWorld", STMJSON(dst::Json("type", "click",
                                               "org", dst::JArray(m_giveup_loc.x + m_giveup_loc.width * 0.5, m_giveup_loc.y + m_giveup_loc.height * 0.5))));
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));*/
        //return QJsonObject();
        return dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5));
    }
};

class myTurnScene : public scene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
    std::shared_ptr<cardsModel> m_cards_model;
    cv::Rect m_card_place;
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

                auto st_pos = m_cards_model->getCardPos(card->getIndex());
                auto st_x = st_pos.x + st_pos.width * 0.5, st_y = st_pos.y + st_pos.height * 0.5;
                TRIG("controlWorld", STMJSON(dst::Json("type", "drag",
                                                       "org", dst::JArray(st_x, st_y),
                                                       "del", dst::JArray(m_card_place.x + m_card_place.width * 0.5 - st_x, m_card_place.y + m_card_place.height * 0.5 - st_y))));
                m_cards_model->placeCard(card);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                gem_count -= card->getCost();
                i -= card->getCost();
            }else
                --i;
        }
    }
    cv::Mat m_screen;
    QImage m_origin;
public:
    myTurnScene() : scene(){
        loadFeatureImage("myTurn", m_button);
        loadFeaturePos("myTurn", m_loc);
        loadFeaturePos("cardPlace", m_card_place);
    }
    double isCurrentScene(const cv::Mat &aScreen, const QImage& aOrigin) override{
        auto ret = calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
        if (ret == 1.0){
            m_screen = aScreen;
            m_origin = aOrigin;
        }
        return ret;
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        m_cards_model = aCards;

        int card_count = m_cards_model->getCardsCount();

        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); //wait for new supplied card

        if (card_count < 10){
            auto pos = m_cards_model->getCardPos(card_count);
            auto roi = m_screen(pos);
            auto cost = trainingServer::instance()->recognizeNumber(roi);
            m_cards_model->supplyCard(std::make_shared<card>(card_count, cost));

            auto id = savePredictResult("supplyCard", m_origin);
            QJsonObject cfg;
            cfg.insert("id", id);
            cfg.insert("images", dst::JArray(id + "/0.png"));
            QJsonObject shps;
            shps.insert(dst::configObject::generateObjectID(),
                        dst::Json("label", QString::number(cost),
                                  "type", "rectangle",
                                  "points", dst::JArray(pos.x, pos.y, pos.x + pos.width, pos.y + pos.height)));
            cfg.insert("shapes", shps);

            QFile fl("config_/supplyCard/" + id + ".json");
            if (fl.open(QFile::WriteOnly)){
                fl.write(QJsonDocument(cfg).toJson());
                fl.close();
            }
        }
        aCards->setGemCount(std::min(10, aCards->getGemCount() + 1));
    }
    QJsonObject calcOperation() override{
        //attackEnemy();
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
    double isCurrentScene(const cv::Mat &aScreen, const QImage& aOrigin) override{
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
        loadFeaturePos("gameOver", m_loc);
        loadFeatureImage("gameOver", m_button);
        if (m_button.cols > 0 && m_button.rows > 0){
            cv::Mat feature(m_button.rows, m_button.cols, CV_8UC1, cv::Scalar(0, 0, 0));
            for (int i = 0; i < 5; ++i){
                cv::Rect loc;
                loadFeaturePos("gameOver_" + QString::number(i), loc);
                if (loc.width > 0 && loc.height > 0)
                    m_button(loc).clone().copyTo(feature(loc));
            }
            m_button = feature;
            //cv::imshow("hello", m_button);
        }
    }
    double isCurrentScene(const cv::Mat& aScreen, const QImage& aOrigin) override{
        return calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc, m_button);
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{

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
        //m_scenes.push_back(std::make_shared<gameOverScene>());

        dst::streamManager::instance()->registerEvent("commandTrainGem", "mdyHearthStone", [this](std::shared_ptr<dst::streamData> aInput){
            trainingServer::instance()->trainGemModel();
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
            auto scr = i->isCurrentScene(dst, aImage);
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

