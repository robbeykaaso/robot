#include "brain.h"
#include "../framework/util.h"
#include "decimal_infer.h"
#include <QJsonDocument>
#include <QDir>
#include <QScreen>
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
    virtual bool calcOperation() { return false;}
protected:
    double calcFeatureIOU(const cv::Mat& aBackground, const cv::Mat& aFeature, const cv::Rect& aPos, cv::Rect& aRetPos,
                          int aInflate = 5, const cv::Mat& aMask = cv::Mat(),
                          std::function<cv::Mat(const cv::Mat&)> aTransform = [](const cv::Mat& aImage){
                              cv::Mat ret;
                              normalize(aImage, ret, 0, 1, cv::NORM_MINMAX);
                              return ret;
                          })
    {
        if (aFeature.cols == 0 || aFeature.rows == 0 || aPos.width == 0 || aPos.height == 0)
            return 0;
        auto src = aBackground(cv::Rect(aPos.x - aInflate, aPos.y - aInflate, aPos.width + 2 * aInflate, aPos.height + 2 * aInflate));

        cv::Mat ret;
        if (aMask.cols == 0){
            cv::Mat fe;
            src = aTransform(src);
            fe = aTransform(aFeature);
            //cv::imshow("hi", src);
            //cv::imshow("hi2", fe);
            cv::matchTemplate(src, fe, ret, cv::TemplateMatchModes::TM_SQDIFF_NORMED);
        }else
            cv::matchTemplate(src, aFeature, ret, cv::TemplateMatchModes::TM_CCORR_NORMED, aMask);

        double minValue, maxValue;
        cv::Point minLocation, maxLocation;
        cv::Point matchLocation;
        minMaxLoc(ret, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());

        if ((aMask.cols == 0 && minValue < 0.05) ||
            (aMask.cols > 0 && maxValue > 0.999)){
            cv::Rect target(minLocation.x, minLocation.y, aPos.width, aPos.height), origin(5, 5, aPos.width, aPos.height);
            aRetPos = cv::Rect(minLocation.x - 5 + aPos.x, minLocation.y - 5 + aPos.y, aPos.width, aPos.height);
            return (target & origin).area() * 1.0 / (target | origin).area();
        }else
            return 0;
        //cv::rectangle(src, minLocation, cv::Point(minLocation.x + m_ready_button.cols, minLocation.y + m_ready_button.rows), cv::Scalar(0,255,0), 2, 8);
        //cv::imshow("匹配后的图像", src);
    }
    virtual QString savePredictResult(const QString& aDirectory, const QImage& aImage, const QString& aLabel = ""){
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
                loadFeaturePos("cards_pos/" + QString::number(i) + "_" + QString::number(j), m_cards_pos[i][j]);
            }
    }
    void addCard(std::shared_ptr<card> aCard){
        dst::showDstLog("card add: index " + QString::number(aCard->getIndex()) + "; cost " + QString::number(aCard->getCost()));
        m_cards[aCard->getCost()].insert(aCard);
        ++m_cards_count;
    }
    int getCardsCount() {return m_cards_count;}
    cv::Rect getCardPos(int aSum, int aIndex){
        return m_cards_pos[aSum][aIndex];
    }
    void supplyCard(std::shared_ptr<card> aCard){
        if (m_cards_count > 9)
            return;
        addCard(aCard);
    }
    void placeCard(std::shared_ptr<card> aCard) {
        dst::showDstLog("card place: index " + QString::number(aCard->getIndex()) + "; cost " + QString::number(aCard->getCost()));
        getCards(aCard->getCost())->erase(aCard);
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
        TRIG("resetGame", STMJSON(QJsonObject()))
    }
    std::set<std::shared_ptr<card>>* getCards(int aCost) {return &m_cards[aCost];}
    void setGemCount(int aCount) { m_gem_count = aCount;}
    int getGemCount() {return m_gem_count;}
private:
    int m_cards_count = 0;
    int m_gem_count = 0;
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
        dst::showDstLog("ready : ");
        aCards->resetModel();
    }
    bool calcOperation() override{
        TRIG("controlWorld", STMJSON(dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5))));
        return true;
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

        auto idx = trainingServer::instance()->recognizeCount34(aScreen, m_loc_3, m_loc_4);
        savePredictResult("firstSelect", aOrigin, QString::number(idx));

        return 1;
    }
