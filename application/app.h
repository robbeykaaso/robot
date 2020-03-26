#ifndef APPLICATION_APP_H_
#define APPLICATION_APP_H_

#include <QApplication>
//#include <Windows.h>
//#include <CommCtrl.h>

class MyApplication:public QApplication
{
    Q_OBJECT

public:
    MyApplication(int argc,char *argv[]){}
    ~MyApplication(){}
protected:
    bool winEventFilter(MSG *message, long *result){
        /*messa
        switch(message->message)
        {
        case WM_MBUTTONDOWN:{
            int test = 0;
        }
        default:
            break;
        }*/
       // return QApplication::winEventFilter(message,result);
        return false;
    }
};

#endif
