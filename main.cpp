
#include <QCoreApplication>
#include <map>
#include <iostream>
#include <QPoint>
#include <QString>
#include <QDebug>
#include <QStringList>
#include <QProcess>
#include <QCommandLineParser>
#include <SDL2/SDL.h>
#include <SDL2/SDL_joystick.h>

using namespace std;

enum InputElements {
    Q_SDL_BUTTON,
    Q_SDL_AXIS
};
enum OutputElements {
    Q_JS_EXEC_FILE,
    Q_JS_SIM_KEYS,
    Q_JS_SIM_MOUSE,
    Q_JS_SIM_SCROLL
};
struct ActionStruct {
    qint8 in_type;
    qint8 in_signal;
    qint8 out_type;
    QString exec_string;
    QStringList keys;
    float mouseMoveFactor;
    QPoint mouseMoveVector;
};

typedef map<qint8,ActionStruct> actionmap;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("JS Runner");
    a.setApplicationVersion("0.1");
    QCommandLineParser cParse;
    cParse.addVersionOption();
    cParse.addHelpOption();
    cParse.addOption(QCommandLineOption("button","Button mapping, examples: js-btn:exec:\"script.sh\"|js-btn:key:Ctrl+Shift+R|js-btn:mousemove:5,5","mapping"));
    cParse.addOption(QCommandLineOption("axis","Axis mapping, examples: [axis:threshold:exec:\"script.sh\"|axis:threshold:key:Ctrl+Shift+R|axis:threshold:mousemove:x:5]","mapping"));
    cParse.addPositionalArgument("joystick","Joystick identifier","<joystick name>");
    cParse.process(a.arguments());

    qDebug() << cParse.positionalArguments();
    qDebug() << cParse.values("button");
    qDebug() << cParse.values("axis");
    actionmap test;
    ActionStruct testAction;
    testAction.in_type = Q_SDL_BUTTON;
    test[1] = testAction;
    test[1] = testAction;
    test[1] = testAction;
    test.find(1);
    for(actionmap::iterator finder=test.begin();finder!=test.end();finder++)
        cout << finder.in_type;

    if(SDL_Init(SDL_INIT_GAMECONTROLLER)<0){
        qFatal("Failed to init GAMECONTROLLER");
        return 10;
    }

    return 0;
}
