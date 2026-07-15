#include "about_dialog.h"
#include <QVBoxLayout>
#include <QLabel>
AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("About ReDroidCPP");
    resize(500, 400);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* title = new QLabel("<h1>ReDroidCPP</h1><h3>Ultra Advanced Anti-Detection System v3.0</h3>");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addWidget(new QLabel("<p>40+ Anti-Detection Modules<br>98%+ Detection Avoidance<br><br>Banking Edition</p>"));
}
