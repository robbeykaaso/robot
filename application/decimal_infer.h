#ifndef APPLICATION_DECIMAL_INFER_H_
#define APPLICATION_DECIMAL_INFER_H_

#include <opencv2/opencv.hpp>
#include "testserver.h"
#include "tiny_dnn/tiny_dnn.h"

void dnnTest();

class trainingServer : public TestServer{
public:
    SINGLENTON(trainingServer)
    void trainGemModel();
    void trainSplitModel();
    int predict(tiny_dnn::network<tiny_dnn::sequential>& aNetwork, const tiny_dnn::vec_t& aROI);
    int predict(tiny_dnn::network<tiny_dnn::sequential>& aNetwork, const tiny_dnn::vec_t& aROI, QJsonArray& aHistogram);
    int recognizeNumber(const cv::Mat& aROI);
    int recognizeCount(const cv::Mat& aROI);
    int recognizeCount34(const cv::Mat& aScreen, const cv::Rect aFeature3_pos[3], const cv::Rect aFeature4_pos[4]);
    int recognizeCount10(const cv::Mat& aScreen, const cv::Rect aFeature_pos[10][10]);
    int recognizeCount7(const cv::Mat& aScreen, const cv::Rect aFeature_pos);
protected:
    void initialize();
private:
    tiny_dnn::network<tiny_dnn::sequential> prepareNetwork(const QString& aName, const QString& aDirectory = "");
    void prepareGemTrainData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
                             std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages);
    void prepareTrainData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
                          std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages,
                          const QString& aRootDirectory, const QStringList& aList, const QString& aModelName);
    void prepareCountTrainData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
                               std::vector<tiny_dnn::vec_t>& aTrainLabels2, std::vector<tiny_dnn::vec_t>& aTestLabels2,
                               std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages,
                               const QJsonArray& aLabelList, const QString& aRootDirectory, const QStringList& aList, const QString& aCount);
    int fillData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
                   std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages);
    int fillData(std::vector<tiny_dnn::vec_t>& aTrainLabels, std::vector<tiny_dnn::vec_t>& aTestLabels,
                 std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages);
    bool tryPrepareJob(const QJsonObject& aRequest);
    tiny_dnn::vec_t prepareGemImage(cv::Mat& aROI);
    tiny_dnn::vec_t prepareSplitImage(cv::Mat& aROI);
    int m_train_epochs = 1;
    tiny_dnn::core::backend_t m_backend_type = tiny_dnn::core::default_engine();
    double m_learning_rate = 1;
    int m_minibatch = 16;
private:
    QString m_project_id;
    QString m_task_id;
    QString m_root = "D:/build-deepinspection-Desktop_Qt_5_12_2_MSVC2015_64bit-Default/deepinspectstorage2";
    QString m_task_name;
    QString m_job_state = "";
    QStringList m_anno_list;
    QStringList m_result_list;
    QVector<QJsonArray> m_result_histogram;
private:
    tiny_dnn::network<tiny_dnn::sequential> getGemNet(){
        if (!m_gem_net_loaded){
            m_gem_net.load("Gem-LeNet-model");
            m_gem_net_loaded = true;
        }
        return m_gem_net;
    }
    tiny_dnn::network<tiny_dnn::sequential> get34Net(){
        if (!m_34_net_loaded){
            m_34_net.load("Count_34-LeNet-model");
            m_34_net_loaded = true;
        }
        return m_34_net;
    }
    tiny_dnn::network<tiny_dnn::sequential> get10Net(){
        if (!m_10_net_loaded){
            m_10_net.load("Count_10-LeNet-model");
            m_10_net_loaded = true;
        }
        return m_10_net;
    }
    tiny_dnn::network<tiny_dnn::sequential> get7Net(){
        if (!m_7_net_loaded){
            m_7_net.load("Count_7-LeNet-model");
            m_7_net_loaded = true;
        }
        return m_7_net;
    }
    tiny_dnn::network<tiny_dnn::sequential> m_gem_net;
    bool m_gem_net_loaded;
    tiny_dnn::network<tiny_dnn::sequential> m_34_net;
    bool m_34_net_loaded;
    tiny_dnn::network<tiny_dnn::sequential> m_10_net;
    bool m_10_net_loaded;
    tiny_dnn::network<tiny_dnn::sequential> m_7_net;
    bool m_7_net_loaded;

    tiny_dnn::network<tiny_dnn::sequential> m_split_net;
    bool m_split_net_loaded;
};

#endif
