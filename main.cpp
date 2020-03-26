//#include "framework/uimanager.h"
#include "../framework/document.h"
//#include "glrender.h"
//#include "../framework/util.h"
//#include "../framework/ImageBoard.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QApplication>
#include <QResource>
#include <QTranslator>
#include <QIcon>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(dst::getCWD("/favicon.png")));
    app.setOrganizationName("somename");
    app.setOrganizationDomain("somename");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("applicationDirPath", QGuiApplication::applicationDirPath()); //https://recalll.co/ask/v/topic/qt-QML%3A-how-to-specify-image-file-path-relative-to-application-folder/55928bae7d3563c7088b7db1
    dst::streamManager::instance()->engine = &engine;
    TRIG("registerQMLClass", std::make_shared<dst::streamData>());

    QJsonObject lan;
    dst::configObject::loadJsonFileConfig("lanInfo", lan);
    QTranslator translator;
    QString ln = lan.value("name").toString();
        translator.load(ln);
    QCoreApplication::installTranslator(&translator);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    //dst::streamManager::instance()->emitSignal("setFocus", STMJSON(dst::Json("board", "panel0")));
    return app.exec();
}
