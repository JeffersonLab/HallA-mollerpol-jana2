#include <JANA/JApplication.h>
#include "JEventProcessor_mollerpol.h"

extern "C" {
    void InitPlugin(JApplication* app) {
        InitJANAPlugin(app);
        app->Add(new JEventProcessor_mollerpol());
    }
}
