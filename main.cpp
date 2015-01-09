
#include <QCoreApplication>
#include <map>
#include <vector>
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
    qint16 axis_threshold;
    QString exec_string;
    QStringList keys;
    float mouseMoveFactor;
    QPoint mouseMoveVector;
};

typedef multimap<qint8,ActionStruct> actionmap;
typedef vector<ActionStruct> actionlist;
typedef vector<SDL_Joystick*> joysticklist;
typedef vector<SDL_GameController*> controllerlist;
typedef pair<qint8,ActionStruct> actionpair;

actionmap rootmap;
actionlist findActions(qint8 key);
void insertElement(actionmap *thismap, qint8 key,ActionStruct value);

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

    QString joystick;
    foreach(QString btn,cParse.values("button")){
        QStringList btnArgs = btn.split(":");
        ActionStruct btnStruct;
        btnStruct.in_type = Q_SDL_BUTTON;
        btnStruct.in_signal = btnArgs.at(0).toInt();
        insertElement(&rootmap,Q_SDL_BUTTON,btnStruct);
    }

    foreach(QString axe,cParse.values("axis")){
        qDebug() << axe;
    }

    actionmap test;
    ActionStruct testAction;
    testAction.in_type = 65;
    insertElement(&rootmap,1,testAction);
    insertElement(&rootmap,1,testAction);
    insertElement(&rootmap,1,testAction);

    foreach(ActionStruct act, findActions(1)){
        cout << act.in_type;
    }


    if(SDL_Init(SDL_INIT_GAMECONTROLLER)<0){
        qFatal("Failed to init GAMECONTROLLER");
        return 10;
    }
    SDL_Event sdlEvent;
    joysticklist jsdevs;
    controllerlist ctllist;
    while(true){
        while(SDL_PollEvent(&sdlEvent)){
            switch(sdlEvent.type){
            case SDL_CONTROLLERDEVICEADDED:{
                SDL_GameController* gc = SDL_GameControllerOpen(sdlEvent.cdevice.which);
                cout << sdlEvent.cdevice.which;
                if(gc){
                    SDL_Joystick* jsdev = SDL_GameControllerGetJoystick(gc);
                    ctllist.push_back(gc);
                }
                break;
            }
            case SDL_KEYDOWN:{
                if(sdlEvent.key == SDLK_q)
                    break;
            }
            }
        }
    }

    return 0;
}

void insertElement(actionmap *thismap, qint8 key,ActionStruct value){
    actionpair insertPair;
    insertPair.first = key;
    insertPair.second = value;
    thismap->insert(insertPair);
}

actionlist findActions(qint8 key){
    actionlist returnlist;
    rootmap.find(key);
    for(auto const it : rootmap)
        returnlist.push_back(it.second);
    return returnlist;
}
