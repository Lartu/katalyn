#include "narivm.hpp"

int main()
{
    if (execute_nambly("PUSH 1\nGSET \"must_not_leak\"\n") != 0)
        return 1;

    // A missing variable is nil. DISP then deliberately raises because nil
    // cannot become text. If the first execution leaked state, this returns 0.
    return execute_nambly("VGET \"must_not_leak\"\nDISP\n") == 1 ? 0 : 1;
}
