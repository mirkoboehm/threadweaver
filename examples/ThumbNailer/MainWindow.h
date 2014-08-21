#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "Model.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent*);

private Q_SLOTS:
    void slotOpenFiles();
    void slotSelectOutputDirectory();
    void slotQuit();

private:
    Ui::MainWindow *ui;
    QString m_outputDirectory;
    Model m_model;
};

#endif // MAINWINDOW_H