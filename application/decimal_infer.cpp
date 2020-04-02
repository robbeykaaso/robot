#include "decimal_infer.h"
#include "tiny_dnn/tiny_dnn.h"
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

void prepareTrainData(std::vector<tiny_dnn::label_t>& aTrainLabels, std::vector<tiny_dnn::label_t>& aTestLabels,
    std::vector<tiny_dnn::vec_t>& aTrainImages, std::vector<tiny_dnn::vec_t>& aTestImages){

    QString rel_dir = "config_/hearthStone/imageInfo";
    QDir dir(rel_dir);
    for (auto i : dir.entryList())
        if (i != "." && i != ".."){
            QFile fl(rel_dir + "/" + i);
            fl.open(QFile::ReadOnly);
            auto cfg = QJsonDocument::fromJson(fl.readAll()).object();
            auto img_pth = "config_/hearthStone/image/" + cfg.value("id").toString() + "/" + cfg.value("source").toArray()[0].toString();
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
                            ret[i * img.cols + j] = ptr[j] / float_t(255) * 2 - 1;
                    }
                    aTrainImages.push_back(ret);
                    aTrainLabels.push_back(shp.value("label").toString().toInt());
                }
            }
            fl.close();
        }

    int sz = aTrainImages.size();
    srand( (unsigned)time(NULL));
    for (int i = sz; i < 45000; ++i){
        auto idx = std::rand() % sz;
        aTrainLabels.push_back(aTrainLabels.at(idx));
        aTrainImages.push_back(aTrainImages.at(idx));
    }
    for (int i = 0; i < 15000; ++i){
        auto idx = std::rand() % sz;
        aTestLabels.push_back(aTrainLabels.at(idx));
        aTestImages.push_back(aTrainImages.at(idx));
    }
}

void trainGemModel(){
  int n_train_epochs = 4;
  tiny_dnn::core::backend_t backend_type = tiny_dnn::core::default_engine();
  double learning_rate = 1;
  int n_minibatch = 16;


  tiny_dnn::network<tiny_dnn::sequential> nn;
  tiny_dnn::adagrad optimizer;
  //construct_net(nn, backend_type);
  nn.load("Gem-LeNet-model", tiny_dnn::content_type::weights_and_model, tiny_dnn::file_format::json);
  std::cout << "load models..." << std::endl;

  std::vector<tiny_dnn::label_t> train_labels, test_labels;
  std::vector<tiny_dnn::vec_t> train_images, test_images;
  prepareTrainData(train_labels, test_labels, train_images, test_images);

  std::cout << "start training" << std::endl;
  tiny_dnn::progress_display disp(train_images.size());
  tiny_dnn::timer t;
  optimizer.alpha *=
    std::min(tiny_dnn::float_t(4),
             static_cast<tiny_dnn::float_t>(sqrt(n_minibatch) * learning_rate));

  int epoch = 1;
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
  nn.train<tiny_dnn::mse>(optimizer, train_images, train_labels, n_minibatch,
                          n_train_epochs, on_enumerate_minibatch,
                          on_enumerate_epoch);
  std::cout << "end training." << std::endl;

  auto ret = nn.test(test_images, test_labels);
  ret.print_detail(std::cout);
  nn.save("Gem-LeNet-model", tiny_dnn::content_type::weights_and_model, tiny_dnn::file_format::json);
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

int recognizeNumber(const cv::Mat& aROI){
    tiny_dnn::network<tiny_dnn::sequential> nn;
    nn.load("LeNet-model");
    tiny_dnn::vec_t data;
    convert_image(aROI, -1.0, 1.0, 32, 32, data);
    auto res = nn.predict(data);
    std::vector<std::pair<double, int>> scores;
    for (int i = 0; i < 10; i++)
        scores.emplace_back(res[i], i);
    sort(scores.begin(), scores.end(), std::greater<std::pair<double, int>>());
    return scores[0].second;
}
