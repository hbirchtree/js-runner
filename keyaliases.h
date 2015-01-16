#ifndef KEYALIASES
#define KEYALIASES

#include <map>
#include <QString>

typedef std::map<QString,qint32> keytranslation;

keytranslation constant_translation();
keytranslation constant_btn_translation();

keytranslation qstringKeyTranslation = constant_translation();
keytranslation qstringBtnTranslation = constant_btn_translation();

keytranslation constant_translation(){
    keytranslation m;
    m["Shift"] = Qt::Key_Shift;
    m["Ctrl"] = Qt::Key_Control;
    m["Alt"] = Qt::Key_Alt;
    m["AltGr"] = Qt::Key_AltGr;
    m["Super"] = Qt::Key_Super_L;
    return m;
}

keytranslation constant_btn_translation(){
    keytranslation m;
    m["Left"] = Qt::LeftButton;
    m["Middle"] = Qt::MiddleButton;
    m["Right"] = Qt::RightButton;
    m["Back"] = Qt::BackButton;
    m["Forward"] = Qt::ForwardButton;
    return m;
}



#endif // KEYALIASES

