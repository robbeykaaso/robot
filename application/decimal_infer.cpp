#include "decimal_infer.h"
#include "tiny_dnn/util/image.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

//sample train

void construct_net(tiny_dnn::network<tiny_dnn::sequential> &nn,
                          tiny_dnn::core::backend_t backend_type) {
// connection table [Y.Lecun, 1998 Table.1]
#define O true
#define X false
    // clang-format off
static const bool tbl[] = {
    O, X, X, X, O, O, O, X, X, O, O, O, O, X, O, O,
    O, O, X, X, X, O, O, O, X, X, O, O, O, O, X, O,
    O, O, O, X, X, X, O, O, O, X, X, O, X, O, O, O,
    X, O, O, O, X, X, O, O, O, O, X, X, O, X, O, O,
    X, X, O, O, O, X, X, O, O, O, O, X, O, O, X, O,
    X, X, X, O, O, O, X, X, O, O, O, O, X, O, O, O
};
// clang-format on
#undef O
#undef X

    // construct nets
    //
    // C : convolution
    // S : sub-sampling
    // F : fully connected
    // clang-format off
  using fc = tiny_dnn::layers::fc;
  using conv = tiny_dnn::layers::conv;
  using ave_pool = tiny_dnn::layers::ave_pool;
  using tanh = tiny_dnn::activation::tanh;

  using tiny_dnn::core::connection_table;
  using padding = tiny_dnn::padding;

  nn << conv(32, 32, 5, 1, 6,   // C1, 1@32x32-in, 6@28x28-out
             padding::valid, true, 1, 1, 1, 1, backend_type)
     << tanh()
     << ave_pool(28, 28, 6, 2)   // S2, 6@28x28-in, 6@14x14-out
     << tanh()
     << conv(14, 14, 5, 6, 16,   // C3, 6@14x14-in, 16@10x10-out
             connection_table(tbl, 6, 16),
             padding::valid, true, 1, 1, 1, 1, backend_type)
     << tanh()
     << ave_pool(10, 10, 16, 2)  // S4, 16@10x10-in, 16@5x5-out
     << tanh()
     << conv(5, 5, 5, 16, 120,   // C5, 16@5x5-in, 120@1x1-out
             padding::valid, true, 1, 1, 1, 1, backend_type)
     << tanh()
     << fc(120, 11, true, backend_type)  // F6, 120-in, 12-out
     << tanh();
}

void train_lenet(const std::string &data_dir_path,
                        double learning_rate,
                        const int n_train_epochs,
                        const int n_minibatch,
                        tiny_dnn::core::backend_t backend_type) {
  // specify loss-function and learning strategy
  tiny_dnn::network<tiny_dnn::sequential> nn;
  tiny_dnn::adagrad optimizer;

  construct_net(nn, backend_type);

  std::cout << "load models..." << std::endl;

  // load MNIST dataset
  std::vector<tiny_dnn::label_t> train_labels, test_labels;
  std::vector<tiny_dnn::vec_t> train_images, test_images;

  tiny_dnn::parse_mnist_labels(data_dir_path + "/train-labels.idx1-ubyte",
                               &train_labels);
  tiny_dnn::parse_mnist_images(data_dir_path + "/train-images.idx3-ubyte",
                               &train_images, -1.0, 1.0, 2, 2);
  tiny_dnn::parse_mnist_labels(data_dir_path + "/t10k-labels.idx1-ubyte",
                               &test_labels);
  tiny_dnn::parse_mnist_images(data_dir_path + "/t10k-images.idx3-ubyte",
                               &test_images, -1.0, 1.0, 2, 2);

  std::cout << "start training" << std::endl;

  tiny_dnn::progress_display disp(train_images.size());
  tiny_dnn::timer t;

  optimizer.alpha *=
    std::min(tiny_dnn::float_t(4),
             static_cast<tiny_dnn::float_t>(sqrt(n_minibatch) * learning_rate));

  int epoch = 1;
  // create callback
  auto on_enumerate_epoch = [&]() {
    std::cout << "Epoch " << epoch << "/" << n_train_epochs << " finished. "
              << t.elapsed() << "s elapsed." << std::endl;
    ++epoch;
    tiny_dnn::result res = nn.test(test_images, test_labels);
    std::cout << res.num_success << "/" << res.num_total << std::endl;

    disp.restart(train_images.size());
    t.restart();
  };

  auto on_enumerate_minibatch = [&]() { disp += n_minibatch; };

  // training
  nn.train<tiny_dnn::mse>(optimizer, train_images, train_labels, n_minibatch,
                          n_train_epochs, on_enumerate_minibatch,
                          on_enumerate_epoch);

  std::cout << "end training." << std::endl;

  // test and show results
  nn.test(test_images, test_labels).print_detail(std::cout);
  // save network model & trained weights
  nn.save("LeNet-model");
}

