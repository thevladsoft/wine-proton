/*
 *    MSDMO tests
 *
 * Copyright 2014 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#define COBJMACROS
#include "dmo.h"
#include "uuids.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
static const GUID GUID_unknowndmo = {0x14d99047,0x441f,0x4cd3,{0xbc,0xa8,0x3e,0x67,0x99,0xaf,0x34,0x75}};
static const GUID GUID_unknowncategory = {0x14d99048,0x441f,0x4cd3,{0xbc,0xa8,0x3e,0x67,0x99,0xaf,0x34,0x75}};
static const GUID GUID_wmp1 = {0x13a7995e,0x7d8f,0x45b4,{0x9c,0x77,0x81,0x92,0x65,0x22,0x57,0x63}};

static const char *guid_to_string(const GUID *guid)
{
    static char buffer[50];
    sprintf(buffer, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            guid->Data1, guid->Data2, guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return buffer;
}

static void test_DMOUnregister(void)
{
    static char buffer[200];
    static const WCHAR testdmoW[] = {'t','e','s','t','d','m','o',0};
    HRESULT hr;

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_unknowncategory);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    /* can't register for all categories */
    hr = DMORegister(testdmoW, &GUID_unknowndmo, &GUID_NULL, 0, 0, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = DMORegister(testdmoW, &GUID_unknowndmo, &GUID_unknowncategory, 0, 0, NULL, 0, NULL);
    if (hr != S_OK) {
        win_skip("Failed to register DMO. Probably user doesn't have persmissions to do so.\n");
        return;
    }

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    /* clean up category since Windows doesn't */
    sprintf(buffer, "DirectShow\\MediaObjects\\Categories\\%s", guid_to_string(&GUID_unknowncategory));
    RegDeleteKeyA(HKEY_CLASSES_ROOT, buffer);
}

static void test_DMOGetName(void)
{
    WCHAR name[80];
    HRESULT hr;

    hr = DMOGetName(&GUID_unknowndmo, NULL);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

    /* no such DMO */
    name[0] = 'a';
    hr = DMOGetName(&GUID_wmp1, name);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(name[0] == 'a', "got %x\n", name[0]);
}

static void test_DMOEnum(void)
{
    IEnumDMO *enum_dmo;
    HRESULT hr;
    CLSID clsid;
    WCHAR *name;
    DWORD count;

    hr = DMOEnum(&GUID_unknowncategory, 0, 0, NULL, 0, NULL, &enum_dmo);
    ok(hr == S_OK, "DMOEnum() failed with %#x\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 1, &clsid, &name, NULL);
    ok(hr == S_FALSE, "expected S_FALSE, got %#x\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 2, &clsid, &name, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 2, &clsid, &name, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#x\n", hr);
    ok(count == 0, "expected 0, got %d\n", count);

    hr = IEnumDMO_Next(enum_dmo, 2, NULL, &name, &count);
    ok(hr == E_POINTER, "expected S_FALSE, got %#x\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 2, &clsid, NULL, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#x\n", hr);
    ok(count == 0, "expected 0, got %d\n", count);

    IEnumDMO_Release(enum_dmo);
}

static void test_DMOs(void)
{
    DMO_MEDIA_TYPE type;
    IEnumDMO *enum_dmo;
    IMediaObject *dmo;
    CLSID clsid;
    WCHAR *name;
    HRESULT hr;

    CoInitialize(NULL);

    hr = DMOEnum(&GUID_NULL, 0, 0, NULL, 0, NULL, &enum_dmo);
    ok(hr == S_OK, "got %#x\n", hr);

    while ((hr = IEnumDMO_Next(enum_dmo, 1, &clsid, &name, NULL)) == S_OK)
    {
        DWORD inputs, outputs, i;

        trace("testing %s\n", wine_dbgstr_guid(&clsid));

        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaObject, (void **)&dmo);
        ok(hr == S_OK, "got %#x\n", hr);

        hr = IMediaObject_GetStreamCount(dmo, &inputs, &outputs);
        ok(hr == S_OK, "got %#x\n", hr);

        /* test inputs */
        for (i = 0; i < inputs; i++)
        {
            int type_index = 0;

            hr = IMediaObject_GetInputCurrentType(dmo, i, &type);
            ok(hr == DMO_E_TYPE_NOT_SET, "got %#x\n", hr);

            while ((hr = IMediaObject_GetInputType(dmo, i, type_index, &type)) == S_OK)
            {
                MoFreeMediaType(&type);
                type_index++;
            }
            ok(hr == DMO_E_NO_MORE_ITEMS || broken(hr == DMO_E_TYPE_NOT_SET), "got %#x\n", hr);
        }

        hr = IMediaObject_GetInputCurrentType(dmo, i, &type);
        ok(hr == DMO_E_INVALIDSTREAMINDEX || broken(hr == DMO_E_TYPE_NOT_SET), "got %#x\n", hr);

        /* and outputs */
        for (i = 0; i < outputs; i++)
        {
            int type_index = 0;

            hr = IMediaObject_GetOutputCurrentType(dmo, i, &type);
            ok(hr == DMO_E_TYPE_NOT_SET, "got %#x\n", hr);

            while ((hr = IMediaObject_GetOutputType(dmo, i, type_index, &type)) == S_OK)
            {
                MoFreeMediaType(&type);
                type_index++;
            }
            ok(hr == DMO_E_NO_MORE_ITEMS || broken(hr == DMO_E_TYPE_NOT_SET || hr == E_FAIL), "got %#x\n", hr);
        }

        hr = IMediaObject_GetOutputCurrentType(dmo, i, &type);
        ok(hr == DMO_E_INVALIDSTREAMINDEX || broken(hr == DMO_E_TYPE_NOT_SET), "got %#x\n", hr);

        IMediaObject_Release(dmo);
    }
    ok(hr == S_FALSE, "got %#x\n", hr);

    IEnumDMO_Release(enum_dmo);

    CoUninitialize();
}

START_TEST(msdmo)
{
    test_DMOUnregister();
    test_DMOGetName();
    test_DMOEnum();
    test_DMOs();
}
