#include <QCoreApplication>
#include <signal.h>
#include <map>
#include <thread>
#include <vector>
#include <iostream>
#include "keyaliases.h"
#include <QPoint>
#include <QString>
#include <QDebug>
#include <QStringList>
#include <QKeySequence>
#include <QProcess>
#include <QCommandLineParser>
#include <SDL2/SDL.h>
#include <SDL2/SDL_joystick.h>

using namespace std;

enum InputElements {
    Q_SDL_BUTTON,
    Q_SDL_AXIS,
    Q_SDL_CORRUPT
};
enum OutputElements {
    Q_JS_EXEC_FILE, //Will only spawn a process and detach from it.
    Q_JS_SIM_KEYS,
    Q_JS_SIM_MOUSE,
    Q_JS_SIM_CLICK,
    Q_JS_SIM_SCROLL
};

enum StringEnum {
    Q_OPT_BUTTON,
    Q_OPT_AXIS,
    Q_OPT_ALLJOY,
    Q_DESC_BUTTONOPT,
    Q_DESC_AXISOPT,
    Q_DESC_ALLJOYOPT,
    Q_DESC_JOYPOS
};

struct ActionStruct {
    qint8 in_type;
    quint8 in_signal;
    quint8 out_type;
    quint16 axis_threshold;
    quint8 mouse_btn;
    QString exec_string;
    QStringList keys;
    list<qint32> keys_int;
    QPoint mouseMoveVector;
};

//Typedefs
typedef vector<ActionStruct> actionlist;
typedef list<qint16> keylist;

typedef multimap<qint8,ActionStruct> actionmap;
typedef pair<qint8,ActionStruct> actionpair;

typedef map<Sint32,SDL_Joystick*> joystickmap;
typedef pair<Sint32,SDL_Joystick*> jspair;

typedef map<Sint32,SDL_GameController*> controllermap;
typedef pair<Sint32,SDL_GameController*> gcpair;

typedef map<qint8,QString> stringmap;
typedef pair<qint8,QString> strpr;

typedef map<quint8,qint16> axismap; //Used for keeping track of active axes, as they may be held without changing their values. SDL is only invoked when the values change.
typedef pair<quint8,qint16> axispair;


//Maps
actionmap rootmap;
joystickmap jsdevs;
controllermap ctllist;
string targetJs;
stringmap optstrings;


bool sdl_do_loop = true;
std::thread* sdl_thread;

//Functions
actionlist findActions(qint8 key);
void insertElement(actionmap *thismap, qint8 key,ActionStruct value);
void addController(Sint32 which);
void removeController(Sint32 which);
void sdl_eventloop();
void destructor(int s);
void handleJsEvent(qint8 eventType, quint8 eventMetaData, qint16 eventData);
ActionStruct outEventInterpret(QStringList eventArgs, qint8 eventType);
list<qint32> cheekyKeyConvert(QStringList keys);