protected:
    QString savePredictResult(const QString& aDirectory, const QImage& aImage, const QString& aLabel = "") override{
        auto ret = scene::savePredictResult(aDirectory, aImage);
        QJsonObject cfg;
        if (aLabel != "")
            cfg.insert("labels", dst::Json("custom", aLabel));
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
        if (ret > 0.99){
            dst::showDstLog("first select conf: " + QString::number(ret));
            selectScene::isCurrentScene(aScreen, aOrigin);
        }
        return ret;
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        dst::showDstLog("first select : ");
        for (auto i : m_cards)
            aCards->addCard(i);
    }
    bool calcOperation() override{
        /*TRIG("controlWorld", STMJSON(dst::Json("type", "click",
                                               "org", dst::JArray(m_set_loc.x + m_set_loc.width * 0.5, m_set_loc.y + m_set_loc.height * 0.5))));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        TRIG("controlWorld", STMJSON(dst::Json("type", "click",
                                               "org", dst::JArray(m_giveup_loc.x + m_giveup_loc.width * 0.5, m_giveup_loc.y + m_giveup_loc.height * 0.5))));
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));*/
        //return QJsonObject();
        TRIG("controlWorld", STMJSON(dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5))));
        return true;
    }
};

class attendant{
private:
    int m_attack;
    int m_health;
};

