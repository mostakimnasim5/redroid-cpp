#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H
#include <QDialog>
class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);
};
#endif
