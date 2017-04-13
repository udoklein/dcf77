//
// Obviously I would prefer to include this in the main sketch.
// However if I put it here I get a chance to overwrite sysTickHook
// before Arduino's main.cpp can overwrite it.
// In older versions of Arduino it was possible to overwrite sysTickHook
// in the same file but this seems to be impossible with 1.8 and above.
//

#if defined(__SAM3X8E__)
extern void process_one_sample();

extern "C" {
    // sysTicks will be triggered once per 1 ms
    int sysTickHook(void) {
        process_one_sample();
        return 0;
    }
}
#endif