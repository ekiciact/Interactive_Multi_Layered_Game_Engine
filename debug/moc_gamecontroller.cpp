/****************************************************************************
** Meta object code from reading C++ file 'gamecontroller.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../gamecontroller.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gamecontroller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSGameControllerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSGameControllerENDCLASS = QtMocHelpers::stringData(
    "GameController",
    "switchView",
    "",
    "startAutoPlay",
    "saveGame",
    "loadGame",
    "newGame",
    "restartGame"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSGameControllerENDCLASS_t {
    uint offsetsAndSizes[16];
    char stringdata0[15];
    char stringdata1[11];
    char stringdata2[1];
    char stringdata3[14];
    char stringdata4[9];
    char stringdata5[9];
    char stringdata6[8];
    char stringdata7[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSGameControllerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSGameControllerENDCLASS_t qt_meta_stringdata_CLASSGameControllerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 14),  // "GameController"
        QT_MOC_LITERAL(15, 10),  // "switchView"
        QT_MOC_LITERAL(26, 0),  // ""
        QT_MOC_LITERAL(27, 13),  // "startAutoPlay"
        QT_MOC_LITERAL(41, 8),  // "saveGame"
        QT_MOC_LITERAL(50, 8),  // "loadGame"
        QT_MOC_LITERAL(59, 7),  // "newGame"
        QT_MOC_LITERAL(67, 11)   // "restartGame"
    },
    "GameController",
    "switchView",
    "",
    "startAutoPlay",
    "saveGame",
    "loadGame",
    "newGame",
    "restartGame"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGameControllerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x08,    1 /* Private */,
       3,    0,   51,    2, 0x08,    2 /* Private */,
       4,    0,   52,    2, 0x08,    3 /* Private */,
       5,    0,   53,    2, 0x08,    4 /* Private */,
       6,    0,   54,    2, 0x08,    5 /* Private */,
       7,    0,   55,    2, 0x08,    6 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject GameController::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_CLASSGameControllerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGameControllerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGameControllerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GameController, std::true_type>,
        // method 'switchView'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startAutoPlay'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveGame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadGame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'newGame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'restartGame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void GameController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GameController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->switchView(); break;
        case 1: _t->startAutoPlay(); break;
        case 2: _t->saveGame(); break;
        case 3: _t->loadGame(); break;
        case 4: _t->newGame(); break;
        case 5: _t->restartGame(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *GameController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GameController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGameControllerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int GameController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
