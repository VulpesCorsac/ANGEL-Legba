#ifndef LEGBA_H
#define LEGBA_H

#include <QMainWindow>

#include <QSerialPort>
#include <QSerialPortInfo>

#include <iostream>
#include <iomanip>
#include <stdio.h>

#include <string>

#include "../ANGEL/Angel.h"

namespace Ui {
    class Legba;
}

class Legba : public QMainWindow
{
    Q_OBJECT

public:
    explicit Legba(QWidget *parent = 0);
    ~Legba();

private:
    QSerialPort serial;

    double yMin = 0;
    double yMax = 0;
    bool run = false;

    QString absolutePath       = "";
    QString dataPath           = "";

    QStringList dataUnits;

    int writeTimeout = 10;
    int readTimeout = 2000;
    int readWaitTimeout = 100;

private slots:
    bool openSerial();
    void disconnect();
    bool send(const QString &command, QString &response, const bool &wait_for_response);
    bool sendCommand(const QString &command);
    bool sendQuery(const QString &command, QString &response);
    QString ask(const QString &command);
    double getPressure();

    void on_actionCom_UPD_triggered();
    void on_pushButtonAction_clicked();

    void on_plot_Clicked(QMouseEvent* event) const;

    void on_pushButtonPlot_clicked();
    void RunExperiment();

private:
    Ui::Legba *ui;
};

#endif // LEGBA_H