int main(int argc, char *argv[])
{
    signal(SIGINT,destructor);

    //Because I am a lazy-ass. And I change the names sometimes.
    optstrings[Q_OPT_BUTTON] = "button";
    optstrings[Q_DESC_BUTTONOPT] = "Button mapping, examples: js-btn:exec:\"script.sh\"|js-btn:key:Ctrl+Shift+R|js-btn:mousemove:5,5";
    optstrings[Q_OPT_AXIS] = "axis";
    optstrings[Q_DESC_AXISOPT] = "Axis mapping, examples: [axis:threshold:exec:\"script.sh\"|axis:threshold:key:Ctrl+Shift+R|axis:threshold:mousemove:x:5]";
    optstrings[Q_OPT_ALLJOY] = "all-joysticks";
    optstrings[Q_DESC_ALLJOYOPT] = "Take all joystick devices, you do not need to specify joystick name with this.";
    optstrings[Q_DESC_JOYPOS] = "Joystick identifier";

    QCoreApplication a(argc, argv);
    a.setApplicationName("JS Runner");
    a.setApplicationVersion("0.1");
    QCommandLineParser cParse;
    cParse.addVersionOption();
    cParse.addHelpOption();
    cParse.addOption(QCommandLineOption(optstrings[Q_OPT_BUTTON],optstrings[Q_DESC_BUTTONOPT],"mapping"));
    cParse.addOption(QCommandLineOption(optstrings[Q_OPT_AXIS],optstrings[Q_DESC_AXISOPT],"mapping"));
    cParse.addOption(QCommandLineOption(optstrings[Q_OPT_ALLJOY],optstrings[Q_DESC_ALLJOYOPT]));
    cParse.addPositionalArgument("joystick",optstrings[Q_DESC_JOYPOS],"<joystick name>");
    cParse.addPositionalArgument("handler","The library that will take care of inputs","<lib*.so>");
    cParse.process(a.arguments());

    QStringList posArgs = cParse.positionalArguments();
    targetJs = "";
    if(!cParse.optionNames().contains(optstrings[Q_OPT_ALLJOY])&&posArgs.size()>1){
        targetJs = posArgs.first().toUtf8().constData();
        posArgs.pop_front();
        qDebug() << "Only taking input from:" << QString::fromStdString(targetJs);
    }else
        cParse.showHelp(0);
    if(posArgs.size()>1){

    }
    foreach(QString btn,cParse.values(optstrings[Q_OPT_BUTTON])){
        QStringList btnArgs = btn.split(":");
        ActionStruct btnStruct = outEventInterpret(btnArgs,Q_SDL_BUTTON);
        if(btnStruct.in_type==Q_SDL_CORRUPT)
            continue;
        btnStruct.in_type = Q_SDL_BUTTON;
        insertElement(&rootmap,Q_SDL_BUTTON,btnStruct);
    }

    foreach(QString axe,cParse.values(optstrings[Q_OPT_AXIS])){
        QStringList axeArgs = axe.split(":");
        ActionStruct axeStruct = outEventInterpret(axeArgs,Q_SDL_AXIS);
        if(axeStruct.in_type==Q_SDL_CORRUPT)
            continue;
        axeStruct.in_type = Q_SDL_AXIS;
        insertElement(&rootmap,Q_SDL_AXIS,axeStruct);
    }

    for(auto const it : rootmap){
        ActionStruct act = it.second;
        qDebug() << act.in_type << act.in_signal << act.axis_threshold << act.out_type << act.keys << QList<qint32>::fromStdList(act.keys_int) << act.mouseMoveVector << act.exec_string;
    }

    sdl_thread = new std::thread(sdl_eventloop);

    sdl_thread->join();
    return 0;
}

ActionStruct outEventInterpret(QStringList eventArgs,qint8 eventType){
    ActionStruct actStruct;
    actStruct.in_type = Q_SDL_CORRUPT;
    quint8 _offs = 0; //In case of axes. Beware of axes.
    if(eventType==Q_SDL_AXIS){
        _offs++;
        actStruct.axis_threshold=eventArgs.at(1).toInt();
    }
    if(eventArgs.size()<2+_offs)
        return actStruct;
    actStruct.in_signal = eventArgs.at(0).toInt();
    QString outType = eventArgs.at(1+_offs);
    QString outData;
    for(auto const it : eventArgs.mid(2+_offs,-1)) //In case we chopped up something we shouldn't have
        if(outData.isEmpty()){
            outData = it;
        }else
            outData.append(":"+it); //If this breaks something, we'll fix it later.
    if(outType=="exec"){
        actStruct.out_type = Q_JS_EXEC_FILE;
        actStruct.exec_string = outData;
    }else if(outType=="key"){
        actStruct.out_type = Q_JS_SIM_KEYS;
        actStruct.keys_int = cheekyKeyConvert(outData.split("+"));
        actStruct.keys = outData.split("+");
    }else if(outType=="mousemove"&&eventType!=Q_SDL_AXIS){
        actStruct.out_type = Q_JS_SIM_MOUSE;
        QPoint mouseVec;
        QStringList _strs = outData.split(",");
        if(_strs.size()==2)
            for(qint8 i=0;i<_strs.size();i++)
                switch(i){
                case 0: mouseVec.setX(_strs.at(i).toInt()); break;
                case 1: mouseVec.setY(_strs.at(i).toInt()); break;
                }
        actStruct.mouseMoveVector = mouseVec;
    }else if(outType=="mousemove"&&eventType==Q_SDL_AXIS){
        actStruct.out_type = Q_JS_SIM_MOUSE;
        QPoint mouseVec;
        QStringList _strs = outData.split(":");
        if(_strs.size()<2)
            return actStruct;
        if(_strs.at(0)=="x"){
            mouseVec.setX(_strs.at(1).toInt());
        }else if(_strs.at(0)=="y"){
            mouseVec.setY(_strs.at(1).toInt());
        }
        actStruct.mouseMoveVector = mouseVec;
    }else if(outType=="click"){
        actStruct.out_type = Q_JS_SIM_CLICK;
        for(auto const it : qstringBtnTranslation){
            if(outData==it.first){
                actStruct.mouse_btn = it.second;break;
            }
        }
    }else if(outType=="scroll"){
        actStruct.out_type = Q_JS_SIM_SCROLL;
        QPoint mouseVec;
        QStringList _strs = outData.split(",");
        if(_strs.size()==2)
            for(qint8 i=0;i<_strs.size();i++)
                switch(i){
                case 0: mouseVec.setX(_strs.at(i).toInt()); break;
                case 1: mouseVec.setY(_strs.at(i).toInt()); break;
                }
        actStruct.mouseMoveVector = mouseVec;
    }else{
        qDebug() << "Unknown output event type";
        return actStruct;
    }
    actStruct.in_type = 0;
    return actStruct;
}

