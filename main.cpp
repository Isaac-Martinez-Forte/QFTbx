#include "GUI/windowsgeneral.h"
#include <QApplication>
#include <boost/log/trivial.hpp>

qint32 main(qint32 argc, char *argv[])
{

    BOOST_LOG_TRIVIAL(trace) << "This is a trace severity message";
    BOOST_LOG_TRIVIAL(debug) << "This is a debug severity message";
    BOOST_LOG_TRIVIAL(info) << "This is an informational severity message";
    BOOST_LOG_TRIVIAL(warning) << "This is a warning severity message";
    BOOST_LOG_TRIVIAL(error) << "This is an error severity message";
    BOOST_LOG_TRIVIAL(fatal) << "and this is a fatal severity message";
    
    QApplication a(argc, argv);

    WindowsGeneral * w = new WindowsGeneral();
    w->show();
    
    qint32 i = a.exec();

    delete w;
    return i;

}


