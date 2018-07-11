#include "Legba.h"
#include <QApplication>
#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Legba w;

//    w.setFixedSize(1000, 1000);
    QBrush br(Qt::TexturePattern);
    br.setTextureImage(QImage(":/kubyky.jpg"));
    QPalette plt = w.palette();
    plt.setBrush(QPalette::Background, br);
    w.setPalette(plt);

    w.show();

    return a.exec();
}
