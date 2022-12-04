//**************************************************************************
//
// HostLibrary.cpp
//
// Core support for library routines projected by the host into the Python namespace.
//
//**************************************************************************
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//**************************************************************************

#include "PyProvider.h"

// @TODO: This is temporary.  See host.diagnostics.debugLog.  A proper channel should be established for this
#include <DbgEng.h>

using namespace Microsoft::WRL;
using namespace Debugger::DataModel;
using namespace Debugger::DataModel::ScriptProvider;
using namespace Debugger::DataModel::ScriptProvider::Python;

namespace Debugger::DataModel::ScriptProvider::Python::Functions
{

PyObject *PythonHostLibrary_DebugLog::InvokeVa(_In_ PyObject *pArgs)
{
    PythonProvider *pProvider = PythonProvider::Get();
    Marshal::PythonMarshaler *pMarshaler = GetMarshaler();

    //
    // @TODO: This **ABSOLUTELY** should not touch IDebugControl but should define a new channel for diagnostics
    //        information back to the host or script client.  For now, this will go to engine output to simply
    //        allow some level of "printf" style debugging.  This API is **NOT** intended for general printf output
    //        and makes NO GUARANTEE that the debug channel is such.  In actuality, this will probably go to some
    //        "log window" or the like on the client which is completely up to the client as to presentation.
    //
    ComPtr<IDebugHost> spHost;
    ComPtr<IUnknown> spPrivate;
    IfFailedThrow(pProvider->GetHostSymbols()->QueryInterface(IID_PPV_ARGS(&spHost)));
    IfFailedThrow(spHost->GetHostDefinedInterface(&spPrivate));

    ComPtr<IDebugControl4> spCtrl;
    IfFailedThrow(spPrivate.As(&spCtrl));

    std::string outputString;
    auto fn = [&]()
    {
        HRESULT hr = S_OK;

        Py_ssize_t argCount = PyTuple_Size(pArgs);
        for (Py_ssize_t i = 0; i < argCount; ++i)
        {
            PyObject *pArg = PyTuple_GetItem(pArgs, i);
            if (pArg == nullptr)
            {
                return E_UNEXPECTED;
            }

            PyObject *pStr;
            PinnedReference str;
            if (PyUnicode_Check(pArg))
            {
                pStr = pArg;
            }
            else
            {
                pStr = PyObject_Str(pArg);
                if (pStr == nullptr)
                {
                    return E_FAIL;
                }
                str = PinnedReference::Take(pStr);
            }

            const char *pUTF8Str = PyUnicode_AsUTF8AndSize(pStr, nullptr);
            outputString += pUTF8Str;
        }

        return hr;
    };

    IfFailedThrow(ConvertException(fn));
    IfFailedThrow(spCtrl->OutputWide(DEBUG_OUTPUT_NORMAL, L"%S", outputString.c_str()));

    Py_RETURN_NONE;
};

} // Debugger::DataModel::ScriptProvider::Python::Functions

