#ifndef DIALOG_DPV_H
#define DIALOG_DPV_H

#include <QDialog>

namespace Ui {
class Dialog_DPV;
}

class Dialog_DPV : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_DPV(QWidget *parent = 0);
    ~Dialog_DPV();

public:
    Ui::Dialog_DPV *ui;
};

#endif // DIALOG_DPV_H
