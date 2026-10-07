#include "board.h"
#include "app.h"
#include "cdfw_mcxn947_boot_validation.h"

int main(void)
{
    BOARD_InitHardware();

    cdfw_mcxn947_boot_validation_run();

    for(;;)
    {

    }
}
