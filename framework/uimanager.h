//duty:
//for trigging the back-end service from qml or update front-end from back-end
//for transforming the data type between c++ and js
#ifndef REAL_FRAMEWORK_UIMANAGER_H_
#define REAL_FRAMEWORK_UIMANAGER_H_

#include <QQuickItem>
#include <QDoubleValidator>
#include "rxmanager.h"
#include "util.h"

namespace dst {

class DSTDLL uimanager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(uimanager)
public:
    Q_PROPERTY(QString layerOneColor READ getLayerOneColor CONSTANT)  //https://codeday.me/bug/20171209/106759.html
    Q_PROPERTY(QString layerTwoColor READ getLayerTwoColor CONSTANT)
    Q_PROPERTY(QString layerThreeColor READ getlayerThreeColor CONSTANT)
    Q_PROPERTY(QString fontColor READ getFontColor CONSTANT)
    Q_PROPERTY(QString fontFamily READ getFontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(QString editColor READ getEditColor CONSTANT)
    Q_PROPERTY(QString buttonColor READ getButtonColor CONSTANT)
    Q_PROPERTY(int fontTitleSize READ getTitleSize CONSTANT)
    Q_PROPERTY(int fontDespSize READ getDespSize CONSTANT)
    uimanager() {}
signals:
    void fontFamilyChanged();
public:
    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine)
    {
        Q_UNUSED(engine);
        Q_UNUSED(scriptEngine);

        return new uimanager();
    }
    static Q_INVOKABLE void setCommand(const QJsonObject& aCommand, QJSValue aCallback);
    static Q_INVOKABLE void unregisterPipe(const QString& aSignal, const QString& aName);
    static Q_INVOKABLE void registerPipe(const QString& aSignal, const QString& aName, QJSValue aEvent, const QString& aPreviouses = "", const QString& aNexts = "");
    QString getFontColor() {return "#c8c8c8";}
    QString getFontFamily() {
        return m_font_family;
    }
    void setFontFamily(const QString& aFamily);
    QString getLayerOneColor() {return "#282828";}
    QString getLayerTwoColor() {return "#424242";}
    QString getlayerThreeColor() {return "#535353";}
    QString getEditColor() {return "#4b4b4b";}
    QString getButtonColor() {return "#015bac";}
    int getTitleSize() {return 15;}
    int getDespSize() {return 12;}
public slots:
private:
    QString m_font_family = "思源黑体";
};

//https://stackoverflow.com/questions/35178569/doublevalidator-is-not-checking-the-ranges-properly
class DSTDLL TextFieldDoubleValidator : public QDoubleValidator {
public:
    TextFieldDoubleValidator (QObject * parent = 0) : QDoubleValidator(parent) {}
    TextFieldDoubleValidator (double bottom, double top, int decimals, QObject * parent) :
                                                                                         QDoubleValidator(bottom, top, decimals, parent) {}

    QValidator::State validate(QString & s, int & pos) const {
        if (s.isEmpty() || (s.startsWith("-") && s.length() == 1)) {
            // allow empty field or standalone minus sign
            return QValidator::Intermediate;
        }
        // check length of decimal places
        QChar point = locale().decimalPoint();
        if(s.indexOf(point) != -1) {
            int lengthDecimals = s.length() - s.indexOf(point) - 1;
            if (lengthDecimals > decimals()) {
                return QValidator::Invalid;
            }
        }
        // check range of value
        bool isNumber;
        double value = locale().toDouble(s, &isNumber);
        if (isNumber && bottom() <= value && value <= top()) {
            return QValidator::Acceptable;
        }
        return QValidator::Invalid;
    }

};

}

#endif
