#include "Legba.h"
#include "ui_Legba.h"

Legba::Legba(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Legba)
{
    ui->setupUi(this);

    setbuf(stdout, NULL);

    QDir currentFolder = QDir();
    this->absolutePath = currentFolder.absolutePath() + "\\";
    this->dataPath = this->absolutePath + "DataLegba\\";
    QDir dataFolder = QDir(this->dataPath);
    if (!dataFolder.exists())
        dataFolder.mkpath(".");

    on_actionCom_UPD_triggered();

    this->serial.setDataBits(QSerialPort::Data8);
    this->serial.setBaudRate(QSerialPort::Baud9600);
    this->serial.setStopBits(QSerialPort::OneStop);
    this->serial.setParity(QSerialPort::NoParity);
    this->serial.setFlowControl(QSerialPort::NoFlowControl);

    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    ui->customPlot->setNotAntialiasedElements(QCP::aeAll);
    ui->customPlot->xAxis->setTickLabelFont(font);
    ui->customPlot->yAxis->setTickLabelFont(font);
    ui->customPlot->legend->setFont(font);
    ui->customPlot->axisRect()->setupFullAxesBox();

    ui->customPlot->clearGraphs();
    ui->customPlot->addGraph();
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    connect(ui->customPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(on_plot_Clicked(QMouseEvent*)));

    this->dataUnits.clear();
    this->dataUnits.append("Volts");
    this->dataUnits.append("mBar");
    this->dataUnits.append("Pa");
    this->dataUnits.append("Torr");
    ui->comboBoxUnits->clear();
    ui->comboBoxUnits->addItems(this->dataUnits);
    ui->comboBoxUnits->setCurrentText("Torr");

    if (freopen("ConfigLegba.conf", "r+", stdin) != nullptr) {
        std::string buffer;
        while (getline(std::cin, buffer)) {
            QString command = QString(buffer.c_str());
            if (command.startsWith("PrS")) {
                if (command.contains("Com")) {
                    ui->comboBoxComPorts->setCurrentText(command.split(" ")[1]);
                    if (ui->comboBoxComPorts->currentText() == command.split(" ")[1])
                        on_pushButtonAction_clicked();
                }
            }
            if (command.startsWith("COM")) {
                if (command.contains("Baud")) {
                    this->serial.setBaudRate(command.split(" ")[1].toInt());
                }
            }
        }
    }

    ui->pushButtonPlot->setEnabled(false);
}

Legba::~Legba()
{
    delete ui;
}

bool Legba::openSerial()
{
    if (!this->serial.open(QIODevice::ReadWrite)) {
        QSerialPort::SerialPortError error = QSerialPort::NoError;
        this->serial.error(error);

        return false;
    } else {
        return true;
    }
}

void Legba::disconnect()
{
    if (this->serial.isOpen())
        this->serial.close();
    return;
}

