#ifndef J3DCORE_TESTS_J3DTESTRUNNER_H
#define J3DCORE_TESTS_J3DTESTRUNNER_H

#include <stddef.h>

#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

// Kind of a test suite. System suites need a real device, window or display; disruptive suites can also switch
// fullscreen modes or disturb devices and are only built on request.
typedef enum eJ3DTestSuiteKind
{
    J3DTEST_SUITE_UNIT       = 0x1,
    J3DTEST_SUITE_SYSTEM     = 0x2,
    J3DTEST_SUITE_DISRUPTIVE = 0x4,
} J3DTestSuiteKind;

// One test suite of a module test executable: a Unity test group runner.
typedef struct sJ3DTestSuite
{
    const char* pName;      // Name of the group runner, e.g. "stdMath" for TEST_GROUP_RUNNER(stdMath)
    void (*pfRun)(void);    // TEST_<name>_GROUP_RUNNER
    J3DTestSuiteKind kind;
} J3DTestSuite;

/**
 * Runs the selected suites of a module test executable with Unity Fixture.
 * Every suite runs by default. Options (all other arguments, e.g. -v, -g, -n, are passed on to Unity):
 *   --suite A,B           run only these suites (exact names, can be repeated)
 *   --exclude-suite C,D   run all but these suites (can be repeated)
 *   --kind unit,system,disruptive
 *                         run only suites of these kinds
 *   --exclude-system      skip system and disruptive suites
 *   --list                print the suites and their kind, run nothing
 *   -h, --help            print these options and the Unity options
 * @return The number of failed tests, or 2 for invalid options
 */
int J3DTest_RunSuites(int argc, const char* argv[], const J3DTestSuite* aSuites, size_t numSuites);

J3D_EXTERN_C_END

#endif // J3DCORE_TESTS_J3DTESTRUNNER_H
