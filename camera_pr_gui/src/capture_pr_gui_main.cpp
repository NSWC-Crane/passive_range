#include "capture_pr_gui.h"

#include <QApplication>

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
  #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif


int main(int argc, char *argv[])
{

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
//    if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
//    {
//        freopen("CONOUT$", "w", stdout);
//        freopen("CONOUT$", "w", stderr);
//    }
#endif

    QApplication a(argc, argv);
    capture_pr_gui w;
    w.show();
    return a.exec();
}
