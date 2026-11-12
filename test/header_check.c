// compile-only check that validates that an installation's C header is self-contained and C-compatible
// compiled during builds with testing enabled and in CI on the installed headers, see ci.yml

#include <stc/stc.h>

// simple for now, as it's not our goal to test library-specifics here
// it just uses a couple of symbols of different kinds (macro, typedef, constant, function) to check
// that we can compile

int main(void) {
    if (stc_abi_version() != STC_ABI_VERSION)
        return 1;

    STC_Status status = STC_SUCCESS;
    (void)status;

    return 0;
}
