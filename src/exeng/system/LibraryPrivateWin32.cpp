
#include "LibraryPrivate.hpp"
#include "../DetectEnv.hpp"

#ifdef EXENG_WINDOWS

#include <Windows.h>

namespace exeng
{
    namespace system
    {
        Library::Private::Private()
        {
            this->handle = NULL;
        }


        void Library::Private::load(const std::string &name)
        {
            this->handle = ::LoadLibraryA(name.c_str());
        }


        void Library::Private::unload()
        {
            ::HMODULE handle = static_cast<::HMODULE>(this->handle);
            ::FreeLibrary(handle);
        }


        FunctionPtr Library::Private::getFunctionPtr(const std::string &name)
        {
            HMODULE handle = NULL;
            FunctionPtr ret = NULL;
            FARPROC procAddress = NULL;

            handle = static_cast<HMODULE>(this->handle);
            procAddress = ::GetProcAddress(handle, name.c_str());
            ret = static_cast<FunctionPtr>(procAddress);

            return ret;
        }
    }
}

#endif