class myTurnScene : public scene{
private:
    cv::Mat m_screen;
    QImage m_origin;
private:
    cv::Rect m_my_count_feature;
    cv::Rect m_enemy_count_feature;
    cv::Rect m_;
    void attackEnemy(){
        /*if (m_my_count_feature.width > 0 && m_enemy_count_feature.width > 0){
            auto my_cnt = trainingServer::instance()->recognizeCount(m_screen(m_my_count_feature));
            auto enemy_cnt = trainingServer::instance()->recognizeCount(m_screen(m_enemy_count_feature));

            auto id = savePredictResult("attendentCount", m_origin);
            QJsonObject cfg;
            cfg.insert("id", id);
            cfg.insert("images", dst::JArray(id + "/0.png"));
            QJsonObject shps;
            shps.insert(dst::configObject::generateObjectID(),
                        dst::Json("label", QString::number(my_cnt),
                                  "type", "rectangle",
                                  "points", dst::JArray(m_my_count_feature.x, m_my_count_feature.y,
                                                        m_my_count_feature.x + m_my_count_feature.width,
                                                        m_my_count_feature.y + m_my_count_feature.height)));
            shps.insert(dst::configObject::generateObjectID(),
                        dst::Json("label", QString::number(enemy_cnt),
                                  "type", "rectangle",
                                  "points", dst::JArray(m_enemy_count_feature.x, m_enemy_count_feature.y,
                                                        m_enemy_count_feature.x + m_enemy_count_feature.width,
                                                        m_enemy_count_feature.y + m_enemy_count_feature.height)));
            cfg.insert("shapes", shps);

            QFile fl("config_/attendentCount/" + id + ".json");
            if (fl.open(QFile::WriteOnly)){
                fl.write(QJsonDocument(cfg).toJson());
                fl.close();
            }
        }*/
    }
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
    cv::Rect m_gem_loc;
    cv::Rect m_hero_loc;
    cv::Rect m_enemy_hero_loc;
    std::shared_ptr<cardsModel> m_cards_model;
    cv::Rect m_card_place;
    cv::Rect m_attendant_pos[7][7], m_enemy_pos[7][7];
    int m_offset_left = 70;
    int m_offset_top = 176;
private:
    void captureScreen(){
        QScreen *screen = QGuiApplication::primaryScreen();
        m_origin = screen->grabWindow(0).toImage();
        m_screen = QImage2cvMat(m_origin);
        cv::cvtColor(m_screen, m_screen, cv::COLOR_RGB2GRAY);
    }
    void recognizeAttendants(){
        auto idx = trainingServer::instance()->recognizeCount7(m_screen, m_my_count_feature);
        std::vector<cv::Rect> poses; std::vector<int> lbls;
        for (int i = 0; i < idx; ++i){
            auto pos = m_attendant_pos[idx - 1][i];
            poses.push_back(pos);
            lbls.push_back(trainingServer::instance()->recognizeNumber(m_screen(pos)));
            pos.x = pos.x - m_offset_left;
            poses.push_back(pos);
            lbls.push_back(trainingServer::instance()->recognizeNumber(m_screen(pos)));
        }

        if (m_enemy_hero_loc.width > 0 && m_enemy_hero_loc.height > 0)
            for (auto i : poses){
                auto st_x = i.x + i.width * 0.5, st_y = i.y + i.height * 0.5;
                TRIG("controlWorld", STMJSON(dst::Json("type", "drag",
                                                       "org", dst::JArray(st_x, st_y),
                                                       "del", dst::JArray(m_enemy_hero_loc.x + m_enemy_hero_loc.width * 0.5 - st_x, m_enemy_hero_loc.y + m_enemy_hero_loc.height * 0.5 - st_y))));
            }

        auto idx2 = trainingServer::instance()->recognizeCount7(m_screen, m_enemy_count_feature);
        for (int i = 0; i < idx; ++i){
            auto pos = m_enemy_pos[idx - 1][i];
            poses.push_back(pos);
            lbls.push_back(trainingServer::instance()->recognizeNumber(m_screen(pos)));
            pos.x = pos.x - m_offset_left;
            poses.push_back(pos);
            lbls.push_back(trainingServer::instance()->recognizeNumber(m_screen(pos)));
        }
        savePredictResult2(poses, lbls, dst::Json("custom", QString::number(idx), "enemy", QString::number(idx2)), "attendantCount");
    }
private:
    void placeCards(){
        int gem_count = m_cards_model->getGemCount();
        std::set<std::shared_ptr<card>> used;
        int i = - 1;
        do{
            dst::showDstLog("myTurn GemCount : " + QString::number(gem_count));
            auto higher_cost = std::min(gem_count, 10);
            i = higher_cost;
            auto new_gem_count = gem_count;
            for (; i >= 0; i--){
                auto cards = *m_cards_model->getCards(i);
                for (auto card : cards){
                    if (used.find(card) != used.end())
                        continue;
                    auto st_pos = m_cards_model->getCardPos(m_cards_model->getCardsCount() - 1, card->getIndex());
                    auto st_x = st_pos.x + st_pos.width * 0.5, st_y = st_pos.y + st_pos.height * 0.5;
                    TRIG("controlWorld", STMJSON(dst::Json("type", "drag",
                                                           "org", dst::JArray(st_x, st_y),
                                                           "del", dst::JArray(m_card_place.x + m_card_place.width * 0.5 - st_x, m_card_place.y + m_card_place.height * 0.5 - st_y))));
                    used.insert(card);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    captureScreen();
                    new_gem_count = trainingServer::instance()->recognizeNumber(m_screen(m_gem_loc));
                    dst::showDstLog("myTurn new GemCount : " + QString::number(new_gem_count));
                    savePredictResult2(m_gem_loc, QString::number(new_gem_count));
                    if (new_gem_count != gem_count || card->getCost() == 0){
                        m_cards_model->placeCard(card);
                        break;
                    }else
                        dst::showDstLog("card place fail: index " + QString::number(card->getIndex()) + "; cost " + QString::number(card->getCost()));
                }
                if (new_gem_count != gem_count){
                    gem_count = new_gem_count;
                    break;
                }
            }
        }while(i >= 0);

        if (gem_count > 1)
            TRIG("controlWorld", STMJSON(dst::Json("type", "click",
                                                   "org", dst::JArray(m_hero_loc.x + m_hero_loc.width * 0.5, m_hero_loc.y + m_hero_loc.height * 0.5))))
    }
private:
    void savePredictResult2(const std::vector<cv::Rect>& aPoses, const std::vector<int>& aLabels, const QJsonObject aLabel = QJsonObject(), const QString& aDir = "supplyCard"){
        auto id = savePredictResult(aDir, m_origin);
        QJsonObject cfg;
        cfg.insert("id", id);
        if (!aLabel.empty())
            cfg.insert("labels", aLabel);
        cfg.insert("images", dst::JArray(id + "/0.png"));
        QJsonObject shps;
        for (int i = 0; i < aPoses.size(); ++i)
        shps.insert(dst::configObject::generateObjectID(),
                    dst::Json("label", QString::number(aLabels[i]),
                              "type", "rectangle",
                              "points", dst::JArray(aPoses[i].x, aPoses[i].y,
                                                    aPoses[i].x + aPoses[i].width, aPoses[i].y + aPoses[i].height)));
        cfg.insert("shapes", shps);

        QFile fl("config_/" + aDir + "/" + id + ".json");
        if (fl.open(QFile::WriteOnly)){
            fl.write(QJsonDocument(cfg).toJson());
            fl.close();
        }
    }
    void savePredictResult2(const cv::Rect& aPos, const QString& aLabel){
        auto id = savePredictResult("supplyCard", m_origin);
        QJsonObject cfg;
        cfg.insert("id", id);
        cfg.insert("images", dst::JArray(id + "/0.png"));
        QJsonObject shps;
        shps.insert(dst::configObject::generateObjectID(),
                    dst::Json("label", aLabel,
                              "type", "rectangle",
                              "points", dst::JArray(aPos.x, aPos.y, aPos.x + aPos.width, aPos.y + aPos.height)));
        cfg.insert("shapes", shps);

        QFile fl("config_/supplyCard/" + id + ".json");
        if (fl.open(QFile::WriteOnly)){
            fl.write(QJsonDocument(cfg).toJson());
            fl.close();
        }
    }
public:
    int m_tick = 0;
    myTurnScene() : scene(){
        loadFeatureImage("myTurn", m_button);
        loadFeaturePos("myTurn", m_loc);
        loadFeaturePos("cardPlace", m_card_place);

        loadFeaturePos("myCountFeature", m_my_count_feature);
        loadFeaturePos("enemyCountFeature", m_enemy_count_feature);
        for (int i = 0; i < 7; ++i)
            for (int j = 0; j < i + 1; ++j){
                loadFeaturePos("attendant_pos/" + QString::number(i) + "_" + QString::number(j), m_attendant_pos[i][j]);
                auto ret = m_attendant_pos[i][j];
                m_enemy_pos[i][j] = cv::Rect(ret.x, ret.y - m_offset_top, ret.width, ret.height);
            }

        loadFeaturePos("gemPos", m_gem_loc);
        loadFeaturePos("heroPos", m_hero_loc);
        loadFeaturePos("enemyHeroPos", m_enemy_hero_loc);
    }
    double isCurrentScene(const cv::Mat &aScreen, const QImage& aOrigin) override{
        auto ret = calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
        dst::showDstLog("myTurn conf : " + QString::number(ret));

        if (ret == 1.0){
           // aOrigin.save(QString::number(m_tick++) + ".png");
            double max, min;
            cv::Point min_loc, max_loc;
            cv::minMaxLoc(aScreen(m_opt_loc), &min, &max, &min_loc, &max_loc); //198,200,165,136,93;31
            dst::showDstLog("my turn max: " + QString::number(max));
            if (max < 40)
                return 0;
            else
                return ret;
        }else
            return 0;
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        dst::showDstLog("myTurn : ");

        m_cards_model = aCards;

        int card_count = m_cards_model->getCardsCount();

        std::this_thread::sleep_for(std::chrono::milliseconds(3000)); //wait for new supplied card
        captureScreen();

        recognizeAttendants();

        if (card_count < 10){
            std::vector<int> costs;
            std::vector<cv::Rect> poses;
            int cost;
            cv::Rect pos;
            for (int i = 0; i <= card_count; ++i){
                auto pos = m_cards_model->getCardPos(m_cards_model->getCardsCount(), i);
                auto roi = m_screen(pos);
                cost = trainingServer::instance()->recognizeNumber(roi);
                costs.push_back(cost);
                poses.push_back(pos);
            }
            m_cards_model->supplyCard(std::make_shared<card>(card_count, cost));

            savePredictResult2(poses, costs);
        }
        aCards->setGemCount(std::min(10, aCards->getGemCount() + 1));
    }
    bool calcOperation() override{
        //attackEnemy();
        placeCards();
       // dst::showDstLog("before : ");
        TRIG("controlWorld", STMJSON(dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5))));
       // dst::showDstLog("after : ");
       // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        return true;
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
        /*if (m_button.cols > 0 && m_button.rows > 0){
            cv::Mat feature(m_button.rows, m_button.cols, CV_8UC1, cv::Scalar(0, 0, 0));
            for (int i = 0; i < 5; ++i){
                cv::Rect loc;
                loadFeaturePos("gameOver_" + QString::number(i), loc);
                if (loc.width > 0 && loc.height > 0)
                    m_button(loc).clone().copyTo(feature(loc));
            }
            m_button = feature;
            //cv::imshow("hello", m_button);
        }*/
    }
    double isCurrentScene(const cv::Mat& aScreen, const QImage& aOrigin) override{
        return calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc, 40);
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        dst::showDstLog("game over : ");
    }
    bool calcOperation() override{
        TRIG("controlWorld", STMJSON(dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5))));
        return true;
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
                break;
            }
        }
    }
    void updateModel() override{
        if (m_current_scene)
            m_current_scene->updateModel(m_cards);
    }
    bool calcOperation() override {
        if (m_current_scene)
            return m_current_scene->calcOperation();
        else
            return false;
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