//sample infer

template <typename Activation>
double rescale(double x) {
    Activation a(1);
    return 100.0 * (x - a.scale().first) / (a.scale().second - a.scale().first);
}

void convert_image(const std::string &imagefilename,
                   double minv,
                   double maxv,
                   int w,
                   int h,
                   tiny_dnn::vec_t &data) {
    tiny_dnn::image<> img(imagefilename, tiny_dnn::image_type::grayscale);
    tiny_dnn::image<> resized = resize_image(img, w, h);

    // mnist dataset is "white on black", so negate required
    std::transform(
        resized.begin(), resized.end(), std::back_inserter(data),
        [=](uint8_t c) { return (255 - c) * (maxv - minv) / 255.0 + minv; });
}

void recognize(const std::string &dictionary, const std::string &src_filename) {
    tiny_dnn::network<tiny_dnn::sequential> nn;

    nn.load(dictionary);

    // convert imagefile to vec_t
    tiny_dnn::vec_t data;
    convert_image(src_filename, -1.0, 1.0, 32, 32, data);

    // recognize
    auto res = nn.predict(data);
    std::vector<std::pair<double, int>> scores;

    // sort & print top-3
    for (int i = 0; i < 10; i++)
        scores.emplace_back(rescale<tiny_dnn::tanh_layer>(res[i]), i);

    sort(scores.begin(), scores.end(), std::greater<std::pair<double, int>>());

    for (int i = 0; i < 3; i++)
        std::cout << scores[i].second << "," << scores[i].first << std::endl;
}

//my train

//粗定位网络

//原图:1920 * 1017
//目标:111 * 152

//肉眼缩小5倍仍能识别定位：
//原图: 384 * 203
//目标: 22 * 30
//输出为结果为输出图像上的一点

//第一部分：局部特征提取并拟合
//第一层：提取椭圆轮廓对称6点局部纹理，由于轮廓较窄，宽度在缩小5倍后约1像素，故第一层卷积核为1 * 1 * 6 -> 原图: 384 * 203 * 6 目标： 22 * 30 * 6
//第二部分：局部特征相对位置特征提取并拟合，目的将目标通过卷积描述的相对位置特征缩成一点，该点为最终变换原图中的位置，故需要将目标尺寸通过变换缩到5*5或3*3以减少最后一步卷积的计算量；
//考虑池化，如果池化一次步长太大，那么可能直接导致原图识别关键信息损失，所以采用多步池化
//第二层：max_pool stride_x:2 stride_y:3 -> 原图：192 * 68 * 6 目标： 11 * 10 * 6
//第三层：max_pool stride_x:2 stride_y:5 -> 原图：96 * 34 * 6 目标：5 * 5 * 6
//第四层：卷积核 5 * 5 * 6 -> 92 * 30 目标：1
//如果追加全连接层转码成坐标位置，由于连接数过大，会导致计算过慢，所以这边输出直接为图上一点
//输入：一张原图缩小到 384 * 203 的图片，输出：92 * 30的图片，其中某一点为1，表示定位目标位置

void construct_detect_net1(tiny_dnn::network<tiny_dnn::sequential> &nn,
                          tiny_dnn::core::backend_t backend_type) {
  using fc = tiny_dnn::layers::fc;
  using conv = tiny_dnn::layers::conv;
  using max_pool = tiny_dnn::layers::max_pool;
  using tanh = tiny_dnn::activation::tanh;

  using tiny_dnn::core::connection_table;
  using padding = tiny_dnn::padding;

  nn << conv(384, 203, 1, 1, 6,
             padding::valid, true, 1, 1, 1, 1, backend_type)
     << tanh()
     << max_pool(384, 203, 6, 2, 3, 2, 3)
     << tanh()
     << max_pool(192, 68, 6, 2, 2, 2, 2)
     << tanh()
     << conv(96, 34, 5, 6, 1, padding::valid, true, 1, 1, 1, 1, backend_type)
     << tanh();
}

void trainDetect1Model(){

}

//细定位网络