list<qint32> cheekyKeyConvert(QStringList keys){
    list<qint32> outList;
    for(auto const keke : keys){
        for(auto const kaka : qstringKeyTranslation)
            if(kaka.first==keke){
                outList.insert(outList.end(),kaka.second);
                goto keyconvert_step_out;
            }
        if(keke!=""){
            QKeySequence converter = QKeySequence::fromString(keke);
            outList.insert(outList.end(),converter[0]);
        }
        keyconvert_step_out:
        continue;
    }
    return outList;
}

void sdl_eventloop(){
    if(SDL_Init(SDL_INIT_GAMECONTROLLER)<0){
        qFatal("Failed to init GAMECONTROLLER");
        return;
    }
    SDL_Event sdlEvent;
    qDebug() << "Starting main loop";
    while(sdl_do_loop){
        while(SDL_PollEvent(&sdlEvent)){
            switch(sdlEvent.type){
            case SDL_CONTROLLERDEVICEADDED:{
                qDebug() << "Controller added";
                addController(sdlEvent.cdevice.which);
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED:{
                qDebug() << "Controller removed";
                removeController(sdlEvent.cdevice.which);
                break;
            }
            case SDL_CONTROLLERBUTTONDOWN:{
                handleJsEvent(Q_SDL_BUTTON,sdlEvent.cbutton.state,sdlEvent.cbutton.button);
                break;
            }
            case SDL_CONTROLLERBUTTONUP:{
                handleJsEvent(Q_SDL_BUTTON,sdlEvent.cbutton.state,sdlEvent.cbutton.button);
                break;
            }
            case SDL_CONTROLLERAXISMOTION:{
                handleJsEvent(Q_SDL_AXIS,sdlEvent.caxis.axis,sdlEvent.caxis.value);
                break;
            }
            case SDL_KEYDOWN:{
                qDebug() << "Key pressed";
                switch(sdlEvent.key.keysym.sym){
                case SDLK_q:{
                    qDebug() << "Calling eventloop end";
                    sdl_do_loop = false;
                }
                }
                break;
            }
            }
        }
    }
}

void handleJsEvent(qint8 eventType,quint8 eventMetaData,qint16 eventData){
    switch(eventType){
    case Q_SDL_BUTTON:{
        qDebug() << "js_btn:" << eventMetaData << eventData;
        break;
    }
    case Q_SDL_AXIS:{
        if(eventData<-8000||eventData>8000)
            qDebug() << "js_axis:" << eventMetaData << eventData;
        break;
    }
    }
}

void destructor(int s){
    qDebug() << "dest";
    sdl_do_loop = false;
    sdl_thread->join();
    for(auto const it : jsdevs){
        SDL_JoystickClose(it.second);
        jsdevs.erase(it.first);
    }
    for(auto const it : ctllist){
        SDL_GameControllerClose(it.second);
        ctllist.erase(it.first);
    }
}

void addController(Sint32 which){
    SDL_GameController* gc = SDL_GameControllerOpen(which);
    if(gc){
        SDL_Joystick* jsdev = SDL_GameControllerGetJoystick(gc);
        string gcname = SDL_GameControllerName(gc);
        if(targetJs!=""&&gcname!=targetJs){
            qDebug() << QString::fromStdString(gcname) << ": detected but rejected";
            SDL_GameControllerClose(gc);
            return;
        }
        qDebug() << QString::fromStdString(gcname) << ": detected and opened";

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
            jsdevs.erase(which);
        }
    for(auto const it : ctllist)
        if(it.first==which){
            //Apparently, removing a USB controller does not require us to close it. We'll just remove the pointer, then.
            SDL_GameController* gc = it.second;
            qDebug() << SDL_GameControllerName(gc) << ": disconnected";
            ctllist.erase(which);
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
