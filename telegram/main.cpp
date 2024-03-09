#include "client.h"
#include <iostream>

int main() {
    auto client =
        Client("6738742173:AAFt_YfZJ5Ob_Kqb1MIkRDnx-jGoykM97eY", "https://api.telegram.org", 5);
    client.RunBot();

    return 0;
}
