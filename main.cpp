
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

//Typedefs
typedef vector<ActionStruct> actionlist;

typedef multimap<qint8,ActionStruct> actionmap;
typedef pair<qint8,ActionStruct> actionpair;

typedef map<Sint32,SDL_Joystick*> joystickmap;
typedef pair<Sint32,SDL_Joystick*> jspair;

typedef map<Sint32,SDL_GameController*> controllermap;
typedef pair<Sint32,SDL_GameController*> gcpair;

//Maps
actionmap rootmap;
joystickmap jsdevs;
controllermap ctllist;

//Functions
actionlist findActions(qint8 key);
void insertElement(actionmap *thismap, qint8 key,ActionStruct value);
void addController(Sint32 which);
void removeController(Sint32 which);

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

    string joystick;
    for(auto const it : cParse.positionalArguments()){
        qDebug() << it;
    }
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
    qDebug() << "Starting main loop";
    while(true){
        while(SDL_PollEvent(&sdlEvent)){
            switch(sdlEvent.type){
            case SDL_CONTROLLERDEVICEADDED:{
                addController(sdlEvent.cdevice.which);
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED:{

            }
            case SDL_KEYDOWN:{
                if(sdlEvent.key.keysym.sym == SDLK_q)
                    break;
            }
            }
        }
    }

    return 0;
}

void addController(Sint32 which){
    SDL_GameController* gc = SDL_GameControllerOpen(which);
    cout << which;
    if(gc){
        SDL_Joystick* jsdev = SDL_GameControllerGetJoystick(gc);
        cout << SDL_GameControllerName(gc);

        //We add it to our maps so that we may close them later
        gcpair newGc;
        newGc.first = which;
        newGc.second = gc;
        ctllist.insert(newGc);
        jspair newJs;
        newJs.first = which;
        newJs.second = jsdev;
        jsdevs.insert(newJs);
    }
}

void removeController(Sint32 which){
    //We look up the "which" int, close the js and controller
    for(auto const it : jsdevs)
        if(it.first==which){
            SDL_Joystick* jsdev = it.second;
            SDL_JoystickClose(jsdev);
        }
    for(auto const it : ctllist)
        if(it.first==which){
            SDL_GameController* gc = it.second;
            SDL_GameControllerClose(gc);
        }
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
