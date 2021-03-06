#include "lacan_app.h"
#include <QMessageBox>
#include <QProcess>

lacan_app::lacan_app(int &argc, char **argv):QApplication(argc, argv){}

//Utilizada unicamente para implementar un catcher global de excepciones en el programa
bool lacan_app::notify(QObject* receiver, QEvent* event){
    bool done = true;
    try {
        done = QApplication::notify(receiver, event);
    } catch (std::exception &e) {
        QMessageBox::warning(nullptr, "Ups", "Al parecer hubo una excepcion:\n"+QString(*(e.what())), QMessageBox::Ok);

        this->quit();
        QProcess::startDetached(this->arguments()[0], this->arguments());
    } catch (...) {
        QMessageBox::warning(nullptr, "Ups", "Al parecer hubo una excepcion desconocida", QMessageBox::Ok);
    }
    return done;
}
