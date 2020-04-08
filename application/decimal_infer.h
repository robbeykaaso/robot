#ifndef APPLICATION_DECIMAL_INFER_H_
#define APPLICATION_DECIMAL_INFER_H_

#include <opencv2/opencv.hpp>
#include "testserver.h"
#include "tiny_dnn/tiny_dnn.h"

int recognizeNumber(const cv::Mat& aROI);
void dnnTest();

class trainingServer : public TestServer{
public:
    SINGLENTON(trainingServer)
    void trainGemModel();
protected:
    void initialize();
private:
    tiny_dnn::network<tiny_dnn::sequential> prepareNetwork();
    void prepareGemTrainData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
                             std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages);
    void prepareTrainData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
                          std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages,
                          const QString& aRootDirectory, const QStringList& aList);
    int m_train_epochs = 4;
    tiny_dnn::core::backend_t m_backend_type = tiny_dnn::core::default_engine();
    double m_learning_rate = 1;
    int m_minibatch = 16;
private:
    QString m_project_id;
    QString m_task_id;
    QString m_root = "D:/build-deepinspection-Desktop_Qt_5_12_2_MSVC2015_64bit-Default/deepinspectstorage2";
    QString m_job_state = "";
    QStringList m_anno_list;
    QStringList m_result_list;
};

#endif