//由于粗定位网络，因为缩放，所以最终结果存在定位误差ceil(1920 / 92 = 21), ceil(1017 / 30 = 34)
//所以细定位网络输入原始图像应为 (111 + 2 * 21) * (152 + 2 * 34) = 153 * 220（从原图截取的指定尺寸目标框），
//目标图尺寸仍为:111 * 152
//输出结果为4个坐标（不能为图像上的一点，因为池化过程会损失定位精度，但目的仍然是让目标缩成一点，然后将结果输出图全卷积为四个坐标）
//第一部分：局部特征提取并拟合
//第一层：提取椭圆轮廓对称6点局部纹理，卷积核为 3 * 3 * 6 -> 原图: 151 * 218 * 6 目标: 109 * 150
//第二部分：提取6个位置的相对位置信息
//第二层：max_pool stride_x 4 stride_y 5 -> 原图: 38 * 44 * 6 目标: 28 * 30
//第三层: conv  3 * 3 * 2 -> 36 * 42 * 12 目标: 26 * 28
//第四层: max_pool stride_x 4 stride_y 4 -> 原图: 9 * 10 * 12 目标: 6 * 7
//第五层: 全连接 输出 4个位置


void trainDetect2Model(){

}

tiny_dnn::network<tiny_dnn::sequential> trainingServer::prepareNetwork(){
    tiny_dnn::network<tiny_dnn::sequential> nn;
    if (QDir().exists("Gem-LeNet-model")){
        //nn.load("Gem-LeNet-model", tiny_dnn::content_type::weights_and_model, tiny_dnn::file_format::json);
        nn.load("Gem-LeNet-model");
        std::cout << "load models..." << std::endl;
    }
    else
        construct_net(nn, m_backend_type);
    return nn;
}

void trainingServer::prepareTrainData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
    std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages, const QString& aRootDirectory, const QStringList& aList){
    for (auto i : aList)
        if (i != "." && i != ".."){
            QFile fl(aRootDirectory + "/imageInfo/" + i);
            fl.open(QFile::ReadOnly);
            auto cfg = QJsonDocument::fromJson(fl.readAll()).object();
            auto img_pth = aRootDirectory + "/image/" + cfg.value("id").toString() + "/" + cfg.value("source").toArray()[0].toString();
            cv::Mat bak = cv::imread(img_pth.toLocal8Bit().toStdString());
            auto shps = cfg.value("shapes").toObject();
            for (auto i : shps.keys()){
                auto shp = shps.value(i).toObject();
                if (shp.value("type") == "rectangle"){
                    auto pts = shp.value("points").toArray();
                    auto rect = cv::Rect(pts[0].toInt(), pts[1].toInt(), pts[2].toInt() - pts[0].toInt(), pts[3].toInt() - pts[1].toInt());
                    cv::Mat img = bak(rect);
                    cv::cvtColor(img, img, cv::COLOR_RGB2GRAY);
                    cv::resize(img, img, cv::Size(32, 32));
                    /*cv::namedWindow("hello",0);
                    cv::moveWindow("hello", 300, 0);
                    cv::imshow("hello", img);*/
                    tiny_dnn::vec_t ret(img.cols * img.rows);
                    for (int i = 0; i < img.rows; i++){
                        auto ptr = img.ptr(i);
                        for (int j = 0; j < img.cols; j++)
                            ret[i * img.cols + j] = (255 - ptr[j]) / float_t(255) * 2 - 1;
                    }
                    aTrainImages.push_back(ret);
                    aTestImages.push_back(ret);
                    aTrainLabels.push_back(shp.value("label").toString().toInt());
                    aTestLabels.push_back(shp.value("label").toString().toInt());
                }
            }
            fl.close();
        }
}

void trainingServer::prepareGemTrainData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
                                         std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages){

    QString rel_dir = "config_/hearthStone/imageInfo";
    QDir dir(rel_dir);
    prepareTrainData(aTrainLabels, aTestLabels, aTrainImages, aTestImages, "config_/hearthStone", dir.entryList());

    int sz = aTrainImages.size();
    srand( (unsigned)time(NULL));
    for (int i = sz; i < 45000; ++i){
        auto idx = std::rand() % sz;
        aTrainLabels.push_back(aTrainLabels.at(idx));
        aTrainImages.push_back(aTrainImages.at(idx));
    }
    for (int i = sz; i < 15000; ++i){
        auto idx = std::rand() % sz;
        aTestLabels.push_back(aTrainLabels.at(idx));
        aTestImages.push_back(aTrainImages.at(idx));
    }
}

