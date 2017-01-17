#ifndef SENSORNODE_GUI_H
#define SENSORNODE_GUI_H

#include <QMainWindow>

namespace Ui {
class SensorNode_GUI;
}

class SensorNode_GUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit SensorNode_GUI(QWidget *parent = 0);
    ~SensorNode_GUI();
    void printText(std::string s);

private slots:
    void on_pushButton_clicked();

private:
    Ui::SensorNode_GUI *ui;
};

#endif // SENSORNODE_GUI_H
