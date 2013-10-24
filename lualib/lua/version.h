/* @RELENG_VERSION: 3

    This file is used by the automated build process.

    THIS FILE MUST BE ONLY TAKEN FROM RELENG DIRECTORY.
    DO NOT USE COPY OF THIS FILE FROM ANOTHER PROJECT.

    Modify following macro definitions according to the module
    this file belongs to.

        RELENG_VT_EXECUTABLE           Define this macro if module is executable
        RELENG_VI_INTERNALNAME         Module name (typically a project name)
        RELENG_VI_ORIGINALFILENAME     File name produced when module is built
        RELENG_VI_FILEDESCIPTION       Description of the module

    If module is external to the project and carry it's own version
    number, then modify following macros according to the version of
    the module.
        RELENG_VI_FILEVERSION
        RELENG_VI_FILEVERSION_TEXT

    Note: If RELENG_VI_FILEVERSION is not defined, RELENG_VI_FILEVERSION_TEXT
          is ignored.

    If additional third-party copyright reference is needed,
    define the following macro.
        RELENG_VI_EXTERNALCOPYRIGHT

*/
#ifndef RELENG_PRIVATE_VERSION_H__
#define RELENG_PRIVATE_VERSION_H__

/* Define these macros according to the module */
//#define RELENG_VT_EXECUTABLE
#define RELENG_VI_INTERNALNAME          "Lua"
#define RELENG_VI_ORIGINALFILENAME      "lua.dll"
#define RELENG_VI_FILEDESCIPTION        "Lua 5.2"

/*
    Uncomment and define these macros, if module is external to the project.
    RELENG_VI_FILEVERSION defines comma separated values.
    RELENG_VT_FILEVERSION_TEXT defines the version string, which is typically
                               a dot separated version number.
*/
#define RELENG_VI_FILEVERSION           5,2,2,0
#define RELENG_VI_FILEVERSION_TEXT      "5.2.2.0"

/* Uncomment and define external copyright or credits, if needed. */
#define RELENG_VI_EXTERNALCOPYRIGHT     "Copyright (C) 1994-2013 Lua.org, PUC-Rio"

#endif // RELENG_PRIVATE_VERSION_H__