void trainingServer::trainGemModel(){
  auto nn = prepareNetwork();

  std::vector<tiny_dnn::label_t> train_labels, test_labels;
  std::vector<tiny_dnn::vec_t> train_images, test_images;
  prepareGemTrainData(train_labels, test_labels, train_images, test_images);

  tiny_dnn::adagrad optimizer;
  std::cout << "start training" << std::endl;
  tiny_dnn::progress_display disp(train_images.size());
  tiny_dnn::timer t;
  optimizer.alpha *=
    std::min(tiny_dnn::float_t(4),
             static_cast<tiny_dnn::float_t>(sqrt(m_minibatch) * m_learning_rate));

  int epoch = 1;
  auto on_enumerate_epoch = [&]() {
    std::cout << "Epoch " << epoch << "/" << m_train_epochs << " finished. "
              << t.elapsed() << "s elapsed." << std::endl;
    ++epoch;
    tiny_dnn::result res = nn.test(test_images, test_labels);
    std::cout << res.num_success << "/" << res.num_total << std::endl;

    disp.restart(train_images.size());
    t.restart();
  };

  auto on_enumerate_minibatch = [&]() { disp += m_minibatch; };
  nn.train<tiny_dnn::mse>(optimizer, train_images, train_labels, m_minibatch,
                          m_train_epochs, on_enumerate_minibatch,
                          on_enumerate_epoch);
  std::cout << "end training." << std::endl;

  auto ret = nn.test(test_images, test_labels);
  ret.print_detail(std::cout);
  nn.save("Gem-LeNet-model");
  //nn.save("Gem-LeNet-model", tiny_dnn::content_type::weights_and_model, tiny_dnn::file_format::json);
}

//my infer

void dnnTest(){
    tiny_dnn::network<tiny_dnn::sequential> nn;
    nn.load("test-model", tiny_dnn::content_type::weights_and_model, tiny_dnn::file_format::json);
    tiny_dnn::vec_t data;
    data.push_back(1);
    data.push_back(0);
    data.push_back(0);
    data.push_back(0);
    data.push_back(1);
    data.push_back(0);
    data.push_back(0);
    data.push_back(0);
    data.push_back(1);
    auto res = nn.predict(data);
    int test = 0;
}

void convert_image(const cv::Mat& aImage,
                   double minv,
                   double maxv,
                   int w,
                   int h,
                   tiny_dnn::vec_t &data) {
    tiny_dnn::image<> img(aImage.data, w, h, tiny_dnn::image_type::grayscale);
    tiny_dnn::image<> resized = resize_image(img, w, h);

    // mnist dataset is "white on black", so negate required
    std::transform(
        resized.begin(), resized.end(), std::back_inserter(data),
        [=](uint8_t c) { return (255 - c) * (maxv - minv) / 255.0 + minv; });
}

int recognizeNumber(tiny_dnn::network<tiny_dnn::sequential>& aNetwork, const tiny_dnn::vec_t& aROI){
    auto res = aNetwork.predict(aROI);
    std::vector<std::pair<double, int>> scores;
    for (int i = 0; i < 11; i++)
        scores.emplace_back(rescale<tiny_dnn::tanh_layer>(res[i]), i);
    sort(scores.begin(), scores.end(), std::greater<std::pair<double, int>>());
    return scores[0].second;
}

int recognizeNumber(const cv::Mat& aROI){
    tiny_dnn::network<tiny_dnn::sequential> nn;
    //nn.load("Gem-LeNet-model", tiny_dnn::content_type::weights_and_model, tiny_dnn::file_format::json);
    nn.load("Gem-LeNet-model");
    tiny_dnn::vec_t data;
    convert_image(aROI, -1.0, 1.0, 32, 32, data);
    return recognizeNumber(nn, data);
}

