//#include  "windows.h"
#include <QApplication>

#include "screen.h"
#include "beingsnap.h"
//#include <Winuser.h>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    beingSnap b;
    b.show();
//    Screen s;
//    s.show();
    return a.exec();
}