bool Legba::send(const QString &command, QString &response, const bool &wait_for_response)
{
    response.clear();

    if (!this->serial.isOpen())
        return false;

    QString modyfiedCommand = command.trimmed() + "\r";

    this->serial.write(modyfiedCommand.toLocal8Bit());
    if (this->serial.waitForBytesWritten(this->writeTimeout)) {
        if (wait_for_response) {
            if (this->serial.waitForReadyRead(readTimeout)) {
                QByteArray responseData = this->serial.readAll();
                while (this->serial.waitForReadyRead(readWaitTimeout))
                    responseData += this->serial.readAll();
                response = QString(responseData).trimmed();

                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    } else {
        return false;
    }
}

bool Legba::sendCommand(const QString &command)
{
    QString response;

    return send(command, response, false);
}

bool Legba::sendQuery(const QString &command, QString &response)
{
    return send(command, response, true);
}

QString Legba::ask(const QString &command)
{
    QString response;
    sendQuery(command, response);

    return response;
}

double Legba::getPressure()
{
    bool ok;

    double ans = ask("?GA1").toDouble(&ok);
    if (!ok)
        ans = -1;

    return ans;
}

void Legba::on_actionCom_UPD_triggered()
{
    QStringList allCom;

    allCom.clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        allCom.push_back(info.portName());
    allCom.push_back("SetIt");

    ui->comboBoxComPorts->clear();
    ui->comboBoxComPorts->addItems(allCom);
    ui->comboBoxComPorts->setCurrentText("SetIt");

    return;
}

void Legba::on_pushButtonAction_clicked()
{
    if (ui->pushButtonAction->text() == "Connect") {
        this->serial.setPortName(ui->comboBoxComPorts->currentText());

        if (!openSerial())
            return;

        bool ok = true;
        int units = ask("?US").toInt(&ok);
        if (!ok || units < 0 || units > 3)
            return;

        ui->comboBoxUnits->setCurrentIndex(units);

        ui->pushButtonAction->setText("Disconnect");

        ui->pushButtonPlot->setEnabled(true);
    } else {
        disconnect();

        ui->checkBoxPlotFixed->setChecked(false);

        ui->pushButtonAction->setText("Connect");

        ui->pushButtonPlot->setEnabled(false);
    }

    return;
}

void Legba::on_plot_Clicked(QMouseEvent *event) const
{
    ui->lineEditX->setText(QString::number(ui->customPlot->xAxis->pixelToCoord(event->pos().x())));
    ui->lineEditY->setText(QString::number(ui->customPlot->yAxis->pixelToCoord(event->pos().y())));

    return;
}

void Legba::on_pushButtonPlot_clicked()
{
    if (ui->pushButtonPlot->text() == "Plot") {
        this->setAttribute(Qt::WA_DeleteOnClose, true);

        ui->pushButtonPlot->setText("Stop");

        this->run = true;

        ui->pushButtonAction->setEnabled(false);

        RunExperiment();
    } else {
        ui->pushButtonPlot->setText("Plot");

        this->setAttribute(Qt::WA_DeleteOnClose, false);

        this->run = false;

        ui->pushButtonAction->setEnabled(true);
    }

    return;
}

void Legba::RunExperiment()
{
    this->yMin = this->yMax = -1;

    ui->customPlot->xAxis->setLabel("Time");
    ui->customPlot->yAxis->setLabel("Pressure");
    ui->customPlot->clearGraphs();
    ui->customPlot->addGraph();

    QString File = this->dataPath + QDateTime::currentDateTime().toString("yyyy_MM_dd_HH_mm_ss") + ".dat";
    freopen(File.toStdString().c_str(), "w+", stdout);

    std::cout << "Time\tPressure\n";

    double value = 0;
    double currentTime = 0;
    double timeStart = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;

    while (this->run) {
        currentTime =  QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeStart;
        ui->lineEditCurrentTime->setText(QString::number(currentTime));

        value = getPressure();
        if (value < 0) {
            ui->lineEditCurrentPressure->setText("LOST SIGNAL");
            fprintf(stdout, "%0.20e\tFAIL", currentTime);
        } else {
            ui->lineEditCurrentPressure->setText(QString::number(value));
            fprintf(stdout, "%0.20e\t%0.20e", currentTime, value);
        }

        std::cout << std::flush;

        ui->customPlot->graph(0)->addData(currentTime, value);

        if (yMin < 0 || value < yMin)
            yMin = value;
        if (yMax < 0 || value > yMax)
            yMax = value;

        QTest::qWait(5);
        fprintf(stdout, "\n");

        if (ui->checkBoxPlotFixed->checkState() == Qt::Unchecked) {
            ui->customPlot->xAxis->setRange(0, currentTime);
            ui->customPlot->yAxis->setRange(yMin*0.999, yMax*1.001);
            ui->customPlot->replot();
        }
    }

    fclose(stdout);

    return;
}
