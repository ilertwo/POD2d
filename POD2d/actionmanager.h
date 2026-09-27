#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <QString>

namespace Ui { class MainWindow; }

class ActionManager {
public:
    static void setupShortcuts(Ui::MainWindow *ui);
};

#endif // ACTIONMANAGER_H
