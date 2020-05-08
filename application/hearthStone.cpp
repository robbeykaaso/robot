#include "brain.h"
#include "../framework/util.h"
#include "decimal_infer.h"
#include <QJsonDocument>
#include <QDir>
#include <QScreen>
#include <opencv2/opencv.hpp>

//10以上数字识别
//嘲讽怪识别
//战吼定向操作
//定向卡判断
//神经网络优化：扩展组合种类
//神经网络容量上限？
//神经网络优化：训练好的网络以模块形式直接用于其他任务
//已处理：
//0.软件整体结构（与标注工具联动训练，识屏动作流程）
//1.手牌数识别(纯色问题)，手牌消耗识别
//2.场景识别（我的回合，地方回合，游戏结束，第一次选择回合，进入游戏回合，选择卡牌）
//3.怪物数识别，怪物血量和攻击力识别
//4.截图防震动

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

      /*  if (minLocation.x == aInflate && minLocation.y == aInflate){
            aRetPos = cv::Rect(minLocation.x - aInflate + aPos.x, minLocation.y - aInflate + aPos.y, aPos.width, aPos.height);
            return 1;
        }
        else
            return 0;*/

        if ((aMask.cols == 0 && minValue < 0.05) ||
            (aMask.cols > 0 && maxValue > 0.999)){
            cv::Rect target(minLocation.x, minLocation.y, aPos.width, aPos.height), origin(aInflate, aInflate, aPos.width, aPos.height);
            aRetPos = cv::Rect(minLocation.x - aInflate + aPos.x, minLocation.y - aInflate + aPos.y, aPos.width, aPos.height);
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
    bool isGreenBar(const QImage& aImage, const cv::Rect& aPos){
        auto cld = aImage.copy(aPos.x, aPos.y, aPos.width, aPos.height);
        for (int i = 0; i < aPos.width; ++i)
            for (int j = 0; j < aPos.height; ++j){
                if (cld.pixelColor(i, j).green() != 255){
                    return false;
                }
            }
        return true;
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
        ++m_add_times;
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

    void refreshCards(std::vector<cv::Rect>& aPoses, std::vector<int>& aLabels, int aCardCount, const cv::Mat& aScreen){
        for (int i = 0; i < 11; ++i)
            m_cards[i].clear();
        m_cards_count = 0;
        for (int i = 0; i < aCardCount; ++i){
            auto pos = getCardPos(aCardCount - 1, i);
            auto roi = aScreen(pos);
            auto cost = trainingServer::instance()->recognizeNumber(roi);
            addCard(std::make_shared<card>(i, cost));
            aLabels.push_back(cost);
            aPoses.push_back(pos);
        }
    }

    void placeCard(std::shared_ptr<card> aCard) {
        dst::showDstLog("card place: index " + QString::number(aCard->getIndex()) + "; cost " + QString::number(aCard->getCost()));
        getCards(aCard->getCost())->erase(aCard);
        --m_cards_count;
        for (int i = 0; i < 11; ++i)
            for (auto j : m_cards[i]){
                auto idx = j->getIndex();
                if (idx > aCard->getIndex())
                    j->setIndex(idx - 1);
            }
    }
    void resetModel(){
        for (int i = 0; i < 11; ++i)
            m_cards[i].clear();
        m_cards_count = 0;
        m_add_times = 0;
        TRIG("resetGame", STMJSON(QJsonObject()))
    }
    int getAddTimes() {return m_add_times;}
    std::set<std::shared_ptr<card>>* getCards(int aCost) {return &m_cards[aCost];}
private:
    int m_cards_count = 0;
    std::set<std::shared_ptr<card>> m_cards[11];
    cv::Rect m_cards_pos[10][10];
    int m_add_times = 0;
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
        loadFeaturePos("select34", m_split);
    }
    double isCurrentScene(const cv::Mat& aScreen, const QImage& aOrigin) override{
        cv::Rect* tgt = m_loc_3;
        int sum = 3;
        if (!isGreenBar(aOrigin, m_split)){
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
private:
    cv::Mat m_button;
    cv::Mat m_select;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
    cv::Rect m_gem_loc;
    cv::Rect m_hero_loc;
    cv::Rect m_enemy_hero_loc;
    cv::Rect m_select_loc;
    cv::Rect m_select_34;
    cv::Rect m_select_30;
    cv::Rect m_select_40;
    cv::Rect m_loc_3[3];
    cv::Rect m_loc_4[4];
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

    bool debounceCaptureScreen(){
        captureScreen();
        int cnt = 0;
        do{
            auto fst = m_screen(m_gem_loc).clone();
            captureScreen();
            cv::Rect tar;
            auto ret = calcFeatureIOU(m_screen, fst, m_gem_loc, tar);
            //auto snd = m_screen(cv::Rect(m_gem_loc.x + 50, m_gem_loc.y, m_gem_loc.width, m_gem_loc.height)).clone();
            //auto diff = fst - snd;
            //auto ret = memcmp(fst.data, m_screen(m_gem_loc).data, fst.total() * fst.elemSize());
            //auto ret = cv::countNonZero(diff);
            if (ret == 1.0){ //https://blog.csdn.net/lcgwust/article/details/70837586
                return true;
            }
            //fst = snd;
            dst::showDstLog("gem shaking : " + QString::number(ret));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }while(++cnt < 10);  //to avoid the shaking of the screen image
        return false;  //perhaps the game is over
    }

    std::vector<int> getAttendantFeatureIndex(int aIndex){
        std::vector<int> ret;
        switch (aIndex) {
            case 1: ret.push_back(3); break;
            case 2: ret.push_back(2); ret.push_back(3); break;
            case 3: ret.push_back(2); ret.push_back(3); ret.push_back(4); break;
            case 4: ret.push_back(1); ret.push_back(2); ret.push_back(3); ret.push_back(4); break;
            case 5: ret.push_back(1); ret.push_back(2); ret.push_back(3); ret.push_back(4); ret.push_back(5); break;
            case 6: ret.push_back(0); ret.push_back(1); ret.push_back(2); ret.push_back(3); ret.push_back(4); ret.push_back(5); break;
            case 7: ret.push_back(0); ret.push_back(1); ret.push_back(2); ret.push_back(3); ret.push_back(4); ret.push_back(5); ret.push_back(6); break;
        }
        return ret;
    }

    void recognizeAttendants(std::vector<cv::Rect>& aPoses, std::vector<int>& aLabels, QJsonObject& aImageLabels, std::vector<int>& aSneers, int aResult[2]){
        auto dorecog = [&](const cv::Rect& aFeaturePos, cv::Rect aAttendantPoses[7][7], bool aCheckSneer){
            auto idx = trainingServer::instance()->recognizeCount7(m_screen, aFeaturePos);
            auto idxes = getAttendantFeatureIndex(idx);
            auto m = idx % 2 == 0 ? 5 : 6;
            for (auto i : idxes){
                auto pos = aAttendantPoses[m][i];
                aPoses.push_back(pos);
                aLabels.push_back(trainingServer::instance()->recognizeNumber(m_screen(pos)));
                if (aCheckSneer){
                    auto sneer_chk = cv::Rect(pos.x - 0.66 * m_offset_left, 490, m_offset_left * 0.4, 1);
                    double minValue, maxValue;
                    cv::Point minLocation, maxLocation;
                    cv::Point matchLocation;
                    minMaxLoc(m_screen(sneer_chk), &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());
                    if (minValue < 100){
                        dst::showDstLog("sneer index : " + QString::number(i));
                        aSneers.push_back(i);
                    }
                }
                pos.x = pos.x - m_offset_left;
                aPoses.push_back(pos);
                aLabels.push_back(trainingServer::instance()->recognizeNumber(m_screen(pos)));
            }
            return idx;
        };
        auto idx = dorecog(m_my_count_feature, m_attendant_pos, false);
        dst::showDstLog("myTurn myAttendants : " + QString::number(idx));

        auto idx2 = dorecog(m_enemy_count_feature, m_enemy_pos, true);
        dst::showDstLog("myTurn enemyAttendants : " + QString::number(idx2));

        dst::Json(aImageLabels, "custom", QString::number(idx), "enemy", QString::number(idx2));

        aResult[0] = idx; aResult[1] = idx2;
    }

    int recognizeCardCount(){
        QColor red(255, 0, 0), green(0, 255, 0), white(255, 255, 255);
        auto ret = - 1;
        for (int i = 9; i >= 0; --i){
            bool isret = true;
            for (int j = 0; j <= i; ++j){
                int cnt = 0;
                auto rng = m_cards_model->getCardPos(i, j);
                auto cld = m_origin.copy(rng.x, rng.y, rng.width, rng.height);
                for (int l = 0; l < rng.height; l++){
                    for (int m = 0; m < rng.width; m++){
                        auto clr = cld.pixelColor(m, l);
                        if (clr == white || clr == red || clr == green)
                            cnt++;
                    }
                }
                /*auto roi = m_screen(rng);
                for(int l = 0; l < roi.rows; l++){
                   // auto p = roi.ptr<uchar>(l);
                   for(int m = 0; m < roi.cols; m++){
                       if (roi.at<uchar>(l, m) == 255)
                           cnt++;
                   }
                }*/
                //std::cout << cnt << std::endl;
                if (cnt < 90){
                    isret = false;
                    break;
                }
            }
            if (isret){
                ret = i + 1;
                break;
            }
        }
        return ret;
    }

    void supplyCard(std::vector<cv::Rect>& aPoses, std::vector<int>& aLabels){
        int card_count = m_cards_model->getCardsCount();
        if (card_count < 10){
            int cost;
            cv::Rect pos;
            for (int i = 0; i <= card_count; ++i){
                auto pos = m_cards_model->getCardPos(m_cards_model->getCardsCount(), i);
                auto roi = m_screen(pos);
                cost = trainingServer::instance()->recognizeNumber(roi);
                aLabels.push_back(cost);
                aPoses.push_back(pos);
            }
            m_cards_model->supplyCard(std::make_shared<card>(card_count, cost));
        }
    }

    int recognizeGemCount(std::vector<cv::Rect>& aPoses, std::vector<int>& aLabels, const QJsonObject& aImageLabels = QJsonObject()){
        int gem_count = trainingServer::instance()->recognizeNumber(m_screen(m_gem_loc));
        dst::showDstLog("myTurn GemCount : " + QString::number(gem_count));
        aPoses.push_back(m_gem_loc);
        aLabels.push_back(gem_count);
        savePredictResult2(aPoses, aLabels, aImageLabels, "myTurn");
        return gem_count;
    }
private:
    void play(){
        int cnt[2];
        std::vector<int> lbls, sneers;
        std::vector<cv::Rect> poses;
        QJsonObject img_lbls;

        do{
            recognizeAttendants(poses, lbls, img_lbls, sneers, cnt);
            savePredictResult2(poses, lbls, img_lbls, "myTurn");
            if (sneers.size() > 0 && cnt[0] > 0){
                auto my = cnt[0] % 2 == 0 ? 5 : 6, tar = cnt[1] % 2 == 0 ? 5 : 6;
                auto st_x = m_attendant_pos[my][0].x + m_attendant_pos[my][0].width * 0.5,
                     st_y = m_attendant_pos[my][0].y + m_attendant_pos[my][0].height * 0.5,
                     ed_x = m_attendant_pos[tar][sneers[0]].x + m_attendant_pos[tar][sneers[0]].width * 0.5,
                     ed_y = m_attendant_pos[tar][sneers[0]].y + m_attendant_pos[tar][sneers[0]].height * 0.5;
                TRIG("controlWorld", STMJSON(dst::Json("type", "drag",
                                                       "org", dst::JArray(st_x, st_y),
                                                       "del", dst::JArray(ed_x - st_x, ed_y - st_y))));
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                captureScreen();
                poses.clear();lbls.clear();sneers.clear();
            }else{
                auto idxes = getAttendantFeatureIndex(cnt[0]);
                if (m_enemy_hero_loc.width > 0 && m_enemy_hero_loc.height > 0)
                    for (int i = 0; i < idxes.size() * 2; i += 2){
                        auto st_x = poses[i].x + poses[i].width * 0.5, st_y = poses[i].y + poses[i].height * 0.5;
                        if (lbls[i + 1] > 0)
                            TRIG("controlWorld", STMJSON(dst::Json("type", "drag",
                                                                   "org", dst::JArray(st_x, st_y),
                                                                   "del", dst::JArray(m_enemy_hero_loc.x + m_enemy_hero_loc.width * 0.5 - st_x,
                                                                                      m_enemy_hero_loc.y + m_enemy_hero_loc.height * 0.5 - st_y))));
                    }
                break;
            }
        }while(1);

        //supplyCard(poses, lbls);
        //auto cardmodel = supplyCard2(poses, lbls, card_count);
        int gem_count = recognizeGemCount(poses, lbls, img_lbls);

        std::set<std::shared_ptr<card>> used;
        int i = - 1;
        do{
            auto higher_cost = std::min(gem_count, 10);
            i = higher_cost;
            auto new_gem_count = gem_count;

            poses.clear();lbls.clear();
            int card_count = recognizeCardCount();
            m_cards_model->refreshCards(poses, lbls, card_count, m_screen);
            savePredictResult2(poses, lbls, dst::Json("card", QString::number(card_count)), "myTurn");

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
                    if (card->getCost() == 2 || card->getCost() == 4)
                        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
                    else
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    if (!debounceCaptureScreen())
                        return;
                    checkSelectScene();

                    poses.clear();lbls.clear();
                    new_gem_count = recognizeGemCount(poses, lbls);

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

        if (gem_count > 1){
            TRIG("controlWorld", STMJSON(dst::Json("type", "click",
                                                   "org", dst::JArray(m_hero_loc.x + m_hero_loc.width * 0.5, m_hero_loc.y + m_hero_loc.height * 0.5))))
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //perhaps it will cause shaking
        }
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
public:
    int m_tick = 0;
    myTurnScene() : scene(){
        loadFeatureImage("myTurn", m_button);
        loadFeaturePos("myTurn", m_loc);
        loadFeatureImage("normalSelect", m_select);
        loadFeaturePos("normalSelect", m_select_loc);
        loadFeaturePos("normalSelect34", m_select_34);
        loadFeaturePos("normalSelect30", m_select_30);
        loadFeaturePos("normalSelect40", m_select_40);
        for (int i = 0; i < 3; ++i)
            loadFeaturePos("n_select3_" + QString::number(i), m_loc_3[i]);
        for (int i = 0; i < 4; ++i)
            loadFeaturePos("n_select4_" + QString::number(i), m_loc_4[i]);
        loadFeaturePos("cardPlace", m_card_place);

        loadFeaturePos("myCountFeature", m_my_count_feature);
        loadFeaturePos("enemyCountFeature", m_enemy_count_feature);
        for (int i = 5; i < 7; ++i)
            for (int j = 0; j < i + 1; ++j){
                loadFeaturePos("attendant_pos/" + QString::number(i) + "_" + QString::number(j), m_attendant_pos[i][j]);
                auto ret = m_attendant_pos[i][j];
                m_enemy_pos[i][j] = cv::Rect(ret.x, ret.y - m_offset_top, ret.width, ret.height);
            }

        loadFeaturePos("gemPos", m_gem_loc);
        loadFeaturePos("heroPos", m_hero_loc);
        loadFeaturePos("enemyHeroPos", m_enemy_hero_loc);

        dst::streamManager::instance()->registerEvent("unitTest", "mdyMyTurn4", [this](std::shared_ptr<dst::streamData> aInput){
            QJsonObject info;
            dst::configObject::loadJsonFileConfig("unittestInfo", info);
            if (info.value("myturn_attendant").toBool()){
                auto lst = info.value("myturn_attendant_list").toArray();
                for (auto i : lst){
                    m_origin = QImage(i.toString());
                    m_screen = QImage2cvMat(m_origin);
                    cv::cvtColor(m_screen, m_screen, cv::COLOR_RGB2GRAY);
                    int cnt[2];
                    std::vector<int> lbls, sneers;
                    std::vector<cv::Rect> poses;
                    QJsonObject img_lbls;
                    recognizeAttendants(poses, lbls, img_lbls, sneers, cnt);
                }
            }
            return aInput;
        });

        dst::streamManager::instance()->registerEvent("unitTest", "mdyMyTurn3", [this](std::shared_ptr<dst::streamData> aInput){
            captureScreen();
            //m_origin = QImage("D:/deepsight/deepinspectstorage2/image/1588643072-DAE40344-E9C7-46b5-88BE-FA5F8B6F884F/0.png");
            //m_screen = QImage2cvMat(m_origin);
           // cv::cvtColor(m_screen, m_screen, cv::COLOR_RGB2GRAY);
            checkSelectScene();
            return aInput;
        });

        dst::streamManager::instance()->registerEvent("unitTest", "mdyMyTurn2", [this](std::shared_ptr<dst::streamData> aInput){
            QJsonObject info;
            dst::configObject::loadJsonFileConfig("unittestInfo", info);
            if (info.value("myturn").toBool()){
                auto lst = info.value("myturn_list").toArray();
                for (auto i : lst){
                    auto src = QImage(i.toString());
                    auto src0 = QImage2cvMat(src);
                    cv::cvtColor(src0, src0, cv::COLOR_RGB2GRAY);
                    isCurrentScene(src0, src);
                }
            }
            return aInput;
        });

        dst::streamManager::instance()->registerEvent("unitTest", "mdyMyTurn", [this](std::shared_ptr<dst::streamData> aInput){
            QJsonObject info;
            dst::configObject::loadJsonFileConfig("unittestInfo", info);
            if (info.value("card_count").toBool()){
                auto lst = info.value("card_count_list").toArray();
                for (auto i : lst){
                    m_origin = QImage(i.toString());
                    m_screen = QImage2cvMat(m_origin);
                    cv::cvtColor(m_screen, m_screen, cv::COLOR_RGB2GRAY);
                    m_cards_model = std::make_shared<cardsModel>();
                    auto test = recognizeCardCount();
                    std::cout << test << std::endl;
                }
            }
            return aInput;
        });
    }

    void checkSelectScene(){
        auto src = m_screen(cv::Rect(m_select_loc.x - 5, m_select_loc.y - 5, m_select_loc.width + 2 * 5, m_select_loc.height + 2 * 5));
        cv::Mat tmp1, tmp2, tmp3;
        threshold(src, tmp1, 200, 255, cv::THRESH_BINARY);
        threshold(m_select, tmp2, 200, 255, cv::THRESH_BINARY);
        cv::matchTemplate(tmp1, tmp2, tmp3, cv::TemplateMatchModes::TM_CCORR_NORMED);

        double minValue, maxValue;
        cv::Point minLocation, maxLocation;
        cv::Point matchLocation;
        minMaxLoc(tmp3, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());

        /*cv::imshow("hello1", tmp2);
        cv::moveWindow("hello1", 500, 0);
        cv::imshow("hello2", tmp1);
        cv::moveWindow("hello2", 600, 0);*/

       // std::cout << maxValue << std::endl;
        dst::showDstLog("myTurn select conf : " + QString::number(maxValue));

        if (maxValue > 0.9){
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            captureScreen(); //wait for gold coins over...

            int cnt = 3;
            cv::Rect* tar_locs = m_loc_3;
            if (!isGreenBar(m_origin, m_select_34)){
                cnt = 4;
                tar_locs = m_loc_4;
            }
            int st = 0, ed = cnt;
            if (cnt == 3 && !isGreenBar(m_origin, m_select_30)){
                st = 1; ed = 2;
            }else if (cnt == 4 && !isGreenBar(m_origin, m_select_40)){
                st = 1; ed = 3;
            }

            std::vector<cv::Rect> poses;
            std::vector<int> lbls;
            int mn_cost = - 1, sel = - 1;
            for (int i = st; i < ed; ++i){
                auto roi = m_screen(tar_locs[i]);
                auto cost = trainingServer::instance()->recognizeNumber(roi);
                poses.push_back(tar_locs[i]);
                lbls.push_back(cost);
                if (cost > mn_cost){
                    mn_cost = cost;
                    sel = i;
                }
            }
            TRIG("controlWorld", STMJSON(dst::Json("type", "click",
                                                   "org", dst::JArray(tar_locs[sel].x + tar_locs[sel].width * 0.5, tar_locs[sel].y + tar_locs[sel].height * 0.5))))
            savePredictResult2(poses, lbls, QJsonObject(), "selectScene");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            captureScreen();
        }

    }

    double isCurrentScene(const cv::Mat &aScreen, const QImage& aOrigin) override{
       /* auto ret = calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc);
        dst::showDstLog("myTurn conf : " + QString::number(ret));
        if (ret == 0.0){
            cv::Rect tmp;
            ret = calcFeatureIOU(aScreen, m_button2, m_loc2, m_opt_loc);
            dst::showDstLog("myTurn conf2 : " + QString::number(ret));
        }*/
        auto src = aScreen(cv::Rect(m_loc.x - 5, m_loc.y - 5, m_loc.width + 2 * 5, m_loc.height + 2 * 5)).clone();
        cv::Mat tmp1, tmp2, tmp3;
//
        threshold(255 - src, tmp1, 220, 255, cv::THRESH_BINARY);
        threshold(255 - m_button, tmp2, 220, 255, cv::THRESH_BINARY);

        cv::matchTemplate(tmp1, tmp2, tmp3, cv::TemplateMatchModes::TM_CCORR_NORMED);

        double minValue, maxValue;
        cv::Point minLocation, maxLocation;
        cv::Point matchLocation;
        minMaxLoc(tmp3, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());
        std::cout << maxValue << std::endl;
        //dst::showDstLog("myTurn conf : " + QString::number(maxValue));

        if (maxValue > 0.85){
            m_opt_loc = m_loc;
            return 1;
        }else
            return 0;
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        dst::showDstLog("myTurn : ");
        m_cards_model = aCards;
        if (m_cards_model->getAddTimes() == 3)
            std::this_thread::sleep_for(std::chrono::milliseconds(4000)); //wait for new supplied card, need optimization
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        debounceCaptureScreen();
    }
    bool calcOperation() override{
        play();
       // dst::showDstLog("before : ");
        TRIG("controlWorld", STMJSON(dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5))))
       // dst::showDstLog("after : ");
        return true;
    }
};

/*class enemyTurnScene : public scene{
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
};*/

class gameOverScene : public scene{
private:
    cv::Mat m_button;
    cv::Rect m_loc;
    cv::Rect m_opt_loc;
public:
    gameOverScene() : scene(){
        loadFeaturePos("gameOver", m_loc);
        loadFeatureImage("gameOver", m_button);

        dst::streamManager::instance()->registerEvent("unitTest", "mdyGameOver", [this](std::shared_ptr<dst::streamData> aInput){
            QJsonObject info;
            dst::configObject::loadJsonFileConfig("unittestInfo", info);
            if (info.value("gameover").toBool()){
                auto lst = info.value("gameover_list").toArray();
                for (auto i : lst){
                    auto src = QImage(i.toString());
                    auto src0 = QImage2cvMat(src);
                    cv::cvtColor(src0, src0, cv::COLOR_RGB2GRAY);
                    isCurrentScene(src0, src);
                }
            }
            return aInput;
        });
        //threshold(m_button, m_button, 200, 255, cv::THRESH_BINARY);
        //cv::imshow("hello", m_button);
        /*if (m_button.cols > 0 && m_button.rows > 0){
            cv::Mat feature(m_button.rows, m_button.cols, CV_8UC1, cv::Scalar(0, 0, 0));
            for (int i = 0; i < 5; ++i){
                cv::Rect loc;
                loadFeaturePos("gameOver_" + QString::number(i), loc);
                if (loc.width > 0 && loc.height > 0)
                    m_button(loc).clone().copyTo(feature(loc));
            }
            m_button = feature;
            //
        }*/
    }
    double isCurrentScene(const cv::Mat& aScreen, const QImage& aOrigin) override{
        /*int tmp = 0;
        auto ret = calcFeatureIOU(aScreen, m_button, m_loc, m_opt_loc, 100, cv::Mat(), [&](const cv::Mat& aImage){
            cv::Mat ret;
            normalize(aImage, ret, 0, 1, cv::NORM_MINMAX);

            auto nm = QString::number(tmp++).toStdString();
            cv::imshow("hello" + nm, aImage);
            cv::moveWindow("hello" + nm, 300, 0);

            return ret;
        });*/
        //auto ret = 0;

        auto src = aScreen(cv::Rect(m_loc.x - 40, m_loc.y - 40, m_loc.width + 2 * 40, m_loc.height + 2 * 40));
        cv::Mat tmp1, tmp2, tmp3;
        threshold(src, tmp1, 200, 255, cv::THRESH_BINARY);
        threshold(m_button, tmp2, 200, 255, cv::THRESH_BINARY);

        //cv::imshow("hello", tmp1);
        //cv::moveWindow("hello", 150, 0);
        //cv::imshow("hello2", tmp2);
        //cv::moveWindow("hello2", 300, 0);
        //normalize(m_button, tmp1, 0, 1, cv::NORM_MINMAX);
       // normalize(src, tmp2, 0, 1, cv::NORM_MINMAX);
        cv::matchTemplate(tmp1, tmp2, tmp3, cv::TemplateMatchModes::TM_CCORR_NORMED);

        double minValue, maxValue;
        cv::Point minLocation, maxLocation;
        cv::Point matchLocation;
        minMaxLoc(tmp3, &minValue, &maxValue, &minLocation, &maxLocation, cv::Mat());

        //std::cout << minValue << ";" << maxValue << std::endl;
        //dst::showDstLog("game over conf: " + QString::number(minValue) + ";" + QString::number(maxValue));
        if (maxValue > 0.8){
            //savePredictResult("gaveOver", aOrigin, QString::number(0));
            m_opt_loc = m_loc;
            return 1;
        }
        else
            return 0;
    }
    void updateModel(std::shared_ptr<cardsModel> aCards) override{
        dst::showDstLog("game over : ");
    }
    bool calcOperation() override{
        TRIG("controlWorld", STMJSON(dst::Json("type", "click", "org", dst::JArray(m_opt_loc.x + m_opt_loc.width * 0.5, m_opt_loc.y + m_opt_loc.height * 0.5))))
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
       // m_scenes.push_back(std::make_shared<enemyTurnScene>());
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

