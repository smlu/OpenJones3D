// Added: the Null column's headless entry point. The run logic lives in JonesHeadless.c.
#include <Jones3D/Platform/Null/JonesHeadless.h>

int main(int argc, char** argv)
{
    return JonesHeadless_Run(argc, argv);
}