void trainingServer::initialize(){
    dst::streamManager::instance()->registerEvent("training", "mdysev", [this](std::shared_ptr<dst::streamData> aInput){
        auto cfg = reinterpret_cast<dst::streamJson*>(aInput.get())->getData();
        if (m_job_state != ""){
            Send(QJsonDocument(dst::Json(
                                   "id", cfg->value("id"),
                                   "type", cfg->value("type"),
                                   "state", "begin",
                                   "err_code", 1,
                                   "mgs", "")).toJson(QJsonDocument::Compact).toStdString());
            return aInput;
        }

        m_project_id = cfg->value("project_id").toString();
        m_task_id = cfg->value("task_id").toString();
        m_job_state = "running";
        Send(QJsonDocument(dst::Json(
            "id", cfg->value("id"),
            "type", cfg->value("type"),
            "job_id", cfg->value("id"),
            "state", "begin",
            "err_code", 0,
            "mgs", "")).toJson(QJsonDocument::Compact).toStdString());

        auto dt = cfg->value("data").toObject();
        m_root = dt.value("s3_bucket_name").toString();
        auto lst = dt.value("uuid_list").toArray();
        m_anno_list.clear();
        m_result_list.clear();
        for (auto i : lst)
            m_anno_list.push_back(i.toString() + ".json");
        auto nn = prepareNetwork();

        std::vector<tiny_dnn::label_t> train_labels, test_labels;
        std::vector<tiny_dnn::vec_t> train_images, test_images;
        prepareTrainData(train_labels, test_labels, train_images, test_images, m_root, m_anno_list);

        std::vector<tiny_dnn::label_t> test_ret;
        for (auto j : test_images)
            m_result_list.push_back(QString::number(recognizeNumber(nn, j)));
        m_job_state = "process_finish";

        /*tiny_dnn::adagrad optimizer;
        std::cout << "start training" << std::endl;
        tiny_dnn::progress_display disp(train_images.size());
        tiny_dnn::timer t;
        optimizer.alpha *=
        std::min(tiny_dnn::float_t(4),
                 static_cast<tiny_dnn::float_t>(sqrt(m_minibatch) * m_learning_rate));

        int epoch = 1;
        auto on_enumerate_epoch = [&]() {
        std::cout << "Epoch " << epoch << "/" << m_train_epochs << " finished. "
                  << t.elapsed() << "s elapsed." << std::endl;
        ++epoch;
        tiny_dnn::result res = nn.test(test_images, test_labels);
        std::cout << res.num_success << "/" << res.num_total << std::endl;

        disp.restart(train_images.size());
        t.restart();
        };

        auto on_enumerate_minibatch = [&]() { disp += m_minibatch; };
        nn.train<tiny_dnn::mse>(optimizer, train_images, train_labels, m_minibatch,
                              m_train_epochs, on_enumerate_minibatch,
                              on_enumerate_epoch);
        std::cout << "end training." << std::endl;*/

        //auto ret = nn.test(test_images, test_labels);
        //ret.print_detail(std::cout);
        //nn.save("Gem-LeNet-model");

        return aInput;
    }, "", "", 1);

    dst::streamManager::instance()->registerEvent("task_state", "mdysev", [this](std::shared_ptr<dst::streamData> aInput){
        auto cfg = reinterpret_cast<dst::streamJson*>(aInput.get())->getData();
        Send(QJsonDocument(dst::Json(
                               "id", cfg->value("id"),
                               "type", cfg->value("type"),
                               "state", m_job_state)).toJson(QJsonDocument::Compact).toStdString());
        return aInput;
    });
    dst::streamManager::instance()->registerEvent("upload", "mdysev", [this](std::shared_ptr<dst::streamData> aInput){
        Send(QJsonDocument(dst::Json(
                               "state", "begin",
                               "err_code", 0,
                               "mgs", "")).toJson(QJsonDocument::Compact).toStdString());

        auto cfg = reinterpret_cast<dst::streamJson*>(aInput.get())->getData();
        QString dir = m_root + "/" + cfg->value("data_root").toString();
        QDir().mkdir(dir);
        dir += "/predictions";
        QDir().mkdir(dir);

        int pred_idx = 0;
        for (auto i : m_anno_list){
            QFile fl(m_root + "/imageInfo/" + i);
            if (fl.open(QFile::ReadOnly)){
                auto img = QJsonDocument::fromJson(fl.readAll()).object();
                auto shps = img.value("shapes").toObject();
                QJsonArray ret_shps;
                QJsonArray ori_shps;
                int idx = 0;
                for (auto j : shps){
                    ori_shps.push_back(j);
                    auto shp = j.toObject();
                    shp.insert("origin_shape_index", idx++);
                    shp.insert("score", 1.0);
                    shp.insert("label", m_result_list[pred_idx++]);
                    ret_shps.push_back(shp);
                }
                img.insert("shapes", shps);
                img.insert("predict_shapes", ret_shps);

                QFile fl2(dir + "/" + i);
                if (fl2.open(QFile::WriteOnly)){
                    fl2.write(QJsonDocument(img).toJson());
                    fl2.close();
                }
                fl.close();
            }
        }

        m_job_state = "upload_finish";
        return aInput;
    });
}

std::shared_ptr<dst::streamData> InitializeServer(std::shared_ptr<dst::streamData> aInput){
    trainingServer::instance();
    return aInput;
}
REGISTERPipe2(initializeBackend, inisev, InitializeServer, 0, inistg);
