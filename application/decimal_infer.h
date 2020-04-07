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
                             std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages, const QString& aRoot = "config_/hearthStone/");
    int m_train_epochs = 4;
    tiny_dnn::core::backend_t m_backend_type = tiny_dnn::core::default_engine();
    double m_learning_rate = 1;
    int m_minibatch = 16;
};

#endif
