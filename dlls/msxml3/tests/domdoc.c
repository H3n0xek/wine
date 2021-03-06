/*
 * XML test
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2007-2008 Alistair Leslie-Hughes
 * Copyright 2010-2011 Adam Martinson for CodeWeavers
 * Copyright 2010-2011 Nikolay Sivov for CodeWeavers
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


#define COBJMACROS
#define CONST_VTABLE

#include <stdio.h>
#include <assert.h>

#include "windows.h"

#include "msxml2.h"
#include "msxml2did.h"
#include "ole2.h"
#include "dispex.h"

#include "initguid.h"
#include "objsafe.h"
#include "mshtml.h"

#include "wine/test.h"

DEFINE_GUID(SID_SContainerDispatch, 0xb722be00, 0x4e68, 0x101b, 0xa2, 0xbc, 0x00, 0xaa, 0x00, 0x40, 0x47, 0x70);
DEFINE_GUID(SID_UnknownSID, 0x75dd09cb, 0x6c40, 0x11d5, 0x85, 0x43, 0x00, 0xc0, 0x4f, 0xa0, 0xfb, 0xa3);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    if(!riid)
        return "(null)";

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static int g_unexpectedcall, g_expectedcall;

typedef struct
{
    IDispatch IDispatch_iface;
    LONG ref;
} dispevent;

static inline dispevent *impl_from_IDispatch( IDispatch *iface )
{
    return CONTAINING_RECORD(iface, dispevent, IDispatch_iface);
}

static HRESULT WINAPI dispevent_QueryInterface(IDispatch *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IDispatch) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *ppvObject = iface;
    }
    else
        return E_NOINTERFACE;

    IDispatch_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI dispevent_AddRef(IDispatch *iface)
{
    dispevent *This = impl_from_IDispatch( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI dispevent_Release(IDispatch *iface)
{
    dispevent *This = impl_from_IDispatch( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI dispevent_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    g_unexpectedcall++;
    *pctinfo = 0;
    return S_OK;
}

static HRESULT WINAPI dispevent_GetTypeInfo(IDispatch *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    g_unexpectedcall++;
    return S_OK;
}

static HRESULT WINAPI dispevent_GetIDsOfNames(IDispatch *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    g_unexpectedcall++;
    return S_OK;
}

static HRESULT WINAPI dispevent_Invoke(IDispatch *iface, DISPID member, REFIID riid,
        LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result,
        EXCEPINFO *excepInfo, UINT *argErr)
{
    ok(member == 0, "expected 0 member, got %d\n", member);
    ok(lcid == LOCALE_SYSTEM_DEFAULT, "expected LOCALE_SYSTEM_DEFAULT, got lcid %x\n", lcid);
    ok(flags == DISPATCH_METHOD, "expected DISPATCH_METHOD, got %d\n", flags);

    ok(params->cArgs == 0, "got %d\n", params->cArgs);
    ok(params->cNamedArgs == 0, "got %d\n", params->cNamedArgs);
    ok(params->rgvarg == NULL, "got %p\n", params->rgvarg);
    ok(params->rgdispidNamedArgs == NULL, "got %p\n", params->rgdispidNamedArgs);

    ok(result == NULL, "got %p\n", result);
    ok(excepInfo == NULL, "got %p\n", excepInfo);
    ok(argErr == NULL, "got %p\n", argErr);

    g_expectedcall++;
    return E_FAIL;
}

static const IDispatchVtbl dispeventVtbl =
{
    dispevent_QueryInterface,
    dispevent_AddRef,
    dispevent_Release,
    dispevent_GetTypeInfoCount,
    dispevent_GetTypeInfo,
    dispevent_GetIDsOfNames,
    dispevent_Invoke
};

static IDispatch* create_dispevent(void)
{
    dispevent *event = HeapAlloc(GetProcessHeap(), 0, sizeof(*event));

    event->IDispatch_iface.lpVtbl = &dispeventVtbl;
    event->ref = 1;

    return (IDispatch*)&event->IDispatch_iface;
}

/* object site */
DEFINE_EXPECT(site_qi_IServiceProvider);
DEFINE_EXPECT(site_qi_IXMLDOMDocument);
DEFINE_EXPECT(site_qi_IOleClientSite);

DEFINE_EXPECT(sp_queryservice_SID_SBindHost);
DEFINE_EXPECT(sp_queryservice_SID_SContainerDispatch_htmldoc2);
DEFINE_EXPECT(sp_queryservice_SID_secmgr_htmldoc2);
DEFINE_EXPECT(sp_queryservice_SID_secmgr_xmldomdoc);
DEFINE_EXPECT(sp_queryservice_SID_secmgr_secmgr);

DEFINE_EXPECT(htmldoc2_get_all);
DEFINE_EXPECT(htmldoc2_get_url);
DEFINE_EXPECT(collection_get_length);

typedef struct
{
    IServiceProvider IServiceProvider_iface;
} testprov_t;

testprov_t testprov;

static HRESULT WINAPI site_QueryInterface(IUnknown *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IServiceProvider))
        CHECK_EXPECT2(site_qi_IServiceProvider);

    if (IsEqualGUID(riid, &IID_IXMLDOMDocument))
        CHECK_EXPECT2(site_qi_IXMLDOMDocument);

    if (IsEqualGUID(riid, &IID_IOleClientSite))
        CHECK_EXPECT2(site_qi_IOleClientSite);

    if (IsEqualGUID(riid, &IID_IUnknown))
         *ppvObject = iface;
    else if (IsEqualGUID(riid, &IID_IServiceProvider))
         *ppvObject = &testprov.IServiceProvider_iface;

    if (*ppvObject) IUnknown_AddRef(iface);

    return *ppvObject ? S_OK : E_NOINTERFACE;
}

static ULONG WINAPI site_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI site_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl testsiteVtbl =
{
    site_QueryInterface,
    site_AddRef,
    site_Release
};

typedef struct
{
    IUnknown IUnknown_iface;
} testsite_t;

static testsite_t testsite = { { &testsiteVtbl } };

/* test IHTMLElementCollection */
static HRESULT WINAPI htmlecoll_QueryInterface(IHTMLElementCollection *iface, REFIID riid, void **ppvObject)
{
    ok(0, "unexpected call\n");
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI htmlecoll_AddRef(IHTMLElementCollection *iface)
{
    return 2;
}

static ULONG WINAPI htmlecoll_Release(IHTMLElementCollection *iface)
{
    return 1;
}

static HRESULT WINAPI htmlecoll_GetTypeInfoCount(IHTMLElementCollection *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_GetTypeInfo(IHTMLElementCollection *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_GetIDsOfNames(IHTMLElementCollection *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_Invoke(IHTMLElementCollection *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_toString(IHTMLElementCollection *iface, BSTR *String)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_put_length(IHTMLElementCollection *iface, LONG v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_get_length(IHTMLElementCollection *iface, LONG *v)
{
    CHECK_EXPECT2(collection_get_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_get__newEnum(IHTMLElementCollection *iface, IUnknown **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_item(IHTMLElementCollection *iface, VARIANT name, VARIANT index, IDispatch **pdisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_tags(IHTMLElementCollection *iface, VARIANT tagName, IDispatch **pdisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IHTMLElementCollectionVtbl TestHTMLECollectionVtbl = {
    htmlecoll_QueryInterface,
    htmlecoll_AddRef,
    htmlecoll_Release,
    htmlecoll_GetTypeInfoCount,
    htmlecoll_GetTypeInfo,
    htmlecoll_GetIDsOfNames,
    htmlecoll_Invoke,

    htmlecoll_toString,
    htmlecoll_put_length,
    htmlecoll_get_length,
    htmlecoll_get__newEnum,
    htmlecoll_item,
    htmlecoll_tags
};

typedef struct
{
    IHTMLElementCollection IHTMLElementCollection_iface;
} testhtmlecoll_t;

static testhtmlecoll_t htmlecoll = { { &TestHTMLECollectionVtbl } };

/* test IHTMLDocument2 */
static HRESULT WINAPI htmldoc2_QueryInterface(IHTMLDocument2 *iface, REFIID riid, void **ppvObject)
{
   trace("\n");
   *ppvObject = NULL;
   return E_NOINTERFACE;
}

static ULONG WINAPI htmldoc2_AddRef(IHTMLDocument2 *iface)
{
    return 2;
}

static ULONG WINAPI htmldoc2_Release(IHTMLDocument2 *iface)
{
    return 1;
}

static HRESULT WINAPI htmldoc2_GetTypeInfoCount(IHTMLDocument2 *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_GetTypeInfo(IHTMLDocument2 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_GetIDsOfNames(IHTMLDocument2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_Invoke(IHTMLDocument2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_Script(IHTMLDocument2 *iface, IDispatch **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_all(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    CHECK_EXPECT2(htmldoc2_get_all);
    *p = &htmlecoll.IHTMLElementCollection_iface;
    return S_OK;
}

static HRESULT WINAPI htmldoc2_get_body(IHTMLDocument2 *iface, IHTMLElement **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_activeElement(IHTMLDocument2 *iface, IHTMLElement **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_images(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_applets(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_links(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_forms(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_anchors(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_title(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_title(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_scripts(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_designMode(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_designMode(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_selection(IHTMLDocument2 *iface, IHTMLSelectionObject **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_readyState(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_frames(IHTMLDocument2 *iface, IHTMLFramesCollection2 **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_embeds(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_plugins(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_alinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_alinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_bgColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_bgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_fgColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_linkColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_linkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_vlinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_vlinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_referrer(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_location(IHTMLDocument2 *iface, IHTMLLocation **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_lastModified(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_URL(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_URL(IHTMLDocument2 *iface, BSTR *p)
{
    CHECK_EXPECT2(htmldoc2_get_url);
    *p = SysAllocString(NULL);
    return S_OK;
}

static HRESULT WINAPI htmldoc2_put_domain(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_domain(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_cookie(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_cookie(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_expando(IHTMLDocument2 *iface, VARIANT_BOOL v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_expando(IHTMLDocument2 *iface, VARIANT_BOOL *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_charset(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_charset(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_defaultCharset(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_defaultCharset(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_mimeType(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fileSize(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fileCreatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fileModifiedDate(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fileUpdatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_security(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_protocol(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_nameProp(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_write(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_writeln(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_open(IHTMLDocument2 *iface, BSTR url, VARIANT name,
                        VARIANT features, VARIANT replace, IDispatch **pomWindowResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_close(IHTMLDocument2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_clear(IHTMLDocument2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandSupported(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandEnabled(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandState(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandIndeterm(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandText(IHTMLDocument2 *iface, BSTR cmdID,
                                                        BSTR *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandValue(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_execCommand(IHTMLDocument2 *iface, BSTR cmdID,
                                VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_execCommandShowHelp(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_createElement(IHTMLDocument2 *iface, BSTR eTag,
                                                 IHTMLElement **newElem)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onhelp(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onhelp(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onclick(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onclick(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_ondblclick(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_ondblclick(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onkeyup(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onkeyup(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onkeydown(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onkeydown(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onkeypress(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onkeypress(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmouseup(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmouseup(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmousedown(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmousedown(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmousemove(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmousemove(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmouseout(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmouseout(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmouseover(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmouseover(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onreadystatechange(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onreadystatechange(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onafterupdate(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onafterupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onrowexit(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onrowexit(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onrowenter(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onrowenter(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_ondragstart(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_ondragstart(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onselectstart(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onselectstart(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_elementFromPoint(IHTMLDocument2 *iface, LONG x, LONG y,
                                                        IHTMLElement **elementHit)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_parentWindow(IHTMLDocument2 *iface, IHTMLWindow2 **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_styleSheets(IHTMLDocument2 *iface,
                                                   IHTMLStyleSheetsCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onbeforeupdate(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onbeforeupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onerrorupdate(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onerrorupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_toString(IHTMLDocument2 *iface, BSTR *String)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_createStyleSheet(IHTMLDocument2 *iface, BSTR bstrHref,
                                            LONG lIndex, IHTMLStyleSheet **ppnewStyleSheet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IHTMLDocument2Vtbl TestHTMLDocumentVtbl = {
    htmldoc2_QueryInterface,
    htmldoc2_AddRef,
    htmldoc2_Release,
    htmldoc2_GetTypeInfoCount,
    htmldoc2_GetTypeInfo,
    htmldoc2_GetIDsOfNames,
    htmldoc2_Invoke,
    htmldoc2_get_Script,
    htmldoc2_get_all,
    htmldoc2_get_body,
    htmldoc2_get_activeElement,
    htmldoc2_get_images,
    htmldoc2_get_applets,
    htmldoc2_get_links,
    htmldoc2_get_forms,
    htmldoc2_get_anchors,
    htmldoc2_put_title,
    htmldoc2_get_title,
    htmldoc2_get_scripts,
    htmldoc2_put_designMode,
    htmldoc2_get_designMode,
    htmldoc2_get_selection,
    htmldoc2_get_readyState,
    htmldoc2_get_frames,
    htmldoc2_get_embeds,
    htmldoc2_get_plugins,
    htmldoc2_put_alinkColor,
    htmldoc2_get_alinkColor,
    htmldoc2_put_bgColor,
    htmldoc2_get_bgColor,
    htmldoc2_put_fgColor,
    htmldoc2_get_fgColor,
    htmldoc2_put_linkColor,
    htmldoc2_get_linkColor,
    htmldoc2_put_vlinkColor,
    htmldoc2_get_vlinkColor,
    htmldoc2_get_referrer,
    htmldoc2_get_location,
    htmldoc2_get_lastModified,
    htmldoc2_put_URL,
    htmldoc2_get_URL,
    htmldoc2_put_domain,
    htmldoc2_get_domain,
    htmldoc2_put_cookie,
    htmldoc2_get_cookie,
    htmldoc2_put_expando,
    htmldoc2_get_expando,
    htmldoc2_put_charset,
    htmldoc2_get_charset,
    htmldoc2_put_defaultCharset,
    htmldoc2_get_defaultCharset,
    htmldoc2_get_mimeType,
    htmldoc2_get_fileSize,
    htmldoc2_get_fileCreatedDate,
    htmldoc2_get_fileModifiedDate,
    htmldoc2_get_fileUpdatedDate,
    htmldoc2_get_security,
    htmldoc2_get_protocol,
    htmldoc2_get_nameProp,
    htmldoc2_write,
    htmldoc2_writeln,
    htmldoc2_open,
    htmldoc2_close,
    htmldoc2_clear,
    htmldoc2_queryCommandSupported,
    htmldoc2_queryCommandEnabled,
    htmldoc2_queryCommandState,
    htmldoc2_queryCommandIndeterm,
    htmldoc2_queryCommandText,
    htmldoc2_queryCommandValue,
    htmldoc2_execCommand,
    htmldoc2_execCommandShowHelp,
    htmldoc2_createElement,
    htmldoc2_put_onhelp,
    htmldoc2_get_onhelp,
    htmldoc2_put_onclick,
    htmldoc2_get_onclick,
    htmldoc2_put_ondblclick,
    htmldoc2_get_ondblclick,
    htmldoc2_put_onkeyup,
    htmldoc2_get_onkeyup,
    htmldoc2_put_onkeydown,
    htmldoc2_get_onkeydown,
    htmldoc2_put_onkeypress,
    htmldoc2_get_onkeypress,
    htmldoc2_put_onmouseup,
    htmldoc2_get_onmouseup,
    htmldoc2_put_onmousedown,
    htmldoc2_get_onmousedown,
    htmldoc2_put_onmousemove,
    htmldoc2_get_onmousemove,
    htmldoc2_put_onmouseout,
    htmldoc2_get_onmouseout,
    htmldoc2_put_onmouseover,
    htmldoc2_get_onmouseover,
    htmldoc2_put_onreadystatechange,
    htmldoc2_get_onreadystatechange,
    htmldoc2_put_onafterupdate,
    htmldoc2_get_onafterupdate,
    htmldoc2_put_onrowexit,
    htmldoc2_get_onrowexit,
    htmldoc2_put_onrowenter,
    htmldoc2_get_onrowenter,
    htmldoc2_put_ondragstart,
    htmldoc2_get_ondragstart,
    htmldoc2_put_onselectstart,
    htmldoc2_get_onselectstart,
    htmldoc2_elementFromPoint,
    htmldoc2_get_parentWindow,
    htmldoc2_get_styleSheets,
    htmldoc2_put_onbeforeupdate,
    htmldoc2_get_onbeforeupdate,
    htmldoc2_put_onerrorupdate,
    htmldoc2_get_onerrorupdate,
    htmldoc2_toString,
    htmldoc2_createStyleSheet
};

typedef struct
{
    IHTMLDocument2 IHTMLDocument2_iface;
} testhtmldoc2_t;

static testhtmldoc2_t htmldoc2 = { { &TestHTMLDocumentVtbl } };

static HRESULT WINAPI sp_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IServiceProvider))
    {
        *ppvObject = iface;
        IServiceProvider_AddRef(iface);
        return S_OK;
    }

    ok(0, "unexpected query interface: %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI sp_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI sp_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI sp_QueryService(IServiceProvider *iface, REFGUID service, REFIID riid, void **obj)
{
    *obj = NULL;

    if (IsEqualGUID(service, &SID_SBindHost) &&
        IsEqualGUID(riid, &IID_IBindHost))
    {
        CHECK_EXPECT2(sp_queryservice_SID_SBindHost);
    }
    else if (IsEqualGUID(service, &SID_SContainerDispatch) &&
             IsEqualGUID(riid, &IID_IHTMLDocument2))
    {
        CHECK_EXPECT2(sp_queryservice_SID_SContainerDispatch_htmldoc2);
    }
    else if (IsEqualGUID(service, &SID_SInternetHostSecurityManager) &&
             IsEqualGUID(riid, &IID_IHTMLDocument2))
    {
        CHECK_EXPECT2(sp_queryservice_SID_secmgr_htmldoc2);
        *obj = &htmldoc2.IHTMLDocument2_iface;
        return S_OK;
    }
    else if (IsEqualGUID(service, &SID_SInternetHostSecurityManager) &&
             IsEqualGUID(riid, &IID_IXMLDOMDocument))
    {
        CHECK_EXPECT2(sp_queryservice_SID_secmgr_xmldomdoc);
    }
    else if (IsEqualGUID(service, &SID_SInternetHostSecurityManager) &&
             IsEqualGUID(riid, &IID_IInternetHostSecurityManager))
    {
        CHECK_EXPECT2(sp_queryservice_SID_secmgr_secmgr);
    }
    else if (IsEqualGUID(service, &SID_UnknownSID) &&
             IsEqualGUID(riid, &IID_IStream))
    {
        /* FIXME: unidentified service id */
    }
    else
        ok(0, "unexpected request: sid %s, riid %s\n", debugstr_guid(service), debugstr_guid(riid));

    return E_NOTIMPL;
}

static const IServiceProviderVtbl testprovVtbl =
{
    sp_QueryInterface,
    sp_AddRef,
    sp_Release,
    sp_QueryService
};

testprov_t testprov = { { &testprovVtbl } };

#define EXPECT_CHILDREN(node) _expect_children((IXMLDOMNode*)node, __LINE__)
static void _expect_children(IXMLDOMNode *node, int line)
{
    VARIANT_BOOL b;
    HRESULT hr;

    b = VARIANT_FALSE;
    hr = IXMLDOMNode_hasChildNodes(node, &b);
    ok_(__FILE__,line)(hr == S_OK, "hasChildNodes() failed, 0x%08x\n", hr);
    ok_(__FILE__,line)(b == VARIANT_TRUE, "no children, %d\n", b);
}

#define EXPECT_NO_CHILDREN(node) _expect_no_children((IXMLDOMNode*)node, __LINE__)
static void _expect_no_children(IXMLDOMNode *node, int line)
{
    VARIANT_BOOL b;
    HRESULT hr;

    b = VARIANT_TRUE;
    hr = IXMLDOMNode_hasChildNodes(node, &b);
    ok_(__FILE__,line)(hr == S_FALSE, "hasChildNodes() failed, 0x%08x\n", hr);
    ok_(__FILE__,line)(b == VARIANT_FALSE, "no children, %d\n", b);
}

#define EXPECT_REF(node,ref) _expect_ref((IUnknown*)node, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc = IUnknown_AddRef(obj);
    IUnknown_Release(obj);
    ok_(__FILE__,line)(rc-1 == ref, "expected refcount %d, got %d\n", ref, rc-1);
}

#define EXPECT_LIST_LEN(list,len) _expect_list_len(list, len, __LINE__)
static void _expect_list_len(IXMLDOMNodeList *list, LONG len, int line)
{
    LONG length;
    HRESULT hr;

    length = 0;
    hr = IXMLDOMNodeList_get_length(list, &length);
    ok_(__FILE__,line)(hr == S_OK, "got 0x%08x\n", hr);
    ok_(__FILE__,line)(length == len, "got %d, expected %d\n", length, len);
}

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

static const WCHAR szEmpty[] = { 0 };
static const WCHAR szIncomplete[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',0
};
static const WCHAR szComplete1[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','o','p','e','n','>','<','/','o','p','e','n','>','\n',0
};
static const WCHAR szComplete2[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','o','>','<','/','o','>','\n',0
};
static const WCHAR szComplete3[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','a','>','<','/','a','>','\n',0
};
static const char complete4A[] =
    "<?xml version=\'1.0\'?>\n"
    "<lc dl=\'str1\'>\n"
        "<bs vr=\'str2\' sz=\'1234\'>"
            "fn1.txt\n"
        "</bs>\n"
        "<pr id=\'str3\' vr=\'1.2.3\' pn=\'wine 20050804\'>\n"
            "fn2.txt\n"
        "</pr>\n"
        "<empty></empty>\n"
        "<fo>\n"
            "<ba>\n"
                "f1\n"
            "</ba>\n"
        "</fo>\n"
    "</lc>\n";

static const WCHAR szComplete5[] = {
    '<','S',':','s','e','a','r','c','h',' ','x','m','l','n','s',':','D','=','"','D','A','V',':','"',' ',
    'x','m','l','n','s',':','C','=','"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','o','f','f','i','c','e',':','c','l','i','p','g','a','l','l','e','r','y','"',
    ' ','x','m','l','n','s',':','S','=','"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','o','f','f','i','c','e',':','c','l','i','p','g','a','l','l','e','r','y',':','s','e','a','r','c','h','"','>',
        '<','S',':','s','c','o','p','e','>',
            '<','S',':','d','e','e','p','>','/','<','/','S',':','d','e','e','p','>',
        '<','/','S',':','s','c','o','p','e','>',
        '<','S',':','c','o','n','t','e','n','t','f','r','e','e','t','e','x','t','>',
            '<','C',':','t','e','x','t','o','r','p','r','o','p','e','r','t','y','/','>',
            'c','o','m','p','u','t','e','r',
        '<','/','S',':','c','o','n','t','e','n','t','f','r','e','e','t','e','x','t','>',
    '<','/','S',':','s','e','a','r','c','h','>',0
};

static const WCHAR szComplete6[] = {
    '<','?','x','m','l',' ','v','e','r','s','i','o','n','=','\'','1','.','0','\'',' ',
    'e','n','c','o','d','i','n','g','=','\'','W','i','n','d','o','w','s','-','1','2','5','2','\'','?','>','\n',
    '<','o','p','e','n','>','<','/','o','p','e','n','>','\n',0
};

static const CHAR szNonUnicodeXML[] =
"<?xml version='1.0' encoding='Windows-1252'?>\n"
"<open></open>\n";

static const char szExampleXML[] =
"<?xml version='1.0' encoding='utf-8'?>\n"
"<root xmlns:foo='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' a=\"attr a\" foo:b=\"attr b\" >\n"
"    <elem>\n"
"        <a>A1 field</a>\n"
"        <b>B1 field</b>\n"
"        <c>C1 field</c>\n"
"        <description xmlns:foo='http://www.winehq.org' xmlns:bar='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"            <html xmlns='http://www.w3.org/1999/xhtml'>\n"
"                This is <strong>a</strong> <i>description</i>. <bar:x/>\n"
"            </html>\n"
"            <html xml:space='preserve' xmlns='http://www.w3.org/1999/xhtml'>\n"
"                This is <strong>a</strong> <i>description</i> with preserved whitespace. <bar:x/>\n"
"            </html>\n"
"        </description>\n"
"    </elem>\n"
"\n"
"    <elem>\n"
"        <a>A2 field</a>\n"
"        <b>B2 field</b>\n"
"        <c type=\"old\">C2 field</c>\n"
"    </elem>\n"
"\n"
"    <elem xmlns='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"        <a>A3 field</a>\n"
"        <b>B3 field</b>\n"
"        <c>C3 field</c>\n"
"    </elem>\n"
"\n"
"    <elem>\n"
"        <a>A4 field</a>\n"
"        <b>B4 field</b>\n"
"        <foo:c>C4 field</foo:c>\n"
"    </elem>\n"
"</root>\n";

static const CHAR szNodeTypesXML[] =
"<?xml version='1.0'?>"
"<!-- comment node 0 -->"
"<root id='0' depth='0'>"
"   <!-- comment node 1 -->"
"   text node 0"
"   <x id='1' depth='1'>"
"       <?foo value='PI for x'?>"
"       <!-- comment node 2 -->"
"       text node 1"
"       <a id='3' depth='2'/>"
"       <b id='4' depth='2'/>"
"       <c id='5' depth='2'/>"
"   </x>"
"   <y id='2' depth='1'>"
"       <?bar value='PI for y'?>"
"       <!-- comment node 3 -->"
"       text node 2"
"       <a id='6' depth='2'/>"
"       <b id='7' depth='2'/>"
"       <c id='8' depth='2'/>"
"   </y>"
"</root>";

static const CHAR szTransformXML[] =
"<?xml version=\"1.0\"?>\n"
"<greeting>\n"
"Hello World\n"
"</greeting>";

static  const CHAR szTransformSSXML[] =
"<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\n"
"   <xsl:output method=\"html\"/>\n"
"   <xsl:template match=\"/\">\n"
"       <xsl:apply-templates select=\"greeting\"/>\n"
"   </xsl:template>\n"
"   <xsl:template match=\"greeting\">\n"
"       <html>\n"
"           <body>\n"
"               <h1>\n"
"                   <xsl:value-of select=\".\"/>\n"
"               </h1>\n"
"           </body>\n"
"       </html>\n"
"   </xsl:template>\n"
"</xsl:stylesheet>";

static  const CHAR szTransformOutput[] =
"<html><body><h1>"
"Hello World"
"</h1></body></html>";

static const CHAR szTypeValueXML[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<root xmlns:dt=\"urn:schemas-microsoft-com:datatypes\">\n"
"   <string>Wine</string>\n"
"   <string2 dt:dt=\"string\">String</string2>\n"
"   <number dt:dt=\"number\">12.44</number>\n"
"   <number2 dt:dt=\"NUMbEr\">-3.71e3</number2>\n"
"   <int dt:dt=\"int\">-13</int>\n"
"   <fixed dt:dt=\"fixed.14.4\">7322.9371</fixed>\n"
"   <bool dt:dt=\"boolean\">1</bool>\n"
"   <datetime dt:dt=\"datetime\">2009-11-18T03:21:33.12</datetime>\n"
"   <datetimetz dt:dt=\"datetime.tz\">2003-07-11T11:13:57+03:00</datetimetz>\n"
"   <date dt:dt=\"date\">3721-11-01</date>\n"
"   <time dt:dt=\"time\">13:57:12.31321</time>\n"
"   <timetz dt:dt=\"time.tz\">23:21:01.13+03:21</timetz>\n"
"   <i1 dt:dt=\"i1\">-13</i1>\n"
"   <i2 dt:dt=\"i2\">31915</i2>\n"
"   <i4 dt:dt=\"i4\">-312232</i4>\n"
"   <ui1 dt:dt=\"ui1\">123</ui1>\n"
"   <ui2 dt:dt=\"ui2\">48282</ui2>\n"
"   <ui4 dt:dt=\"ui4\">949281</ui4>\n"
"   <r4 dt:dt=\"r4\">213124.0</r4>\n"
"   <r8 dt:dt=\"r8\">0.412</r8>\n"
"   <float dt:dt=\"float\">41221.421</float>\n"
"   <uuid dt:dt=\"uuid\">333C7BC4-460F-11D0-BC04-0080C7055a83</uuid>\n"
"   <binhex dt:dt=\"bin.hex\">fffca012003c</binhex>\n"
"   <binbase64 dt:dt=\"bin.base64\">YmFzZTY0IHRlc3Q=</binbase64>\n"
"   <binbase64_1 dt:dt=\"bin.base64\">\nYmFzZTY0\nIHRlc3Q=\n</binbase64_1>\n"
"   <binbase64_2 dt:dt=\"bin.base64\">\nYmF\r\t z  ZTY0\nIHRlc3Q=\n</binbase64_2>\n"
"</root>";

static const CHAR szBasicTransformSSXMLPart1[] =
"<?xml version=\"1.0\"?>"
"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" >"
"<xsl:output method=\"html\"/>\n"
"<xsl:template match=\"/\">"
"<HTML><BODY><TABLE>"
"        <xsl:apply-templates select='document(\"";

static const CHAR szBasicTransformSSXMLPart2[] =
"\")/bottle/wine'>"
"           <xsl:sort select=\"cost\"/><xsl:sort select=\"name\"/>"
"        </xsl:apply-templates>"
"</TABLE></BODY></HTML>"
"</xsl:template>"
"<xsl:template match=\"bottle\">"
"   <TR><xsl:apply-templates select=\"name\" /><xsl:apply-templates select=\"cost\" /></TR>"
"</xsl:template>"
"<xsl:template match=\"name\">"
"   <TD><xsl:apply-templates /></TD>"
"</xsl:template>"
"<xsl:template match=\"cost\">"
"   <TD><xsl:apply-templates /></TD>"
"</xsl:template>"
"</xsl:stylesheet>";

static const CHAR szBasicTransformXML[] =
"<?xml version=\"1.0\"?><bottle><wine><name>Wine</name><cost>$25.00</cost></wine></bottle>";

static const CHAR szBasicTransformOutput[] =
"<HTML><BODY><TABLE><TD>Wine</TD><TD>$25.00</TD></TABLE></BODY></HTML>";

#define SZ_EMAIL_DTD \
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"

static const CHAR szEmailXML[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 87)</subject>"
"   <body>"
"       It no longer causes spontaneous combustion..."
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_0D[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 88)</subject>"
"   <body>"
"       <undecl />"
"       XML_ELEMENT_UNDECLARED 0xC00CE00D"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_0E[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 89)</subject>"
"   <body>"
"       XML_ELEMENT_ID_NOT_FOUND 0xC00CE00E"
"   </body>"
"   <attachment id=\"patch\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_11[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 90)</subject>"
"   <body>"
"       XML_EMPTY_NOT_ALLOWED 0xC00CE011"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_13[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<msg attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 91)</subject>"
"   <body>"
"       XML_ROOT_NAME_MISMATCH 0xC00CE013"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</msg>";

static const CHAR szEmailXML_14[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <to>wine-patches@winehq.org</to>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 92)</subject>"
"   <body>"
"       XML_INVALID_CONTENT 0xC00CE014"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_15[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\" ip=\"127.0.0.1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 93)</subject>"
"   <body>"
"       XML_ATTRIBUTE_NOT_DEFINED 0xC00CE015"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_16[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 94)</subject>"
"   <body enc=\"ASCII\">"
"       XML_ATTRIBUTE_FIXED 0xC00CE016"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_17[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\" sent=\"true\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 95)</subject>"
"   <body>"
"       XML_ATTRIBUTE_VALUE 0xC00CE017"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_18[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   oops"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 96)</subject>"
"   <body>"
"       XML_ILLEGAL_TEXT 0xC00CE018"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_20[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email>"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 97)</subject>"
"   <body>"
"       XML_REQUIRED_ATTRIBUTE_MISSING 0xC00CE020"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const char xpath_simple_list[] =
"<?xml version=\"1.0\"?>"
"<root>"
"   <a attr1=\"1\" attr2=\"2\" />"
"   <b/>"
"   <c/>"
"   <d/>"
"</root>";

static const char* leading_spaces[] = {
    "\n<?xml version=\"1.0\"?><root/>",
    " <?xml version=\"1.0\"?><root/>",
    "\t<?xml version=\"1.0\"?><root/>",
    "\r\n<?xml version=\"1.0\"?><root/>",
    "\r<?xml version=\"1.0\"?><root/>",
    0
};

static const char default_ns_doc[] = {
    "<?xml version=\"1.0\"?>"
    "<a xmlns:ns=\"nshref\" xml:lang=\"ru\" ns:b=\"b attr\" xml:c=\"c attr\" "
    "    d=\"d attr\" />"
};

static const WCHAR nonexistent_fileW[] = {
    'c', ':', '\\', 'N', 'o', 'n', 'e', 'x', 'i', 's', 't', 'e', 'n', 't', '.', 'x', 'm', 'l', 0
};
static const WCHAR nonexistent_attrW[] = {
    'n','o','n','E','x','i','s','i','t','i','n','g','A','t','t','r','i','b','u','t','e',0
};
static const WCHAR szDocument[] = {
    '#', 'd', 'o', 'c', 'u', 'm', 'e', 'n', 't', 0
};

static const WCHAR szOpen[] = { 'o','p','e','n',0 };
static WCHAR szdl[] = { 'd','l',0 };
static const WCHAR szvr[] = { 'v','r',0 };
static const WCHAR szlc[] = { 'l','c',0 };
static WCHAR szbs[] = { 'b','s',0 };
static const WCHAR szstr1[] = { 's','t','r','1',0 };
static const WCHAR szstr2[] = { 's','t','r','2',0 };
static const WCHAR szstar[] = { '*',0 };
static const WCHAR szfn1_txt[] = {'f','n','1','.','t','x','t',0};

static WCHAR szComment[] = {'A',' ','C','o','m','m','e','n','t',0 };
static WCHAR szCommentXML[] = {'<','!','-','-','A',' ','C','o','m','m','e','n','t','-','-','>',0 };
static WCHAR szCommentNodeText[] = {'#','c','o','m','m','e','n','t',0 };

static WCHAR szElement[] = {'E','l','e','T','e','s','t', 0 };
static WCHAR szElementXML[]  = {'<','E','l','e','T','e','s','t','/','>',0 };
static WCHAR szElementXML2[] = {'<','E','l','e','T','e','s','t',' ','A','t','t','r','=','"','"','/','>',0 };
static WCHAR szElementXML3[] = {'<','E','l','e','T','e','s','t',' ','A','t','t','r','=','"','"','>',
                                'T','e','s','t','i','n','g','N','o','d','e','<','/','E','l','e','T','e','s','t','>',0 };
static WCHAR szElementXML4[] = {'<','E','l','e','T','e','s','t',' ','A','t','t','r','=','"','"','>',
                                '&','a','m','p',';','x',' ',0x2103,'<','/','E','l','e','T','e','s','t','>',0 };

static WCHAR szAttribute[] = {'A','t','t','r',0 };
static WCHAR szAttributeXML[] = {'A','t','t','r','=','"','"',0 };

static WCHAR szCData[] = {'[','1',']','*','2','=','3',';',' ','&','g','e','e',' ','t','h','a','t','s',
                          ' ','n','o','t',' ','r','i','g','h','t','!', 0};
static WCHAR szCDataXML[] = {'<','!','[','C','D','A','T','A','[','[','1',']','*','2','=','3',';',' ','&',
                             'g','e','e',' ','t','h','a','t','s',' ','n','o','t',' ','r','i','g','h','t',
                             '!',']',']','>',0};
static WCHAR szCDataNodeText[] = {'#','c','d','a','t','a','-','s','e','c','t','i','o','n',0 };
static WCHAR szDocFragmentText[] = {'#','d','o','c','u','m','e','n','t','-','f','r','a','g','m','e','n','t',0 };

static WCHAR szEntityRef[] = {'e','n','t','i','t','y','r','e','f',0 };
static WCHAR szEntityRefXML[] = {'&','e','n','t','i','t','y','r','e','f',';',0 };
static WCHAR szStrangeChars[] = {'&','x',' ',0x2103, 0};

#define expect_bstr_eq_and_free(bstr, expect) { \
    BSTR bstrExp = alloc_str_from_narrow(expect); \
    ok(lstrcmpW(bstr, bstrExp) == 0, "String differs\n"); \
    SysFreeString(bstr); \
    SysFreeString(bstrExp); \
}

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }

#define ole_check(expr) { \
    HRESULT r = expr; \
    ok(r == S_OK, #expr " returned %x\n", r); \
}

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %x, expected %x\n", r, expect); \
}

#define double_eq(x, y) ok((x)-(y)<=1e-14*(x) && (x)-(y)>=-1e-14*(x), "expected %.16g, got %.16g\n", x, y)

static void* _create_object(const GUID *clsid, const char *name, const IID *iid, int line)
{
    void *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, &obj);
    if (hr != S_OK)
        win_skip_(__FILE__,line)("failed to create %s instance: 0x%08x\n", name, hr);

    return obj;
}

#define _create(cls) cls, #cls

#define create_document(iid) _create_object(&_create(CLSID_DOMDocument), iid, __LINE__)
#define create_document_version(v, iid) _create_object(&_create(CLSID_DOMDocument ## v), iid, __LINE__)
#define create_cache(iid) _create_object(&_create(CLSID_XMLSchemaCache), iid, __LINE__)
#define create_cache_version(v, iid) _create_object(&_create(CLSID_XMLSchemaCache ## v), iid, __LINE__)
#define create_xsltemplate(iid) _create_object(&_create(CLSID_XSLTemplate), iid, __LINE__)

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static BSTR alloced_bstrs[256];
static int alloced_bstrs_count;

static BSTR _bstr_(const char *str)
{
    assert(alloced_bstrs_count < sizeof(alloced_bstrs)/sizeof(alloced_bstrs[0]));
    alloced_bstrs[alloced_bstrs_count] = alloc_str_from_narrow(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs(void)
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}

static VARIANT _variantbstr_(const char *str)
{
    VARIANT v;
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_(str);
    return v;
}

static BOOL compareIgnoreReturns(BSTR sLeft, BSTR sRight)
{
    for (;;)
    {
        while (*sLeft == '\r' || *sLeft == '\n') sLeft++;
        while (*sRight == '\r' || *sRight == '\n') sRight++;
        if (*sLeft != *sRight) return FALSE;
        if (!*sLeft) return TRUE;
        sLeft++;
        sRight++;
    }
}

static void get_str_for_type(DOMNodeType type, char *buf)
{
    switch (type)
    {
        case NODE_ATTRIBUTE:
            strcpy(buf, "A");
            break;
        case NODE_ELEMENT:
            strcpy(buf, "E");
            break;
        case NODE_DOCUMENT:
            strcpy(buf, "D");
            break;
        case NODE_TEXT:
            strcpy(buf, "T");
            break;
        case NODE_COMMENT:
            strcpy(buf, "C");
            break;
        case NODE_PROCESSING_INSTRUCTION:
            strcpy(buf, "P");
            break;
        default:
            wsprintfA(buf, "[%d]", type);
    }
}

static int get_node_position(IXMLDOMNode *node)
{
    HRESULT r;
    int pos = 0;

    IXMLDOMNode_AddRef(node);
    do
    {
        IXMLDOMNode *new_node;

        pos++;
        r = IXMLDOMNode_get_previousSibling(node, &new_node);
        ok(SUCCEEDED(r), "get_previousSibling failed\n");
        IXMLDOMNode_Release(node);
        node = new_node;
    } while (r == S_OK);
    return pos;
}

static void node_to_string(IXMLDOMNode *node, char *buf)
{
    HRESULT r = S_OK;
    DOMNodeType type;

    if (node == NULL)
    {
        lstrcpyA(buf, "(null)");
        return;
    }

    IXMLDOMNode_AddRef(node);
    while (r == S_OK)
    {
        IXMLDOMNode *new_node;

        ole_check(IXMLDOMNode_get_nodeType(node, &type));
        get_str_for_type(type, buf);
        buf+=strlen(buf);

        if (type == NODE_ATTRIBUTE)
        {
            BSTR bstr;
            ole_check(IXMLDOMNode_get_nodeName(node, &bstr));
            *(buf++) = '\'';
            wsprintfA(buf, "%ws", bstr);
            buf += strlen(buf);
            *(buf++) = '\'';
            SysFreeString(bstr);

            r = IXMLDOMNode_selectSingleNode(node, _bstr_(".."), &new_node);
        }
        else
        {
            r = IXMLDOMNode_get_parentNode(node, &new_node);
            wsprintf(buf, "%d", get_node_position(node));
            buf += strlen(buf);
        }

        ok(SUCCEEDED(r), "get_parentNode failed (%08x)\n", r);
        IXMLDOMNode_Release(node);
        node = new_node;
        if (r == S_OK)
            *(buf++) = '.';
    }

    *buf = 0;
}

static char *list_to_string(IXMLDOMNodeList *list)
{
    static char buf[4096];
    char *pos = buf;
    LONG len = 0;
    int i;

    if (list == NULL)
    {
        lstrcpyA(buf, "(null)");
        return buf;
    }
    ole_check(IXMLDOMNodeList_get_length(list, &len));
    for (i = 0; i < len; i++)
    {
        IXMLDOMNode *node;
        if (i > 0)
            *(pos++) = ' ';
        ole_check(IXMLDOMNodeList_nextNode(list, &node));
        node_to_string(node, pos);
        pos += strlen(pos);
        IXMLDOMNode_Release(node);
    }
    *pos = 0;
    return buf;
}

#define expect_node(node, expstr) { char str[4096]; node_to_string(node, str); ok(strcmp(str, expstr)==0, "Invalid node: %s, expected %s\n", str, expstr); }
#define expect_list_and_release(list, expstr) { char *str = list_to_string(list); ok(strcmp(str, expstr)==0, "Invalid node list: %s, expected %s\n", str, expstr); if (list) IXMLDOMNodeList_Release(list); }

static void test_domdoc( void )
{
    HRESULT r, hr;
    IXMLDOMDocument *doc;
    IXMLDOMParseError *error;
    IXMLDOMElement *element = NULL;
    IXMLDOMNode *node;
    IXMLDOMText *nodetext = NULL;
    IXMLDOMComment *node_comment = NULL;
    IXMLDOMAttribute *node_attr = NULL;
    IXMLDOMNode *nodeChild = NULL;
    IXMLDOMProcessingInstruction *nodePI = NULL;
    ISupportErrorInfo *support_error = NULL;
    VARIANT_BOOL b;
    VARIANT var;
    BSTR str;
    LONG code;
    LONG nLength = 0;
    WCHAR buff[100];
    const char **ptr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

if (0)
{
    /* crashes on native */
    IXMLDOMDocument_loadXML( doc, (BSTR)0x1, NULL );
}

    /* try some stupid things */
    hr = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    EXPECT_HR(hr, S_FALSE);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, NULL, &b );
    EXPECT_HR(hr, S_FALSE);
    ok( b == VARIANT_FALSE, "failed to load XML string\n");

    /* load document with leading spaces */
    ptr = leading_spaces;
    while (*ptr)
    {
        b = VARIANT_TRUE;
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_(*ptr);
        hr = IXMLDOMDocument_load( doc, var, &b);
        EXPECT_HR(hr, S_FALSE);
        ok( b == VARIANT_FALSE, "got %x\n", b);
        ptr++;
    }

    /* try to load a document from a nonexistent file */
    b = VARIANT_TRUE;
    str = SysAllocString( nonexistent_fileW );
    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = str;

    r = IXMLDOMDocument_load( doc, var, &b);
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* try load an empty document */
    b = VARIANT_TRUE;
    str = SysAllocString( szEmpty );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_async( doc, &b );
    ok( r == S_OK, "get_async failed (%08x)\n", r);
    ok( b == VARIANT_TRUE, "Wrong default value\n");

    /* check that there's no document element */
    element = NULL;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");

    /* try finding a node */
    node = NULL;
    str = SysAllocString( szstr1 );
    r = IXMLDOMDocument_selectSingleNode( doc, str, &node );
    ok( r == S_FALSE, "ret %08x\n", r );
    SysFreeString( str );

    b = VARIANT_TRUE;
    str = SysAllocString( szIncomplete );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* check that there's no document element */
    element = (IXMLDOMElement*)1;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");
    ok( element == NULL, "Element should be NULL\n");

    /* test for BSTR handling, pass broken BSTR */
    memcpy(&buff[2], szComplete1, sizeof(szComplete1));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    b = VARIANT_FALSE;
    r = IXMLDOMDocument_loadXML( doc, &buff[2], &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    /* loadXML ignores the encoding attribute and always expects Unicode */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete6 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* try a BSTR containing a Windows-1252 document */
    b = VARIANT_TRUE;
    str = SysAllocStringByteLen( szNonUnicodeXML, sizeof(szNonUnicodeXML) - 1 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* try to load something valid */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete1 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* check if nodename is correct */
    r = IXMLDOMDocument_get_nodeName( doc, NULL );
    ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

    str = (BSTR)0xdeadbeef;
    r = IXMLDOMDocument_get_baseName( doc, &str );
    ok ( r == S_FALSE, "got 0x%08x\n", r);
    ok (str == NULL, "got %p\n", str);

    /* content doesn't matter here */
    str = NULL;
    r = IXMLDOMDocument_get_nodeName( doc, &str );
    ok ( r == S_OK, "get_nodeName wrong code\n");
    ok ( str != NULL, "str is null\n");
    ok( !lstrcmpW( str, szDocument ), "incorrect nodeName\n");
    SysFreeString( str );

    /* test put_text */
    r = IXMLDOMDocument_put_text( doc, _bstr_("Should Fail") );
    ok( r == E_FAIL, "ret %08x\n", r );

    /* check that there's a document element */
    element = NULL;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    if( element )
    {
        IObjectIdentity *ident;

        r = IXMLDOMElement_QueryInterface( element, &IID_IObjectIdentity, (void**)&ident );
        ok( r == E_NOINTERFACE, "ret %08x\n", r);

        IXMLDOMElement_Release( element );
        element = NULL;
    }

    /* as soon as we call loadXML again, the document element will disappear */
    b = 2;
    r = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    ok( r == S_FALSE, "loadXML failed\n");
    ok( b == 2, "variant modified\n");
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");

    /* try to load something else simple and valid */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete3 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* try something a little more complicated */
    b = FALSE;
    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    r = IXMLDOMDocument_get_parseError( doc, &error );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMParseError_get_errorCode( error, &code );
    ok( r == S_FALSE, "returns %08x\n", r );
    ok( code == 0, "code %d\n", code );
    IXMLDOMParseError_Release( error );

    /* test createTextNode */
    r = IXMLDOMDocument_createTextNode(doc, _bstr_(""), &nodetext);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMText_Release(nodetext);

    str = SysAllocString( szOpen );
    r = IXMLDOMDocument_createTextNode(doc, str, NULL);
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    r = IXMLDOMDocument_createTextNode(doc, str, &nodetext);
    ok( r == S_OK, "returns %08x\n", r );
    SysFreeString( str );
    if(nodetext)
    {
        r = IXMLDOMText_QueryInterface(nodetext, &IID_IXMLDOMElement, (void**)&element);
        ok(r == E_NOINTERFACE, "ret %08x\n", r );

        /* Text Last Child Checks */
        r = IXMLDOMText_get_lastChild(nodetext, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        nodeChild = (IXMLDOMNode*)0x1;
        r = IXMLDOMText_get_lastChild(nodetext, &nodeChild);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok(nodeChild == NULL, "nodeChild not NULL\n");

        /* test length property */
        r = IXMLDOMText_get_length(nodetext, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 4, "expected 4 got %d\n", nLength);

        /* put data Tests */
        r = IXMLDOMText_put_data(nodetext, _bstr_("This &is a ; test <>\\"));
        ok(r == S_OK, "ret %08x\n", r );

        /* get data Tests */
        r = IXMLDOMText_get_data(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect put_data string\n");
        SysFreeString(str);

        /* Confirm XML text is good */
        r = IXMLDOMText_get_xml(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &amp;is a ; test &lt;&gt;\\") ), "incorrect xml string\n");
        SysFreeString(str);

        /* Confirm we get the put_data Text back */
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect xml string\n");
        SysFreeString(str);

        /* test substringData */
        r = IXMLDOMText_substringData(nodetext, 0, 4, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        /* test substringData - Invalid offset */
        str = (BSTR)&szElement;
        r = IXMLDOMText_substringData(nodetext, -1, 4, &str);
        ok(r == E_INVALIDARG, "ret %08x\n", r );
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid offset */
        str = (BSTR)&szElement;
        r = IXMLDOMText_substringData(nodetext, 30, 0, &str);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid size */
        str = (BSTR)&szElement;
        r = IXMLDOMText_substringData(nodetext, 0, -1, &str);
        ok(r == E_INVALIDARG, "ret %08x\n", r );
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid size */
        str = (BSTR)&szElement;
        r = IXMLDOMText_substringData(nodetext, 2, 0, &str);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Start of string */
        r = IXMLDOMText_substringData(nodetext, 0, 4, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This") ), "incorrect substringData string\n");
        SysFreeString(str);

        /* test substringData - Middle of string */
        r = IXMLDOMText_substringData(nodetext, 13, 4, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("test") ), "incorrect substringData string\n");
        SysFreeString(str);

        /* test substringData - End of string */
        r = IXMLDOMText_substringData(nodetext, 20, 4, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("\\") ), "incorrect substringData string\n");
        SysFreeString(str);

        /* test appendData */
        r = IXMLDOMText_appendData(nodetext, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_appendData(nodetext, _bstr_(""));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_appendData(nodetext, _bstr_("Append"));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\Append") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* test insertData */
        str = SysAllocStringLen(NULL, 0);
        r = IXMLDOMText_insertData(nodetext, -1, str);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, -1, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 1000, str);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 1000, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 0, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 0, str);
        ok(r == S_OK, "ret %08x\n", r );
        SysFreeString(str);

        r = IXMLDOMText_insertData(nodetext, -1, _bstr_("Inserting"));
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 1000, _bstr_("Inserting"));
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 0, _bstr_("Begin "));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 17, _bstr_("Middle"));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 39, _bstr_(" End"));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("Begin This &is a Middle; test <>\\Append End") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* delete data */
        /* invalid arguments */
        r = IXMLDOMText_deleteData(nodetext, -1, 1);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_deleteData(nodetext, 0, 0);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_deleteData(nodetext, 0, -1);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 43, "expected 43 got %d\n", nLength);

        r = IXMLDOMText_deleteData(nodetext, nLength, 1);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_deleteData(nodetext, nLength+1, 1);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        /* delete from start */
        r = IXMLDOMText_deleteData(nodetext, 0, 5);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 38, "expected 38 got %d\n", nLength);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &is a Middle; test <>\\Append End") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* delete from end */
        r = IXMLDOMText_deleteData(nodetext, 35, 3);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 35, "expected 35 got %d\n", nLength);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &is a Middle; test <>\\Append") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* delete from inside */
        r = IXMLDOMText_deleteData(nodetext, 1, 33);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 2, "expected 2 got %d\n", nLength);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* delete whole data ... */
        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_deleteData(nodetext, 0, nLength);
        ok(r == S_OK, "ret %08x\n", r );
        /* ... and try again with empty string */
        r = IXMLDOMText_deleteData(nodetext, 0, nLength);
        ok(r == S_OK, "ret %08x\n", r );

        /* test put_data */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(szstr1);
        r = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(r == S_OK, "ret %08x\n", r );
        VariantClear(&var);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, szstr1 ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* test put_data */
        V_VT(&var) = VT_I4;
        V_I4(&var) = 99;
        r = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(r == S_OK, "ret %08x\n", r );
        VariantClear(&var);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("99") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* ::replaceData() */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(szstr1);
        r = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(r == S_OK, "ret %08x\n", r );
        VariantClear(&var);

        r = IXMLDOMText_replaceData(nodetext, 6, 0, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        r = IXMLDOMText_replaceData(nodetext, 0, 0, NULL);
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* NULL pointer means delete */
        r = IXMLDOMText_replaceData(nodetext, 0, 1, NULL);
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("tr1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* empty string means delete */
        r = IXMLDOMText_replaceData(nodetext, 0, 1, _bstr_(""));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("r1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* zero count means insert */
        r = IXMLDOMText_replaceData(nodetext, 0, 0, _bstr_("a"));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("ar1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        r = IXMLDOMText_replaceData(nodetext, 0, 2, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 0, _bstr_("m"));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("m1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* nonempty string, count greater than its length */
        r = IXMLDOMText_replaceData(nodetext, 0, 2, _bstr_("a1.2"));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("a1.2") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* nonempty string, count less than its length */
        r = IXMLDOMText_replaceData(nodetext, 0, 1, _bstr_("wine"));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("wine1.2") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        IXMLDOMText_Release( nodetext );
    }

    /* test Create Comment */
    r = IXMLDOMDocument_createComment(doc, NULL, NULL);
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    node_comment = (IXMLDOMComment*)0x1;

    /* empty comment */
    r = IXMLDOMDocument_createComment(doc, _bstr_(""), &node_comment);
    ok( r == S_OK, "returns %08x\n", r );
    str = (BSTR)0x1;
    r = IXMLDOMComment_get_data(node_comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty string data\n");
    IXMLDOMComment_Release(node_comment);
    SysFreeString(str);

    r = IXMLDOMDocument_createComment(doc, NULL, &node_comment);
    ok( r == S_OK, "returns %08x\n", r );
    str = (BSTR)0x1;
    r = IXMLDOMComment_get_data(node_comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && (SysStringLen(str) == 0), "expected empty string data\n");
    IXMLDOMComment_Release(node_comment);
    SysFreeString(str);

    str = SysAllocString(szComment);
    r = IXMLDOMDocument_createComment(doc, str, &node_comment);
    SysFreeString(str);
    ok( r == S_OK, "returns %08x\n", r );
    if(node_comment)
    {
        /* Last Child Checks */
        r = IXMLDOMComment_get_lastChild(node_comment, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        nodeChild = (IXMLDOMNode*)0x1;
        r = IXMLDOMComment_get_lastChild(node_comment, &nodeChild);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok(nodeChild == NULL, "pLastChild not NULL\n");

        /* baseName */
        str = (BSTR)0xdeadbeef;
        IXMLDOMComment_get_baseName(node_comment, &str);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok(str == NULL, "Expected NULL\n");

        IXMLDOMComment_Release( node_comment );
    }

    /* test Create Attribute */
    str = SysAllocString(szAttribute);
    r = IXMLDOMDocument_createAttribute(doc, NULL, NULL);
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    r = IXMLDOMDocument_createAttribute(doc, str, &node_attr);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMText_Release( node_attr);
    SysFreeString(str);

    /* test Processing Instruction */
    str = SysAllocStringLen(NULL, 0);
    r = IXMLDOMDocument_createProcessingInstruction(doc, str, str, NULL);
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    r = IXMLDOMDocument_createProcessingInstruction(doc, NULL, str, &nodePI);
    ok( r == E_FAIL, "returns %08x\n", r );
    r = IXMLDOMDocument_createProcessingInstruction(doc, str, str, &nodePI);
    ok( r == E_FAIL, "returns %08x\n", r );
    SysFreeString(str);

    r = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\""), &nodePI);
    ok( r == S_OK, "returns %08x\n", r );
    if(nodePI)
    {
        /* Last Child Checks */
        r = IXMLDOMProcessingInstruction_get_lastChild(nodePI, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        nodeChild = (IXMLDOMNode*)0x1;
        r = IXMLDOMProcessingInstruction_get_lastChild(nodePI, &nodeChild);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok(nodeChild == NULL, "nodeChild not NULL\n");

        /* test nodeName */
        r = IXMLDOMProcessingInstruction_get_nodeName(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("xml") ), "incorrect nodeName string\n");
        SysFreeString(str);

        /* test baseName */
        str = (BSTR)0x1;
        r = IXMLDOMProcessingInstruction_get_baseName(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("xml") ), "incorrect nodeName string\n");
        SysFreeString(str);

        /* test Target */
        r = IXMLDOMProcessingInstruction_get_target(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("xml") ), "incorrect target string\n");
        SysFreeString(str);

        /* test get_nodeValue */
        r = IXMLDOMProcessingInstruction_get_nodeValue(nodePI, &var);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( V_BSTR(&var), _bstr_("version=\"1.0\"") ), "incorrect data string\n");
        VariantClear(&var);

        /* test get_data */
        r = IXMLDOMProcessingInstruction_get_data(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("version=\"1.0\"") ), "incorrect data string\n");
        SysFreeString(str);

        /* test put_data */
        r = IXMLDOMProcessingInstruction_put_data(nodePI, _bstr_("version=\"1.0\" encoding=\"UTF-8\""));
        ok(r == E_FAIL, "ret %08x\n", r );

        /* test put_data */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(szOpen);  /* Doesn't matter what the string is, cannot set an xml node. */
        r = IXMLDOMProcessingInstruction_put_nodeValue(nodePI, var);
        ok(r == E_FAIL, "ret %08x\n", r );
        VariantClear(&var);

        /* test get nodeName */
        r = IXMLDOMProcessingInstruction_get_nodeName(nodePI, &str);
        ok( !lstrcmpW( str, _bstr_("xml") ), "incorrect nodeName string\n");
        ok(r == S_OK, "ret %08x\n", r );
        SysFreeString(str);

        IXMLDOMProcessingInstruction_Release(nodePI);
    }

    r = IXMLDOMDocument_QueryInterface( doc, &IID_ISupportErrorInfo, (void**)&support_error );
    ok( r == S_OK, "ret %08x\n", r );
    if(r == S_OK)
    {
        r = ISupportErrorInfo_InterfaceSupportsErrorInfo( support_error, &IID_IXMLDOMDocument );
        todo_wine ok( r == S_OK, "ret %08x\n", r );
        ISupportErrorInfo_Release( support_error );
    }

    r = IXMLDOMDocument_Release( doc );
    ok( r == 0, "document ref count incorrect\n");

    free_bstrs();
}

static void test_persiststreaminit(void)
{
    IXMLDOMDocument *doc;
    IPersistStreamInit *streaminit;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&streaminit);
    ok( hr == S_OK, "failed with 0x%08x\n", hr );

    hr = IPersistStreamInit_InitNew(streaminit);
    ok( hr == S_OK, "failed with 0x%08x\n", hr );

    IXMLDOMDocument_Release(doc);
}

static void test_domnode( void )
{
    HRESULT r;
    IXMLDOMDocument *doc, *owner = NULL;
    IXMLDOMElement *element = NULL;
    IXMLDOMNamedNodeMap *map = NULL;
    IXMLDOMNode *node = NULL, *next = NULL;
    IXMLDOMNodeList *list = NULL;
    IXMLDOMAttribute *attr = NULL;
    DOMNodeType type = NODE_INVALID;
    VARIANT_BOOL b;
    BSTR str;
    VARIANT var;
    LONG count;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    b = FALSE;
    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    EXPECT_CHILDREN(doc);

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    VariantInit(&var);
    ok( V_VT(&var) == VT_EMPTY, "variant init failed\n");

    r = IXMLDOMNode_get_nodeValue( doc, NULL );
    ok(r == E_INVALIDARG, "get_nodeValue ret %08x\n", r );

    r = IXMLDOMNode_get_nodeValue( doc, &var );
    ok( r == S_FALSE, "nextNode returned wrong code\n");
    ok( V_VT(&var) == VT_NULL, "variant wasn't empty\n");
    ok( V_BSTR(&var) == NULL, "variant value wasn't null\n");

    if (element)
    {
        owner = NULL;
        r = IXMLDOMNode_get_ownerDocument( element, &owner );
        ok( r == S_OK, "get_ownerDocument return code\n");
        ok( owner != doc, "get_ownerDocument return\n");
        IXMLDOMDocument_Release(owner);

        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( element, &type);
        ok( r == S_OK, "got %08x\n", r);
        ok( type == NODE_ELEMENT, "node not an element\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( element, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szlc) == 0, "basename was wrong\n");
        SysFreeString(str);

        /* check if nodename is correct */
        r = IXMLDOMElement_get_nodeName( element, NULL );
        ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

        /* content doesn't matter here */
        str = NULL;
        r = IXMLDOMElement_get_nodeName( element, &str );
        ok ( r == S_OK, "get_nodeName wrong code\n");
        ok ( str != NULL, "str is null\n");
        ok( !lstrcmpW( str, szlc ), "incorrect nodeName\n");
        SysFreeString( str );

        str = SysAllocString( nonexistent_fileW );
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0x1234;
        r = IXMLDOMElement_getAttribute( element, str, &var );
        ok( r == E_FAIL, "getAttribute ret %08x\n", r );
        ok( V_VT(&var) == VT_NULL || V_VT(&var) == VT_EMPTY, "vt = %x\n", V_VT(&var));
        VariantClear(&var);

        str = SysAllocString( szdl );	
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0x1234;
        r = IXMLDOMElement_getAttribute( element, str, &var );
        ok( r == S_OK, "getAttribute ret %08x\n", r );
        ok( V_VT(&var) == VT_BSTR, "vt = %x\n", V_VT(&var));
        ok( !lstrcmpW(V_BSTR(&var), szstr1), "wrong attr value\n");
        VariantClear( &var );

        r = IXMLDOMElement_getAttribute( element, NULL, &var );
        ok( r == E_INVALIDARG, "getAttribute ret %08x\n", r );

        r = IXMLDOMElement_getAttribute( element, str, NULL );
        ok( r == E_INVALIDARG, "getAttribute ret %08x\n", r );

        attr = NULL;
        r = IXMLDOMElement_getAttributeNode( element, str, &attr);
        ok( r == S_OK, "GetAttributeNode ret %08x\n", r );
        ok( attr != NULL, "getAttributeNode returned NULL\n" );
        if (attr)
        {
            r = IXMLDOMAttribute_get_parentNode( attr, NULL );
            ok( r == E_INVALIDARG, "Expected E_INVALIDARG, ret %08x\n", r );

            /* attribute doesn't have a parent in msxml interpretation */
            node = (IXMLDOMNode*)0xdeadbeef;
            r = IXMLDOMAttribute_get_parentNode( attr, &node );
            ok( r == S_FALSE, "Expected S_FALSE, ret %08x\n", r );
            ok( node == NULL, "Expected NULL, got %p\n", node );

            IXMLDOMAttribute_Release(attr);
        }

        SysFreeString( str );

        r = IXMLDOMElement_get_attributes( element, &map );
        ok( r == S_OK, "get_attributes returned wrong code\n");
        ok( map != NULL, "should be attributes\n");

        EXPECT_CHILDREN(element);
    }
    else
        ok( FALSE, "no element\n");

    if (map)
    {
        ISupportErrorInfo *support_error;
        r = IXMLDOMNamedNodeMap_QueryInterface( map, &IID_ISupportErrorInfo, (void**)&support_error );
        ok( r == S_OK, "ret %08x\n", r );

        r = ISupportErrorInfo_InterfaceSupportsErrorInfo( support_error, &IID_IXMLDOMNamedNodeMap );
todo_wine
{
        ok( r == S_OK, "ret %08x\n", r );
}
        ISupportErrorInfo_Release( support_error );

        str = SysAllocString( szdl );
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, &node );
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( node != NULL, "should be attributes\n");
        IXMLDOMNode_Release(node);
        SysFreeString( str );

        str = SysAllocString( szdl );
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, NULL );
        ok( r == E_INVALIDARG, "getNamedItem should return E_INVALIDARG\n");
        SysFreeString( str );

        /* something that isn't in complete4A */
        str = SysAllocString( szOpen );
        node = (IXMLDOMNode *) 1;
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, &node );
        ok( r == S_FALSE, "getNamedItem found a node that wasn't there\n");
        ok( node == NULL, "getNamedItem should have returned NULL\n");
        SysFreeString( str );

	/* test indexed access of attributes */
        r = IXMLDOMNamedNodeMap_get_length( map, NULL );
        ok ( r == E_INVALIDARG, "get_length should return E_INVALIDARG\n");

        r = IXMLDOMNamedNodeMap_get_length( map, &count );
        ok ( r == S_OK, "get_length wrong code\n");
        ok ( count == 1, "get_length != 1\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, -1, &node);
        ok ( r == S_FALSE, "get_item (-1) wrong code\n");
        ok ( node == NULL, "there is no node\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, 1, &node);
        ok ( r == S_FALSE, "get_item (1) wrong code\n");
        ok ( node == NULL, "there is no attribute\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, 0, &node);
        ok ( r == S_OK, "get_item (0) wrong code\n");
        ok ( node != NULL, "should be attribute\n");

        r = IXMLDOMNode_get_nodeName( node, NULL );
        ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

        /* content doesn't matter here */
        str = NULL;
        r = IXMLDOMNode_get_nodeName( node, &str );
        ok ( r == S_OK, "get_nodeName wrong code\n");
        ok ( str != NULL, "str is null\n");
        ok( !lstrcmpW( str, szdl ), "incorrect node name\n");
        SysFreeString( str );
        IXMLDOMNode_Release( node );

        /* test sequential access of attributes */
        node = NULL;
        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r == S_OK, "nextNode (first time) wrong code\n");
        ok ( node != NULL, "nextNode, should be attribute\n");
        IXMLDOMNode_Release( node );

        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r != S_OK, "nextNode (second time) wrong code\n");
        ok ( node == NULL, "nextNode, there is no attribute\n");

        r = IXMLDOMNamedNodeMap_reset( map );
        ok ( r == S_OK, "reset should return S_OK\n");

        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r == S_OK, "nextNode (third time) wrong code\n");
        ok ( node != NULL, "nextNode, should be attribute\n");
    }
    else
        ok( FALSE, "no map\n");

    if (node)
    {
        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( node, &type);
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( type == NODE_ATTRIBUTE, "node not an attribute\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, NULL );
        ok( r == E_INVALIDARG, "get_baseName returned wrong code\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szdl) == 0, "basename was wrong\n");
        SysFreeString( str );

        r = IXMLDOMNode_get_nodeValue( node, &var );
        ok( r == S_OK, "returns %08x\n", r );
        ok( V_VT(&var) == VT_BSTR, "vt %x\n", V_VT(&var));
        ok( !lstrcmpW(V_BSTR(&var), szstr1), "nodeValue incorrect\n");
        VariantClear(&var);

        r = IXMLDOMNode_get_childNodes( node, NULL );
        ok( r == E_INVALIDARG, "get_childNodes returned wrong code\n");

        r = IXMLDOMNode_get_childNodes( node, &list );
        ok( r == S_OK, "get_childNodes returned wrong code\n");

        if (list)
        {
            r = IXMLDOMNodeList_nextNode( list, &next );
            ok( r == S_OK, "nextNode returned wrong code\n");
        }
        else
            ok( FALSE, "no childlist\n");

        if (next)
        {
            EXPECT_NO_CHILDREN(next);

            type = NODE_INVALID;
            r = IXMLDOMNode_get_nodeType( next, &type);
            ok( r == S_OK, "getNamedItem returned wrong code\n");
            ok( type == NODE_TEXT, "node not text\n");

            str = (BSTR) 1;
            r = IXMLDOMNode_get_baseName( next, &str );
            ok( r == S_FALSE, "get_baseName returned wrong code\n");
            ok( str == NULL, "basename was wrong\n");
            SysFreeString(str);
        }
        else
            ok( FALSE, "no next\n");

        if (next)
            IXMLDOMNode_Release( next );
        next = NULL;
        if (list)
            IXMLDOMNodeList_Release( list );
        list = NULL;
        if (node)
            IXMLDOMNode_Release( node );
    }
    else
        ok( FALSE, "no node\n");
    node = NULL;

    if (map)
        IXMLDOMNamedNodeMap_Release( map );

    /* now traverse the tree from the root element */
    if (element)
    {
        r = IXMLDOMNode_get_childNodes( element, &list );
        ok( r == S_OK, "get_childNodes returned wrong code\n");

        /* using get_item for child list doesn't advance the position */
        ole_check(IXMLDOMNodeList_get_item(list, 1, &node));
        expect_node(node, "E2.E2.D1");
        IXMLDOMNode_Release(node);
        ole_check(IXMLDOMNodeList_nextNode(list, &node));
        expect_node(node, "E1.E2.D1");
        IXMLDOMNode_Release(node);
        ole_check(IXMLDOMNodeList_reset(list));

        IXMLDOMNodeList_AddRef(list);
        expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1");
        ole_check(IXMLDOMNodeList_reset(list));

        node = (void*)0xdeadbeef;
        str = SysAllocString(szdl);
        r = IXMLDOMNode_selectSingleNode( element, str, &node );
        SysFreeString(str);
        ok( r == S_FALSE, "ret %08x\n", r );
        ok( node == NULL, "node %p\n", node );

        str = SysAllocString(szbs);
        r = IXMLDOMNode_selectSingleNode( element, str, &node );
        SysFreeString(str);
        ok( r == S_OK, "ret %08x\n", r );
        r = IXMLDOMNode_Release( node );
        ok( r == 0, "ret %08x\n", r );
    }
    else
        ok( FALSE, "no element\n");

    if (list)
    {
        r = IXMLDOMNodeList_get_item(list, 0, NULL);
        ok(r == E_INVALIDARG, "Expected E_INVALIDARG got %08x\n", r);

        r = IXMLDOMNodeList_get_length(list, NULL);
        ok(r == E_INVALIDARG, "Expected E_INVALIDARG got %08x\n", r);

        r = IXMLDOMNodeList_get_length( list, &count );
        ok( r == S_OK, "get_length returns %08x\n", r );
        ok( count == 4, "get_length got %d\n", count );

        r = IXMLDOMNodeList_nextNode(list, NULL);
        ok(r == E_INVALIDARG, "Expected E_INVALIDARG got %08x\n", r);

        r = IXMLDOMNodeList_nextNode( list, &node );
        ok( r == S_OK, "nextNode returned wrong code\n");
    }
    else
        ok( FALSE, "no list\n");

    if (node)
    {
        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( node, &type);
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( type == NODE_ELEMENT, "node not text\n");

        VariantInit(&var);
        ok( V_VT(&var) == VT_EMPTY, "variant init failed\n");
        r = IXMLDOMNode_get_nodeValue( node, &var );
        ok( r == S_FALSE, "nextNode returned wrong code\n");
        ok( V_VT(&var) == VT_NULL, "variant wasn't empty\n");
        ok( V_BSTR(&var) == NULL, "variant value wasn't null\n");

        r = IXMLDOMNode_hasChildNodes( node, NULL );
        ok( r == E_INVALIDARG, "hasChildNodes bad return\n");

        EXPECT_CHILDREN(node);

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szbs) == 0, "basename was wrong\n");
        SysFreeString(str);
    }
    else
        ok( FALSE, "no node\n");

    if (node)
        IXMLDOMNode_Release( node );
    if (list)
        IXMLDOMNodeList_Release( list );
    if (element)
        IXMLDOMElement_Release( element );

    b = FALSE;
    str = SysAllocString( szComplete5 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    EXPECT_CHILDREN(doc);

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    if (element)
    {
        static const WCHAR szSSearch[] = {'S',':','s','e','a','r','c','h',0};
        BSTR tag = NULL;

        /* check if the tag is correct */
        r = IXMLDOMElement_get_tagName( element, &tag );
        ok( r == S_OK, "couldn't get tag name\n");
        ok( tag != NULL, "tag was null\n");
        ok( !lstrcmpW( tag, szSSearch ), "incorrect tag name\n");
        SysFreeString( tag );
    }

    if (element)
        IXMLDOMElement_Release( element );
    ok(IXMLDOMDocument_Release( doc ) == 0, "document is not destroyed\n");

    free_bstrs();
}

static void test_refs(void)
{
    HRESULT r;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element = NULL;
    IXMLDOMNode *node = NULL, *node2;
    IXMLDOMNodeList *node_list = NULL;
    LONG ref;
    IUnknown *unk, *unk2;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    EXPECT_REF(doc, 1);
    ref = IXMLDOMDocument_Release(doc);
    ok( ref == 0, "ref %d\n", ref);

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    EXPECT_REF(doc, 1);
    IXMLDOMDocument_AddRef( doc );
    EXPECT_REF(doc, 2);
    IXMLDOMDocument_AddRef( doc );
    EXPECT_REF(doc, 3);

    IXMLDOMDocument_Release( doc );
    IXMLDOMDocument_Release( doc );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    EXPECT_REF(doc, 1);
    todo_wine EXPECT_REF(element, 2);

    IXMLDOMElement_AddRef(element);
    todo_wine EXPECT_REF(element, 3);
    IXMLDOMElement_Release(element);

    r = IXMLDOMElement_get_childNodes( element, &node_list );
    ok( r == S_OK, "rets %08x\n", r);

    todo_wine EXPECT_REF(element, 2);
    EXPECT_REF(node_list, 1);

    IXMLDOMNodeList_get_item( node_list, 0, &node );
    ok( r == S_OK, "rets %08x\n", r);
    EXPECT_REF(node_list, 1);
    EXPECT_REF(node, 1);

    IXMLDOMNodeList_get_item( node_list, 0, &node2 );
    ok( r == S_OK, "rets %08x\n", r);
    EXPECT_REF(node_list, 1);
    EXPECT_REF(node2, 1);

    ref = IXMLDOMNode_Release( node );
    ok( ref == 0, "ref %d\n", ref );
    ref = IXMLDOMNode_Release( node2 );
    ok( ref == 0, "ref %d\n", ref );

    ref = IXMLDOMNodeList_Release( node_list );
    ok( ref == 0, "ref %d\n", ref );

    ok( node != node2, "node %p node2 %p\n", node, node2 );

    ref = IXMLDOMDocument_Release( doc );
    ok( ref == 0, "ref %d\n", ref );

    todo_wine EXPECT_REF(element, 2);

    /* IUnknown must be unique however we obtain it */
    r = IXMLDOMElement_QueryInterface( element, &IID_IUnknown, (void**)&unk );
    ok( r == S_OK, "rets %08x\n", r );
    EXPECT_REF(element, 2);
    r = IXMLDOMElement_QueryInterface( element, &IID_IXMLDOMNode, (void**)&node );
    ok( r == S_OK, "rets %08x\n", r );
    todo_wine EXPECT_REF(element, 2);
    r = IXMLDOMNode_QueryInterface( node, &IID_IUnknown, (void**)&unk2 );
    ok( r == S_OK, "rets %08x\n", r );
    todo_wine EXPECT_REF(element, 2);
    ok( unk == unk2, "unk %p unk2 %p\n", unk, unk2 );
    todo_wine ok( element != (void*)node, "node %p element %p\n", node, element );

    IUnknown_Release( unk2 );
    IUnknown_Release( unk );
    IXMLDOMNode_Release( node );
    todo_wine EXPECT_REF(element, 2);

    IXMLDOMElement_Release( element );

    free_bstrs();
}

static void test_create(void)
{
    static const WCHAR szOne[] = {'1',0};
    static const WCHAR szOneGarbage[] = {'1','G','a','r','b','a','g','e',0};
    HRESULT r;
    VARIANT var;
    BSTR str, name;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMComment *comment;
    IXMLDOMText *text;
    IXMLDOMCDATASection *cdata;
    IXMLDOMNode *root, *node, *child;
    IXMLDOMNamedNodeMap *attr_map;
    IUnknown *unk;
    LONG ref;
    LONG num;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    EXPECT_REF(doc, 1);

    /* types not supported for creation */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_DOCUMENT;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_DOCUMENT_TYPE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_NOTATION;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    /* NODE_COMMENT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_COMMENT;
    node = NULL;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node != NULL, "\n");

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMComment_get_data(comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMComment_get_data(comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMComment_get_data(comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    /* NODE_TEXT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_TEXT;
    node = NULL;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node != NULL, "\n");

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMText_get_data(text, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMText_get_data(text, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMText_get_data(text, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    /* NODE_CDATA_SECTION */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_CDATA_SECTION;
    node = NULL;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node != NULL, "\n");

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMCDATASection_get_data(cdata, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMCDATASection_get_data(cdata, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMCDATASection_get_data(cdata, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    /* NODE_ATTRIBUTE */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( szlc );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );
    SysFreeString(str);

    /* a name is required for attribute, try a BSTR with first null wchar */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( szstr1 );
    str[0] = 0;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);
    SysFreeString(str);

    /* NODE_PROCESSING_INSTRUCTION */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_("pi"), NULL, NULL );
    ok( r == E_INVALIDARG, "returns %08x\n", r );

    /* NODE_ENTITY_REFERENCE */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY_REFERENCE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY_REFERENCE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    /* NODE_ELEMENT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    str = SysAllocString( szlc );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, NULL );
    ok( r == E_INVALIDARG, "returns %08x\n", r );

    V_VT(&var) = VT_R4;
    V_R4(&var) = NODE_ELEMENT;
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szOne );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szOneGarbage );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );
    VariantClear(&var);

    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ELEMENT;
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    EXPECT_REF(doc, 1);
    r = IXMLDOMDocument_appendChild( doc, node, &root );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node == root, "%p %p\n", node, root );
    EXPECT_REF(doc, 1);

    EXPECT_REF(node, 2);

    ref = IXMLDOMNode_Release( node );
    ok(ref == 1, "ref %d\n", ref);
    SysFreeString( str );

    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ELEMENT;
    str = SysAllocString( szbs );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    SysFreeString( str );

    EXPECT_REF(node, 1);

    r = IXMLDOMNode_QueryInterface( node, &IID_IUnknown, (void**)&unk );
    ok( r == S_OK, "returns %08x\n", r );

    EXPECT_REF(unk, 2);

    V_VT(&var) = VT_EMPTY;
    child = NULL;
    r = IXMLDOMNode_insertBefore( root, (IXMLDOMNode*)unk, var, &child );
    ok( r == S_OK, "returns %08x\n", r );
    ok( unk == (IUnknown*)child, "%p %p\n", unk, child );

    todo_wine EXPECT_REF(unk, 4);

    IXMLDOMNode_Release( child );
    IUnknown_Release( unk );

    V_VT(&var) = VT_NULL;
    V_DISPATCH(&var) = (IDispatch*)node;
    r = IXMLDOMNode_insertBefore( root, node, var, &child );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node == child, "%p %p\n", node, child );
    IXMLDOMNode_Release( child );

    V_VT(&var) = VT_NULL;
    V_DISPATCH(&var) = (IDispatch*)node;
    r = IXMLDOMNode_insertBefore( root, node, var, NULL );
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release( node );

    r = IXMLDOMNode_QueryInterface( root, &IID_IXMLDOMElement, (void**)&element );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 0, "num %d\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr1 );
    name = SysAllocString( szdl );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 1, "num %d\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr2 );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 1, "num %d\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);
    r = IXMLDOMElement_getAttribute( element, name, &var );
    ok( r == S_OK, "returns %08x\n", r );
    ok( !lstrcmpW(V_BSTR(&var), szstr2), "wrong attr value\n");
    VariantClear(&var);
    SysFreeString(name);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr1 );
    name = SysAllocString( szlc );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 2, "num %d\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);
    SysFreeString(name);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 10;
    name = SysAllocString( szbs );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    VariantClear(&var);
    r = IXMLDOMElement_getAttribute( element, name, &var );
    ok( r == S_OK, "returns %08x\n", r );
    ok( V_VT(&var) == VT_BSTR, "variant type %x\n", V_VT(&var));
    VariantClear(&var);
    SysFreeString(name);

    /* Create an Attribute */
    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( szAttribute );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node != NULL, "node was null\n");
    SysFreeString(str);

    IXMLDOMElement_Release( element );
    IXMLDOMNode_Release( root );
    IXMLDOMDocument_Release( doc );
}

static void test_getElementsByTagName(void)
{
    IXMLDOMNodeList *node_list;
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    WCHAR buff[100];
    VARIANT_BOOL b;
    HRESULT r;
    LONG len;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    str = SysAllocString( szstar );

    /* null arguments cases */
    r = IXMLDOMDocument_getElementsByTagName(doc, NULL, &node_list);
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, NULL);
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 6, "len %d\n", len );

    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    /* broken query BSTR */
    memcpy(&buff[2], szstar, sizeof(szstar));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    r = IXMLDOMDocument_getElementsByTagName(doc, &buff[2], &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 6, "len %d\n", len );
    IXMLDOMNodeList_Release( node_list );

    str = SysAllocString( szbs );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 1, "len %d\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    str = SysAllocString( szdl );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 0, "len %d\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    str = SysAllocString( szstr1 );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 0, "len %d\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    /* test for element */
    r = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok( r == S_OK, "ret %08x\n", r );

    str = SysAllocString( szstar );

    /* null arguments cases */
    r = IXMLDOMElement_getElementsByTagName(elem, NULL, &node_list);
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    r = IXMLDOMElement_getElementsByTagName(elem, str, NULL);
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMElement_getElementsByTagName(elem, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 5, "len %d\n", len );
    expect_list_and_release(node_list, "E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1 E1.E4.E2.D1");
    SysFreeString( str );

    /* broken query BSTR */
    memcpy(&buff[2], szstar, sizeof(szstar));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    r = IXMLDOMElement_getElementsByTagName(elem, &buff[2], &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 5, "len %d\n", len );
    IXMLDOMNodeList_Release( node_list );

    IXMLDOMElement_Release(elem);

    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_get_text(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *node2, *node3;
    IXMLDOMNode *nodeRoot;
    IXMLDOMNodeList *node_list;
    IXMLDOMNamedNodeMap *node_map;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    str = SysAllocString( szbs );
    r = IXMLDOMDocument_getElementsByTagName( doc, str, &node_list );
    ok( r == S_OK, "ret %08x\n", r );
    SysFreeString(str);

    /* Test to get all child node text. */
    r = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMNode, (void**)&nodeRoot);
    ok( r == S_OK, "ret %08x\n", r );
    if(r == S_OK)
    {
        r = IXMLDOMNode_get_text( nodeRoot, &str );
        ok( r == S_OK, "ret %08x\n", r );
        ok( compareIgnoreReturns(str, _bstr_("fn1.txt\n\n fn2.txt \n\nf1\n")), "wrong get_text: %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMNode_Release(nodeRoot);
    }

    r = IXMLDOMNodeList_get_length( node_list, NULL );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 1, "expect 1 got %d\n", len );

    r = IXMLDOMNodeList_get_item( node_list, 0, NULL );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMNodeList_nextNode( node_list, NULL );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMNodeList_get_item( node_list, 0, &node );
    ok( r == S_OK, "ret %08x\n", r );
    IXMLDOMNodeList_Release( node_list );

    /* Invalid output parameter*/
    r = IXMLDOMNode_get_text( node, NULL );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMNode_get_text( node, &str );
    ok( r == S_OK, "ret %08x\n", r );
    ok( !memcmp(str, szfn1_txt, lstrlenW(szfn1_txt) ), "wrong string\n" );
    SysFreeString(str);

    r = IXMLDOMNode_get_attributes( node, &node_map );
    ok( r == S_OK, "ret %08x\n", r );

    str = SysAllocString( szvr );
    r = IXMLDOMNamedNodeMap_getNamedItem( node_map, str, &node2 );
    ok( r == S_OK, "ret %08x\n", r );
    SysFreeString(str);

    r = IXMLDOMNode_get_text( node2, &str );
    ok( r == S_OK, "ret %08x\n", r );
    ok( !memcmp(str, szstr2, sizeof(szstr2)), "wrong string\n" );
    SysFreeString(str);

    r = IXMLDOMNode_get_firstChild( node2, &node3 );
    ok( r == S_OK, "ret %08x\n", r );

    r = IXMLDOMNode_get_text( node3, &str );
    ok( r == S_OK, "ret %08x\n", r );
    ok( !memcmp(str, szstr2, sizeof(szstr2)), "wrong string\n" );
    SysFreeString(str);


    IXMLDOMNode_Release( node3 );
    IXMLDOMNode_Release( node2 );
    IXMLDOMNamedNodeMap_Release( node_map );
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_get_childNodes(void)
{
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *node, *node2;
    IXMLDOMNodeList *node_list, *node_list2;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    EXPECT_HR(hr, S_OK);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMElement_get_childNodes( element, &node_list );
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNodeList_get_length( node_list, &len );
    EXPECT_HR(hr, S_OK);
    ok( len == 4, "len %d\n", len);

    hr = IXMLDOMNodeList_get_item( node_list, 2, &node );
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNode_get_childNodes( node, &node_list2 );
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNodeList_get_length( node_list2, &len );
    EXPECT_HR(hr, S_OK);
    ok( len == 0, "len %d\n", len);

    hr = IXMLDOMNodeList_get_item( node_list2, 0, &node2);
    EXPECT_HR(hr, S_FALSE);

    IXMLDOMNodeList_Release( node_list2 );
    IXMLDOMNode_Release( node );
    IXMLDOMNodeList_Release( node_list );
    IXMLDOMElement_Release( element );

    /* test for children of <?xml ..?> node */
    hr = IXMLDOMDocument_get_firstChild(doc, &node);
    EXPECT_HR(hr, S_OK);

    str = NULL;
    hr = IXMLDOMNode_get_nodeName(node, &str);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(str, _bstr_("xml")), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* it returns empty but valid node list */
    node_list = (void*)0xdeadbeef;
    hr = IXMLDOMNode_get_childNodes(node, &node_list);
    EXPECT_HR(hr, S_OK);

    len = -1;
    hr = IXMLDOMNodeList_get_length(node_list, &len);
    EXPECT_HR(hr, S_OK);
    ok(len == 0, "got %d\n", len);

    IXMLDOMNodeList_Release( node_list );
    IXMLDOMNode_Release(node);

    IXMLDOMDocument_Release( doc );
    free_bstrs();
}

static void test_get_firstChild(void)
{
    static WCHAR xmlW[] = {'x','m','l',0};
    IXMLDOMDocument *doc;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    HRESULT r;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    r = IXMLDOMDocument_get_firstChild( doc, &node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_nodeName( node, &str );
    ok( r == S_OK, "ret %08x\n", r);

    ok(!lstrcmpW(str, xmlW), "expected \"xml\" node name, got %s\n", wine_dbgstr_w(str));

    SysFreeString(str);
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_get_lastChild(void)
{
    static WCHAR lcW[] = {'l','c',0};
    static WCHAR foW[] = {'f','o',0};
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *child;
    VARIANT_BOOL b;
    HRESULT r;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    r = IXMLDOMDocument_get_lastChild( doc, &node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_nodeName( node, &str );
    ok( r == S_OK, "ret %08x\n", r);

    ok(memcmp(str, lcW, sizeof(lcW)) == 0, "expected \"lc\" node name\n");
    SysFreeString(str);

    r = IXMLDOMNode_get_lastChild( node, &child );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_nodeName( child, &str );
    ok( r == S_OK, "ret %08x\n", r);

    ok(memcmp(str, foW, sizeof(foW)) == 0, "expected \"fo\" node name\n");
    SysFreeString(str);

    IXMLDOMNode_Release( child );
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_removeChild(void)
{
    HRESULT r;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element, *lc_element;
    IXMLDOMNode *fo_node, *ba_node, *removed_node, *temp_node, *lc_node;
    IXMLDOMNodeList *root_list, *fo_list;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);
    todo_wine EXPECT_REF(element, 2);

    r = IXMLDOMElement_get_childNodes( element, &root_list );
    ok( r == S_OK, "ret %08x\n", r);
    EXPECT_REF(root_list, 1);

    r = IXMLDOMNodeList_get_item( root_list, 3, &fo_node );
    ok( r == S_OK, "ret %08x\n", r);
    EXPECT_REF(fo_node, 1);

    r = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok( r == S_OK, "ret %08x\n", r);
    EXPECT_REF(fo_list, 1);

    r = IXMLDOMNodeList_get_item( fo_list, 0, &ba_node );
    ok( r == S_OK, "ret %08x\n", r);
    EXPECT_REF(ba_node, 1);

    /* invalid parameter: NULL ptr */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_removeChild( element, NULL, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* ba_node is a descendant of element, but not a direct child. */
    removed_node = (void*)0xdeadbeef;
    EXPECT_REF(ba_node, 1);
    EXPECT_CHILDREN(fo_node);
    r = IXMLDOMElement_removeChild( element, ba_node, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == NULL, "%p\n", removed_node );
    EXPECT_REF(ba_node, 1);
    EXPECT_CHILDREN(fo_node);

    EXPECT_REF(ba_node, 1);
    EXPECT_REF(fo_node, 1);
    r = IXMLDOMElement_removeChild( element, fo_node, &removed_node );
    ok( r == S_OK, "ret %08x\n", r);
    ok( fo_node == removed_node, "node %p node2 %p\n", fo_node, removed_node );
    EXPECT_REF(fo_node, 2);
    EXPECT_REF(ba_node, 1);

    /* try removing already removed child */
    temp_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_removeChild( element, fo_node, &temp_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r);
    ok( temp_node == NULL, "%p\n", temp_node );
    IXMLDOMNode_Release( fo_node );

    /* the removed node has no parent anymore */
    r = IXMLDOMNode_get_parentNode( removed_node, &temp_node );
    ok( r == S_FALSE, "ret %08x\n", r);
    ok( temp_node == NULL, "%p\n", temp_node );

    IXMLDOMNode_Release( removed_node );
    IXMLDOMNode_Release( ba_node );
    IXMLDOMNodeList_Release( fo_list );

    r = IXMLDOMNodeList_get_item( root_list, 0, &lc_node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_QueryInterface( lc_node, &IID_IXMLDOMElement, (void**)&lc_element );
    ok( r == S_OK, "ret %08x\n", r);

    /* MS quirk: passing wrong interface pointer works, too */
    r = IXMLDOMElement_removeChild( element, (IXMLDOMNode*)lc_element, NULL );
    ok( r == S_OK, "ret %08x\n", r);
    IXMLDOMElement_Release( lc_element );

    temp_node = (void*)0xdeadbeef;
    r = IXMLDOMNode_get_parentNode( lc_node, &temp_node );
    ok( r == S_FALSE, "ret %08x\n", r);
    ok( temp_node == NULL, "%p\n", temp_node );

    IXMLDOMNode_Release( lc_node );
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_replaceChild(void)
{
    HRESULT r;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element, *ba_element;
    IXMLDOMNode *fo_node, *ba_node, *lc_node, *removed_node, *temp_node;
    IXMLDOMNodeList *root_list, *fo_list;
    IUnknown * unk1, *unk2;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_get_childNodes( element, &root_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( root_list, 0, &lc_node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( root_list, 3, &fo_node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( fo_list, 0, &ba_node );
    ok( r == S_OK, "ret %08x\n", r);

    IXMLDOMNodeList_Release( fo_list );

    /* invalid parameter: NULL ptr for element to remove */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_replaceChild( element, ba_node, NULL, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* invalid parameter: NULL for replacement element. (Sic!) */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_replaceChild( element, NULL, fo_node, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* invalid parameter: OldNode is not a child */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_replaceChild( element, lc_node, ba_node, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == NULL, "%p\n", removed_node );
    IXMLDOMNode_Release( lc_node );

    /* invalid parameter: would create loop */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMNode_replaceChild( fo_node, fo_node, ba_node, &removed_node );
    ok( r == E_FAIL, "ret %08x\n", r );
    ok( removed_node == NULL, "%p\n", removed_node );

    r = IXMLDOMElement_replaceChild( element, ba_node, fo_node, NULL );
    ok( r == S_OK, "ret %08x\n", r );

    r = IXMLDOMNodeList_get_item( root_list, 3, &temp_node );
    ok( r == S_OK, "ret %08x\n", r );

    /* ba_node and temp_node refer to the same node, yet they
       are different interface pointers */
    ok( ba_node != temp_node, "ba_node %p temp_node %p\n", ba_node, temp_node);
    r = IXMLDOMNode_QueryInterface( temp_node, &IID_IUnknown, (void**)&unk1);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNode_QueryInterface( ba_node, &IID_IUnknown, (void**)&unk2);
    ok( r == S_OK, "ret %08x\n", r );
    todo_wine ok( unk1 == unk2, "unk1 %p unk2 %p\n", unk1, unk2);

    IUnknown_Release( unk1 );
    IUnknown_Release( unk2 );

    /* ba_node should have been removed from below fo_node */
    r = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok( r == S_OK, "ret %08x\n", r );

    /* MS quirk: replaceChild also accepts elements instead of nodes */
    r = IXMLDOMNode_QueryInterface( ba_node, &IID_IXMLDOMElement, (void**)&ba_element);
    ok( r == S_OK, "ret %08x\n", r );
    EXPECT_REF(ba_element, 2);

    removed_node = NULL;
    r = IXMLDOMElement_replaceChild( element, ba_node, (IXMLDOMNode*)ba_element, &removed_node );
    ok( r == S_OK, "ret %08x\n", r );
    ok( removed_node != NULL, "got %p\n", removed_node);
    EXPECT_REF(ba_element, 3);
    IXMLDOMElement_Release( ba_element );

    r = IXMLDOMNodeList_get_length( fo_list, &len);
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 0, "len %d\n", len);

    IXMLDOMNodeList_Release( fo_list );

    IXMLDOMNode_Release(ba_node);
    IXMLDOMNode_Release(fo_node);
    IXMLDOMNode_Release(temp_node);
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_removeNamedItem(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *pr_node, *removed_node, *removed_node2;
    IXMLDOMNodeList *root_list;
    IXMLDOMNamedNodeMap * pr_attrs;
    VARIANT_BOOL b;
    BSTR str;
    LONG len;
    HRESULT r;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    r = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_get_childNodes( element, &root_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( root_list, 1, &pr_node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_attributes( pr_node, &pr_attrs );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNamedNodeMap_get_length( pr_attrs, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 3, "length %d\n", len);

    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, NULL, &removed_node);
    ok ( r == E_INVALIDARG, "ret %08x\n", r);
    ok ( removed_node == (void*)0xdeadbeef, "got %p\n", removed_node);

    removed_node = (void*)0xdeadbeef;
    str = SysAllocString(szvr);
    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, str, &removed_node);
    ok ( r == S_OK, "ret %08x\n", r);

    removed_node2 = (void*)0xdeadbeef;
    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, str, &removed_node2);
    ok ( r == S_FALSE, "ret %08x\n", r);
    ok ( removed_node2 == NULL, "got %p\n", removed_node2 );

    r = IXMLDOMNamedNodeMap_get_length( pr_attrs, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 2, "length %d\n", len);

    r = IXMLDOMNamedNodeMap_setNamedItem( pr_attrs, removed_node, NULL);
    ok ( r == S_OK, "ret %08x\n", r);
    IXMLDOMNode_Release(removed_node);

    r = IXMLDOMNamedNodeMap_get_length( pr_attrs, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 3, "length %d\n", len);

    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, str, NULL);
    ok ( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNamedNodeMap_get_length( pr_attrs, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 2, "length %d\n", len);

    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, str, NULL);
    ok ( r == S_FALSE, "ret %08x\n", r);

    SysFreeString(str);

    IXMLDOMNamedNodeMap_Release( pr_attrs );
    IXMLDOMNode_Release( pr_node );
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

#define test_IObjectSafety_set(p, r, r2, s, m, e, e2) _test_IObjectSafety_set(__LINE__,p, r, r2, s, m, e, e2)
static void _test_IObjectSafety_set(unsigned line, IObjectSafety *safety, HRESULT result,
                                    HRESULT result2, DWORD set, DWORD mask, DWORD expected,
                                    DWORD expected2)
{
    DWORD enabled, supported;
    HRESULT hr;

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL, set, mask);
    if (result == result2)
        ok_(__FILE__,line)(hr == result, "SetInterfaceSafetyOptions: expected %08x, returned %08x\n", result, hr );
    else
        ok_(__FILE__,line)(broken(hr == result) || hr == result2,
           "SetInterfaceSafetyOptions: expected %08x, got %08x\n", result2, hr );

    supported = enabled = 0xCAFECAFE;
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok(hr == S_OK, "ret %08x\n", hr );
    if (expected == expected2)
        ok_(__FILE__,line)(enabled == expected, "Expected %08x, got %08x\n", expected, enabled);
    else
        ok_(__FILE__,line)(broken(enabled == expected) || enabled == expected2,
           "Expected %08x, got %08x\n", expected2, enabled);

    /* reset the safety options */

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
            INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_SECURITY_MANAGER,
            0);
    ok_(__FILE__,line)(hr == S_OK, "ret %08x\n", hr );

    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok_(__FILE__,line)(hr == S_OK, "ret %08x\n", hr );
    ok_(__FILE__,line)(enabled == 0, "Expected 0, got %08x\n", enabled);
}

#define test_IObjectSafety_common(s) _test_IObjectSafety_common(__LINE__,s)
static void _test_IObjectSafety_common(unsigned line, IObjectSafety *safety)
{
    DWORD enabled = 0, supported = 0;
    HRESULT hr;

    /* get */
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, NULL, &enabled);
    ok_(__FILE__,line)(hr == E_POINTER, "ret %08x\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, NULL);
    ok_(__FILE__,line)(hr == E_POINTER, "ret %08x\n", hr );

    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok_(__FILE__,line)(hr == S_OK, "ret %08x\n", hr );
    ok_(__FILE__,line)(broken(supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA)) ||
       supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER) /* msxml3 SP8+ */,
        "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER), "
             "got %08x\n", supported);
    ok_(__FILE__,line)(enabled == 0, "Expected 0, got %08x\n", enabled);

    /* set -- individual flags */

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_DATA, INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA, INTERFACESAFE_FOR_UNTRUSTED_DATA);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACE_USES_SECURITY_MANAGER, INTERFACE_USES_SECURITY_MANAGER,
        0, INTERFACE_USES_SECURITY_MANAGER /* msxml3 SP8+ */);

    /* set INTERFACE_USES_DISPEX  */

    test_IObjectSafety_set(safety, S_OK, E_FAIL /* msxml3 SP8+ */,
        INTERFACE_USES_DISPEX, INTERFACE_USES_DISPEX,
        0, 0);

    test_IObjectSafety_set(safety, S_OK, E_FAIL /* msxml3 SP8+ */,
        INTERFACE_USES_DISPEX, 0,
        0, 0);

    test_IObjectSafety_set(safety, S_OK, S_OK /* msxml3 SP8+ */,
        0, INTERFACE_USES_DISPEX,
        0, 0);

    /* set option masking */

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACE_USES_SECURITY_MANAGER,
        0,
        0);

    /* set -- inheriting previous settings */

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
                                                         INTERFACESAFE_FOR_UNTRUSTED_CALLER,
                                                         INTERFACESAFE_FOR_UNTRUSTED_CALLER);
    ok_(__FILE__,line)(hr == S_OK, "ret %08x\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok_(__FILE__,line)(hr == S_OK, "ret %08x\n", hr );
    ok_(__FILE__,line)(enabled == INTERFACESAFE_FOR_UNTRUSTED_CALLER, "Expected INTERFACESAFE_FOR_UNTRUSTED_CALLER got %08x\n", enabled);
    ok_(__FILE__,line)(broken(supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA)) ||
       supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER) /* msxml3 SP8+ */,
        "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER), "
             "got %08x\n", supported);

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
                                                         INTERFACESAFE_FOR_UNTRUSTED_DATA,
                                                         INTERFACESAFE_FOR_UNTRUSTED_DATA);
    ok_(__FILE__,line)(hr == S_OK, "ret %08x\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok_(__FILE__,line)(hr == S_OK, "ret %08x\n", hr );
    ok_(__FILE__,line)(broken(enabled == INTERFACESAFE_FOR_UNTRUSTED_DATA) ||
                       enabled == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA),
                       "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA) got %08x\n", enabled);
    ok_(__FILE__,line)(broken(supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA)) ||
       supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER) /* msxml3 SP8+ */,
        "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER), "
             "got %08x\n", supported);
}

static void test_XMLHTTP(void)
{
    static const WCHAR wszBody[] = {'m','o','d','e','=','T','e','s','t',0};
    static const WCHAR wszUrl[] = {'h','t','t','p',':','/','/',
        'c','r','o','s','s','o','v','e','r','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m','/',
        'p','o','s','t','t','e','s','t','.','p','h','p',0};
    static const WCHAR xmltestW[] = {'h','t','t','p',':','/','/',
        'c','r','o','s','s','o','v','e','r','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m','/',
        'x','m','l','t','e','s','t','.','x','m','l',0};
    static const WCHAR wszExpectedResponse[] = {'F','A','I','L','E','D',0};
    static const CHAR xmltestbodyA[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<a>TEST</a>\n";

    IXMLHttpRequest *xhr;
    IObjectSafety *safety;
    IObjectWithSite *obj_site, *obj_site2;
    BSTR bstrResponse, url;
    VARIANT dummy;
    VARIANT async;
    VARIANT varbody;
    LONG state, status, bound;
    IDispatch *event;
    void *ptr;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER,
        &IID_IXMLHttpRequest, (void**)&xhr);
    if (FAILED(hr))
    {
        win_skip("IXMLHTTPRequest is not available (0x%08x)\n", hr);
        return;
    }

    VariantInit(&dummy);
    V_VT(&dummy) = VT_ERROR;
    V_ERROR(&dummy) = DISP_E_MEMBERNOTFOUND;
    VariantInit(&async);
    V_VT(&async) = VT_BOOL;
    V_BOOL(&async) = VARIANT_FALSE;
    V_VT(&varbody) = VT_BSTR;
    V_BSTR(&varbody) = SysAllocString(wszBody);

    url = SysAllocString(wszUrl);

    hr = IXMLHttpRequest_put_onreadystatechange(xhr, NULL);
    EXPECT_HR(hr, S_OK);

    hr = IXMLHttpRequest_abort(xhr);
    EXPECT_HR(hr, S_OK);

    /* send before open */
    hr = IXMLHttpRequest_send(xhr, dummy);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "got 0x%08x\n", hr);

    /* initial status code */
    hr = IXMLHttpRequest_get_status(xhr, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    status = 0xdeadbeef;
    hr = IXMLHttpRequest_get_status(xhr, &status);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "got 0x%08x\n", hr);
    ok(status == 0xdeadbeef, "got %d\n", status);

    /* invalid parameters */
    hr = IXMLHttpRequest_open(xhr, NULL, NULL, async, dummy, dummy);
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLHttpRequest_open(xhr, _bstr_("POST"), NULL, async, dummy, dummy);
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLHttpRequest_open(xhr, NULL, url, async, dummy, dummy);
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLHttpRequest_setRequestHeader(xhr, NULL, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLHttpRequest_setRequestHeader(xhr, _bstr_("header1"), NULL);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "got 0x%08x\n", hr);

    hr = IXMLHttpRequest_setRequestHeader(xhr, NULL, _bstr_("value1"));
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLHttpRequest_setRequestHeader(xhr, _bstr_("header1"), _bstr_("value1"));
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "got 0x%08x\n", hr);

    hr = IXMLHttpRequest_get_readyState(xhr, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    state = -1;
    hr = IXMLHttpRequest_get_readyState(xhr, &state);
    EXPECT_HR(hr, S_OK);
    ok(state == READYSTATE_UNINITIALIZED, "got %d, expected READYSTATE_UNINITIALIZED\n", state);

    event = create_dispevent();

    EXPECT_REF(event, 1);
    hr = IXMLHttpRequest_put_onreadystatechange(xhr, event);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(event, 2);

    g_unexpectedcall = g_expectedcall = 0;

    hr = IXMLHttpRequest_open(xhr, _bstr_("POST"), url, async, dummy, dummy);
    EXPECT_HR(hr, S_OK);

    ok(g_unexpectedcall == 0, "unexpected disp event call\n");
    ok(g_expectedcall == 1 || broken(g_expectedcall == 0) /* win2k */, "no expected disp event call\n");

    /* status code after ::open() */
    status = 0xdeadbeef;
    hr = IXMLHttpRequest_get_status(xhr, &status);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "got 0x%08x\n", hr);
    ok(status == 0xdeadbeef, "got %d\n", status);

    state = -1;
    hr = IXMLHttpRequest_get_readyState(xhr, &state);
    EXPECT_HR(hr, S_OK);
    ok(state == READYSTATE_LOADING, "got %d, expected READYSTATE_LOADING\n", state);

    hr = IXMLHttpRequest_abort(xhr);
    EXPECT_HR(hr, S_OK);

    state = -1;
    hr = IXMLHttpRequest_get_readyState(xhr, &state);
    EXPECT_HR(hr, S_OK);
    ok(state == READYSTATE_UNINITIALIZED || broken(state == READYSTATE_LOADING) /* win2k */,
        "got %d, expected READYSTATE_UNINITIALIZED\n", state);

    hr = IXMLHttpRequest_open(xhr, _bstr_("POST"), url, async, dummy, dummy);
    EXPECT_HR(hr, S_OK);

    hr = IXMLHttpRequest_setRequestHeader(xhr, _bstr_("header1"), _bstr_("value1"));
    EXPECT_HR(hr, S_OK);

    hr = IXMLHttpRequest_setRequestHeader(xhr, NULL, _bstr_("value1"));
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLHttpRequest_setRequestHeader(xhr, _bstr_(""), _bstr_("value1"));
    EXPECT_HR(hr, E_INVALIDARG);

    SysFreeString(url);

    hr = IXMLHttpRequest_send(xhr, varbody);
    if (hr == INET_E_RESOURCE_NOT_FOUND)
    {
        skip("No connection could be made with crossover.codeweavers.com\n");
        IXMLHttpRequest_Release(xhr);
        return;
    }
    EXPECT_HR(hr, S_OK);

    /* status code after ::send() */
    status = 0xdeadbeef;
    hr = IXMLHttpRequest_get_status(xhr, &status);
    EXPECT_HR(hr, S_OK);
    ok(status == 200, "got %d\n", status);

    /* another ::send() after completed request */
    hr = IXMLHttpRequest_send(xhr, varbody);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "got 0x%08x\n", hr);

    VariantClear(&varbody);

    hr = IXMLHttpRequest_get_responseText(xhr, &bstrResponse);
    EXPECT_HR(hr, S_OK);
    /* the server currently returns "FAILED" because the Content-Type header is
     * not what the server expects */
    if(hr == S_OK)
    {
        ok(!memcmp(bstrResponse, wszExpectedResponse, sizeof(wszExpectedResponse)),
            "expected %s, got %s\n", wine_dbgstr_w(wszExpectedResponse), wine_dbgstr_w(bstrResponse));
        SysFreeString(bstrResponse);
    }

    /* GET request */
    url = SysAllocString(xmltestW);

    hr = IXMLHttpRequest_open(xhr, _bstr_("GET"), url, async, dummy, dummy);
    EXPECT_HR(hr, S_OK);

    V_VT(&varbody) = VT_EMPTY;

    hr = IXMLHttpRequest_send(xhr, varbody);
    if (hr == INET_E_RESOURCE_NOT_FOUND)
    {
        skip("No connection could be made with crossover.codeweavers.com\n");
        IXMLHttpRequest_Release(xhr);
        return;
    }
    EXPECT_HR(hr, S_OK);

    hr = IXMLHttpRequest_get_responseText(xhr, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLHttpRequest_get_responseText(xhr, &bstrResponse);
    EXPECT_HR(hr, S_OK);
    ok(!memcmp(bstrResponse, _bstr_(xmltestbodyA), sizeof(xmltestbodyA)*sizeof(WCHAR)),
        "expected %s, got %s\n", xmltestbodyA, wine_dbgstr_w(bstrResponse));
    SysFreeString(bstrResponse);

    hr = IXMLHttpRequest_get_responseBody(xhr, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    V_VT(&varbody) = VT_EMPTY;
    hr = IXMLHttpRequest_get_responseBody(xhr, &varbody);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&varbody) == (VT_ARRAY|VT_UI1), "got type %d, expected %d\n", V_VT(&varbody), VT_ARRAY|VT_UI1);
    ok(SafeArrayGetDim(V_ARRAY(&varbody)) == 1, "got %d, expected one dimension\n", SafeArrayGetDim(V_ARRAY(&varbody)));

    bound = -1;
    hr = SafeArrayGetLBound(V_ARRAY(&varbody), 1, &bound);
    EXPECT_HR(hr, S_OK);
    ok(bound == 0, "got %d, expected zero bound\n", bound);

    hr = SafeArrayAccessData(V_ARRAY(&varbody), &ptr);
    EXPECT_HR(hr, S_OK);
    ok(memcmp(ptr, xmltestbodyA, sizeof(xmltestbodyA)-1) == 0, "got wrong body data\n");
    SafeArrayUnaccessData(V_ARRAY(&varbody));

    VariantClear(&varbody);

    SysFreeString(url);

    hr = IXMLHttpRequest_QueryInterface(xhr, &IID_IObjectSafety, (void**)&safety);
    EXPECT_HR(hr, S_OK);
    if(hr == S_OK)
    {
        test_IObjectSafety_common(safety);
        IObjectSafety_Release(safety);
    }

    IDispatch_Release(event);

    /* interaction with object site */
    EXPECT_REF(xhr, 1);
    hr = IXMLHttpRequest_QueryInterface(xhr, &IID_IObjectWithSite, (void**)&obj_site);
    EXPECT_HR(hr, S_OK);
todo_wine {
    EXPECT_REF(xhr, 1);
    EXPECT_REF(obj_site, 1);
}

    IObjectWithSite_AddRef(obj_site);
todo_wine {
    EXPECT_REF(obj_site, 2);
    EXPECT_REF(xhr, 1);
}
    IObjectWithSite_Release(obj_site);

    hr = IXMLHttpRequest_QueryInterface(xhr, &IID_IObjectWithSite, (void**)&obj_site2);
    EXPECT_HR(hr, S_OK);
todo_wine {
    EXPECT_REF(xhr, 1);
    EXPECT_REF(obj_site, 1);
    EXPECT_REF(obj_site2, 1);
    ok(obj_site != obj_site2, "expected new instance\n");
}
    SET_EXPECT(site_qi_IServiceProvider);
    SET_EXPECT(sp_queryservice_SID_SBindHost);
    SET_EXPECT(sp_queryservice_SID_SContainerDispatch_htmldoc2);
    SET_EXPECT(sp_queryservice_SID_secmgr_htmldoc2);
    SET_EXPECT(sp_queryservice_SID_secmgr_xmldomdoc);
    SET_EXPECT(sp_queryservice_SID_secmgr_secmgr);

    /* calls to IHTMLDocument2 */
    SET_EXPECT(htmldoc2_get_all);
    SET_EXPECT(collection_get_length);
    SET_EXPECT(htmldoc2_get_url);

    SET_EXPECT(site_qi_IXMLDOMDocument);
    SET_EXPECT(site_qi_IOleClientSite);

    hr = IObjectWithSite_SetSite(obj_site, &testsite.IUnknown_iface);
    EXPECT_HR(hr, S_OK);

todo_wine{
    CHECK_CALLED(site_qi_IServiceProvider);

    CHECK_CALLED(sp_queryservice_SID_SBindHost);
    CHECK_CALLED(sp_queryservice_SID_SContainerDispatch_htmldoc2);
    CHECK_CALLED(sp_queryservice_SID_secmgr_htmldoc2);
    CHECK_CALLED(sp_queryservice_SID_secmgr_xmldomdoc);
    /* this one isn't very reliable
    CHECK_CALLED(sp_queryservice_SID_secmgr_secmgr); */

    CHECK_CALLED(htmldoc2_get_all);
    CHECK_CALLED(collection_get_length);
    CHECK_CALLED(htmldoc2_get_url);

    CHECK_CALLED(site_qi_IXMLDOMDocument);
    CHECK_CALLED(site_qi_IOleClientSite);
}
    IObjectWithSite_Release(obj_site);

    todo_wine EXPECT_REF(xhr, 1);
    IXMLHttpRequest_Release(xhr);

    /* still works after request is released */
    SET_EXPECT(site_qi_IServiceProvider);

    hr = IObjectWithSite_SetSite(obj_site2, &testsite.IUnknown_iface);
    EXPECT_HR(hr, S_OK);
    IObjectWithSite_Release(obj_site2);

    free_bstrs();
}

static void test_IXMLDOMDocument2(void)
{
    static const WCHAR emptyW[] = {0};
    IXMLDOMDocument2 *doc2, *dtddoc2;
    IXMLDOMDocument *doc;
    IXMLDOMParseError* err;
    IDispatchEx *dispex;
    VARIANT_BOOL b;
    VARIANT var;
    HRESULT r;
    LONG res;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    dtddoc2 = create_document(&IID_IXMLDOMDocument2);
    if (!dtddoc2)
    {
        IXMLDOMDocument_Release(doc);
        return;
    }

    r = IXMLDOMDocument_QueryInterface( doc, &IID_IXMLDOMDocument2, (void**)&doc2 );
    ok( r == S_OK, "ret %08x\n", r );
    ok( doc == (IXMLDOMDocument*)doc2, "interfaces differ\n");

    ole_expect(IXMLDOMDocument2_get_readyState(doc2, NULL), E_INVALIDARG);
    ole_check(IXMLDOMDocument2_get_readyState(doc2, &res));
    ok(res == READYSTATE_COMPLETE, "expected READYSTATE_COMPLETE (4), got %i\n", res);

    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(doc2, NULL), S_FALSE);
    ole_expect(IXMLDOMDocument2_validate(doc2, &err), S_FALSE);
    ok(err != NULL, "expected a pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_E_NOTWF */
        ok(res == E_XML_NOTWF, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    r = IXMLDOMDocument_loadXML( doc2, _bstr_(complete4A), &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    ole_check(IXMLDOMDocument2_get_readyState(doc, &res));
    ok(res == READYSTATE_COMPLETE, "expected READYSTATE_COMPLETE (4), got %i\n", res);

    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(doc2, &err), S_FALSE);
    ok(err != NULL, "expected a pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_E_NODTD */
        ok(res == E_XML_NODTD, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    r = IXMLDOMDocument_QueryInterface( doc, &IID_IDispatchEx, (void**)&dispex );
    ok( r == S_OK, "ret %08x\n", r );
    if(r == S_OK)
    {
        IDispatchEx_Release(dispex);
    }

    /* we will check if the variant got cleared */
    IXMLDOMDocument2_AddRef(doc2);
    EXPECT_REF(doc2, 3); /* doc, doc2, AddRef*/

    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown *)doc2;

    /* invalid calls */
    ole_expect(IXMLDOMDocument2_getProperty(doc2, _bstr_("askldhfaklsdf"), &var), E_FAIL);
    expect_eq(V_VT(&var), VT_UNKNOWN, int, "%x");
    ole_expect(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), NULL), E_INVALIDARG);

    /* valid call */
    ole_check(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), &var));
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    expect_bstr_eq_and_free(V_BSTR(&var), "XSLPattern");
    V_VT(&var) = VT_R4;

    /* the variant didn't get cleared*/
    expect_eq(IXMLDOMDocument2_Release(doc2), 2, int, "%d");

    /* setProperty tests */
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("askldhfaklsdf"), var), E_FAIL);
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), var), E_FAIL);
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("alskjdh faklsjd hfk")), E_FAIL);
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern")));
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern")));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(emptyW);
    r = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionNamespaces"), var);
    ok(r == S_OK, "got 0x%08x\n", r);
    VariantClear(&var);

    V_VT(&var) = VT_I2;
    V_I2(&var) = 0;
    r = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionNamespaces"), var);
    ok(r == E_FAIL, "got 0x%08x\n", r);

    /* contrary to what MSDN claims you can switch back from XPath to XSLPattern */
    ole_check(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), &var));
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    expect_bstr_eq_and_free(V_BSTR(&var), "XSLPattern");

    IXMLDOMDocument2_Release( doc2 );
    IXMLDOMDocument_Release( doc );

    /* DTD validation */
    ole_check(IXMLDOMDocument2_put_validateOnParse(dtddoc2, VARIANT_FALSE));
    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_check(IXMLDOMDocument2_validate(dtddoc2, &err));
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_expect(IXMLDOMParseError_get_errorCode(err, &res), S_FALSE);
        ok(res == 0, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_0D), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_ELEMENT_UNDECLARED */
        todo_wine ok(res == 0xC00CE00D, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_0E), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_ELEMENT_ID_NOT_FOUND */
        todo_wine ok(res == 0xC00CE00E, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_11), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_EMPTY_NOT_ALLOWED */
        todo_wine ok(res == 0xC00CE011, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_13), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_ROOT_NAME_MISMATCH */
        todo_wine ok(res == 0xC00CE013, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_14), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_INVALID_CONTENT */
        todo_wine ok(res == 0xC00CE014, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_15), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_ATTRIBUTE_NOT_DEFINED */
        todo_wine ok(res == 0xC00CE015, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_16), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_ATTRIBUTE_FIXED */
        todo_wine ok(res == 0xC00CE016, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_17), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_ATTRIBUTE_VALUE */
        todo_wine ok(res == 0xC00CE017, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_18), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_ILLEGAL_TEXT */
        todo_wine ok(res == 0xC00CE018, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    ole_check(IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_20), &b));
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(dtddoc2, &err), S_FALSE);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        ole_check(IXMLDOMParseError_get_errorCode(err, &res));
        /* XML_REQUIRED_ATTRIBUTE_MISSING */
        todo_wine ok(res == 0xC00CE020, "got %08x\n", res);
        IXMLDOMParseError_Release(err);
    }

    IXMLDOMDocument2_Release( dtddoc2 );
    free_bstrs();
}

#define helper_ole_check(expr) { \
    HRESULT r = expr; \
    ok_(__FILE__, line)(r == S_OK, "=> %i: " #expr " returned %08x\n", __LINE__, r); \
}

#define helper_expect_list_and_release(list, expstr) { \
    char *str = list_to_string(list); \
    ok_(__FILE__, line)(strcmp(str, expstr)==0, "=> %i: Invalid node list: %s, expected %s\n", __LINE__, str, expstr); \
    if (list) IXMLDOMNodeList_Release(list); \
}

#define helper_expect_bstr_and_release(bstr, str) { \
    ok_(__FILE__, line)(lstrcmpW(bstr, _bstr_(str)) == 0, \
       "=> %i: got %s\n", __LINE__, wine_dbgstr_w(bstr)); \
    SysFreeString(bstr); \
}

#define check_ws_ignored(doc, str) _check_ws_ignored(__LINE__, doc, str)
static inline void _check_ws_ignored(int line, IXMLDOMDocument2* doc, char const* str)
{
    IXMLDOMNode *node1, *node2;
    IXMLDOMNodeList *list;
    BSTR bstr;

    helper_ole_check(IXMLDOMDocument2_selectNodes(doc, _bstr_("//*[local-name()='html']"), &list));
    helper_ole_check(IXMLDOMNodeList_get_item(list, 0, &node1));
    helper_ole_check(IXMLDOMNodeList_get_item(list, 1, &node2));
    helper_ole_check(IXMLDOMNodeList_reset(list));
    helper_expect_list_and_release(list, "E1.E4.E1.E2.D1 E2.E4.E1.E2.D1");

    helper_ole_check(IXMLDOMNode_get_childNodes(node1, &list));
    helper_expect_list_and_release(list, "T1.E1.E4.E1.E2.D1 E2.E1.E4.E1.E2.D1 E3.E1.E4.E1.E2.D1 T4.E1.E4.E1.E2.D1 E5.E1.E4.E1.E2.D1");
    helper_ole_check(IXMLDOMNode_get_text(node1, &bstr));
    if (str)
    {
        helper_expect_bstr_and_release(bstr, str);
    }
    else
    {
        helper_expect_bstr_and_release(bstr, "This is a description.");
    }
    IXMLDOMNode_Release(node1);

    helper_ole_check(IXMLDOMNode_get_childNodes(node2, &list));
    helper_expect_list_and_release(list, "T1.E2.E4.E1.E2.D1 E2.E2.E4.E1.E2.D1 T3.E2.E4.E1.E2.D1 E4.E2.E4.E1.E2.D1 T5.E2.E4.E1.E2.D1 E6.E2.E4.E1.E2.D1 T7.E2.E4.E1.E2.D1");
    helper_ole_check(IXMLDOMNode_get_text(node2, &bstr));
    helper_expect_bstr_and_release(bstr, "\n                This is a description with preserved whitespace. \n            ");
    IXMLDOMNode_Release(node2);
}

#define check_ws_preserved(doc, str) _check_ws_preserved(__LINE__, doc, str)
static inline void _check_ws_preserved(int line, IXMLDOMDocument2* doc, char const* str)
{
    IXMLDOMNode *node1, *node2;
    IXMLDOMNodeList *list;
    BSTR bstr;

    helper_ole_check(IXMLDOMDocument2_selectNodes(doc, _bstr_("//*[local-name()='html']"), &list));
    helper_ole_check(IXMLDOMNodeList_get_item(list, 0, &node1));
    helper_ole_check(IXMLDOMNodeList_get_item(list, 1, &node2));
    helper_ole_check(IXMLDOMNodeList_reset(list));
    helper_expect_list_and_release(list, "E2.E8.E2.E2.D1 E4.E8.E2.E2.D1");

    helper_ole_check(IXMLDOMNode_get_childNodes(node1, &list));
    helper_expect_list_and_release(list, "T1.E2.E8.E2.E2.D1 E2.E2.E8.E2.E2.D1 T3.E2.E8.E2.E2.D1 E4.E2.E8.E2.E2.D1 T5.E2.E8.E2.E2.D1 E6.E2.E8.E2.E2.D1 T7.E2.E8.E2.E2.D1");
    helper_ole_check(IXMLDOMNode_get_text(node1, &bstr));
    if (str)
    {
        helper_expect_bstr_and_release(bstr, str);
    }
    else
    {
        helper_expect_bstr_and_release(bstr, "\n                This is a description. \n            ");
    }
    IXMLDOMNode_Release(node1);

    helper_ole_check(IXMLDOMNode_get_childNodes(node2, &list));
    helper_expect_list_and_release(list, "T1.E4.E8.E2.E2.D1 E2.E4.E8.E2.E2.D1 T3.E4.E8.E2.E2.D1 E4.E4.E8.E2.E2.D1 T5.E4.E8.E2.E2.D1 E6.E4.E8.E2.E2.D1 T7.E4.E8.E2.E2.D1");
    helper_ole_check(IXMLDOMNode_get_text(node2, &bstr));
    helper_expect_bstr_and_release(bstr, "\n                This is a description with preserved whitespace. \n            ");
    IXMLDOMNode_Release(node2);
}

static void test_whitespace(void)
{
    VARIANT_BOOL b;
    IXMLDOMDocument2 *doc1, *doc2, *doc3, *doc4;

    doc1 = create_document(&IID_IXMLDOMDocument2);
    doc2 = create_document(&IID_IXMLDOMDocument2);
    if (!doc1 || !doc2) return;

    ole_check(IXMLDOMDocument2_put_preserveWhiteSpace(doc2, VARIANT_TRUE));
    ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc1, &b));
    ok(b == VARIANT_FALSE, "expected false\n");
    ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc2, &b));
    ok(b == VARIANT_TRUE, "expected true\n");

    ole_check(IXMLDOMDocument2_loadXML(doc1, _bstr_(szExampleXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");
    ole_check(IXMLDOMDocument2_loadXML(doc2, _bstr_(szExampleXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* switch to XPath */
    ole_check(IXMLDOMDocument2_setProperty(doc1, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));

    check_ws_ignored(doc1, NULL);
    check_ws_preserved(doc2, NULL);

    /* new instances copy the property */
    ole_check(IXMLDOMDocument2_QueryInterface(doc1, &IID_IXMLDOMDocument2, (void**) &doc3));
    ole_check(IXMLDOMDocument2_QueryInterface(doc2, &IID_IXMLDOMDocument2, (void**) &doc4));

    ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc3, &b));
    ok(b == VARIANT_FALSE, "expected false\n");
    ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc4, &b));
    ok(b == VARIANT_TRUE, "expected true\n");

    check_ws_ignored(doc3, NULL);
    check_ws_preserved(doc4, NULL);

    /* setting after loading xml affects trimming of leading/trailing ws only */
    ole_check(IXMLDOMDocument2_put_preserveWhiteSpace(doc1, VARIANT_TRUE));
    ole_check(IXMLDOMDocument2_put_preserveWhiteSpace(doc2, VARIANT_FALSE));

    /* the trailing "\n            " isn't there, because it was ws-only node */
    check_ws_ignored(doc1, "\n                This is a description. ");
    check_ws_preserved(doc2, "This is a description.");

    /* it takes effect on reload */
    ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc1, &b));
    ok(b == VARIANT_TRUE, "expected true\n");
    ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc2, &b));
    ok(b == VARIANT_FALSE, "expected false\n");

    ole_check(IXMLDOMDocument2_loadXML(doc1, _bstr_(szExampleXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");
    ole_check(IXMLDOMDocument2_loadXML(doc2, _bstr_(szExampleXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    check_ws_preserved(doc1, NULL);
    check_ws_ignored(doc2, NULL);

    /* other instances follow suit */
    ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc3, &b));
    ok(b == VARIANT_TRUE, "expected true\n");
    ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc4, &b));
    ok(b == VARIANT_FALSE, "expected false\n");

    check_ws_preserved(doc3, NULL);
    check_ws_ignored(doc4, NULL);

    IXMLDOMDocument_Release(doc1);
    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc3);
    IXMLDOMDocument_Release(doc4);
    free_bstrs();
}

static void test_XPath(void)
{
    VARIANT var;
    VARIANT_BOOL b;
    IXMLDOMDocument2 *doc;
    IXMLDOMDocument *doc2;
    IXMLDOMNode *rootNode;
    IXMLDOMNode *elem1Node;
    IXMLDOMNode *node;
    IXMLDOMNodeList *list;
    IXMLDOMElement *elem;
    IXMLDOMAttribute *attr;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument2);
    if (!doc) return;

    ole_check(IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* switch to XPath */
    ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));

    /* some simple queries*/
    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root"), &list);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(doc, 1);
    EXPECT_LIST_LEN(list, 1);

    EXPECT_REF(list, 1);
    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(list, 1);
    EXPECT_REF(rootNode, 1);

    hr = IXMLDOMNodeList_reset(list);
    EXPECT_HR(hr, S_OK);
    expect_list_and_release(list, "E2.D1");

    ole_check(IXMLDOMDocument2_selectNodes(doc, _bstr_("root//c"), &list));
    expect_list_and_release(list, "E3.E1.E2.D1 E3.E2.E2.D1");

    ole_check(IXMLDOMDocument2_selectNodes(doc, _bstr_("//c[@type]"), &list));
    expect_list_and_release(list, "E3.E2.E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("elem"), &list));
    /* using get_item for query results advances the position */
    ole_check(IXMLDOMNodeList_get_item(list, 1, &node));
    expect_node(node, "E2.E2.D1");
    IXMLDOMNode_Release(node);
    ole_check(IXMLDOMNodeList_nextNode(list, &node));
    expect_node(node, "E4.E2.D1");
    IXMLDOMNode_Release(node);
    ole_check(IXMLDOMNodeList_reset(list));
    expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E4.E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("."), &list));
    expect_list_and_release(list, "E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("elem[3]/preceding-sibling::*"), &list));
    ole_check(IXMLDOMNodeList_get_item(list, 0, &elem1Node));
    ole_check(IXMLDOMNodeList_reset(list));
    expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E3.E2.D1");

    /* select an attribute */
    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//@type"), &list));
    expect_list_and_release(list, "A'type'.E3.E2.E2.D1");

    /* would evaluate to a number */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("count(*)"), &list), E_FAIL);
    /* would evaluate to a boolean */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("position()>0"), &list), E_FAIL);
    /* would evaluate to a string */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("name()"), &list), E_FAIL);

    /* no results */
    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("c"), &list));
    expect_list_and_release(list, "");
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("elem//c"), &list));
    expect_list_and_release(list, "");
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("//elem[4]"), &list));
    expect_list_and_release(list, "");
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("root//elem[0]"), &list));
    expect_list_and_release(list, "");

    /* foo undeclared in document node */
    ole_expect(IXMLDOMDocument2_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);
    /* undeclared in <root> node */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//foo:c"), &list), E_FAIL);
    /* undeclared in <elem> node */
    ole_expect(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//foo:c"), &list), E_FAIL);
    /* but this trick can be used */
    ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//*[name()='foo:c']"), &list));
    expect_list_and_release(list, "E3.E4.E2.D1");

    /* it has to be declared in SelectionNamespaces */
    ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'")));

    /* now the namespace can be used */
    ole_check(IXMLDOMDocument2_selectNodes(doc, _bstr_("root//test:c"), &list));
    expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//test:c"), &list));
    expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//test:c"), &list));
    expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_(".//test:x"), &list));
    expect_list_and_release(list, "E5.E1.E4.E1.E2.D1 E6.E2.E4.E1.E2.D1");

    /* SelectionNamespaces syntax error - the namespaces doesn't work anymore but the value is stored */
    ole_expect(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' xmlns:foo=###")), E_FAIL);

    ole_expect(IXMLDOMDocument2_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);

    VariantInit(&var);
    ole_check(IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var));
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    if (V_VT(&var) == VT_BSTR)
        expect_bstr_eq_and_free(V_BSTR(&var), "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' xmlns:foo=###");

    /* extra attributes - same thing*/
    ole_expect(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' param='test'")), E_FAIL);
    ole_expect(IXMLDOMDocument_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);

    IXMLDOMNode_Release(rootNode);
    IXMLDOMNode_Release(elem1Node);

    /* alter document with already built list */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root"), &list);
    EXPECT_HR(hr, S_OK);
    EXPECT_LIST_LEN(list, 1);

    hr = IXMLDOMDocument2_get_lastChild(doc, &rootNode);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(rootNode, 1);
    EXPECT_REF(doc, 1);

    hr = IXMLDOMDocument2_removeChild(doc, rootNode, NULL);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNode_Release(rootNode);

    EXPECT_LIST_LEN(list, 1);

    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(rootNode, 1);

    IXMLDOMNodeList_Release(list);

    hr = IXMLDOMNode_get_nodeName(rootNode, &str);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(str, _bstr_("root")), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(rootNode);

    /* alter node from list and get it another time */
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root"), &list);
    EXPECT_HR(hr, S_OK);
    EXPECT_LIST_LEN(list, 1);

    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNode_QueryInterface(rootNode, &IID_IXMLDOMElement, (void**)&elem);
    EXPECT_HR(hr, S_OK);

    V_VT(&var) = VT_I2;
    V_I2(&var) = 1;
    hr = IXMLDOMElement_setAttribute(elem, _bstr_("attrtest"), var);
    EXPECT_HR(hr, S_OK);
    IXMLDOMElement_Release(elem);
    IXMLDOMNode_Release(rootNode);

    /* now check attribute to be present */
    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNode_QueryInterface(rootNode, &IID_IXMLDOMElement, (void**)&elem);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("attrtest"), &attr);
    EXPECT_HR(hr, S_OK);
    IXMLDOMAttribute_Release(attr);

    IXMLDOMElement_Release(elem);
    IXMLDOMNode_Release(rootNode);

    /* and now check for attribute in original document */
    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("attrtest"), &attr);
    EXPECT_HR(hr, S_OK);
    IXMLDOMAttribute_Release(attr);

    IXMLDOMElement_Release(elem);

    /* attach node from list to another document */
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root"), &list);
    EXPECT_HR(hr, S_OK);
    EXPECT_LIST_LEN(list, 1);

    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(rootNode, 1);

    hr = IXMLDOMDocument_appendChild(doc2, rootNode, NULL);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(rootNode, 1);
    EXPECT_REF(doc2, 1);
    EXPECT_REF(list, 1);

    EXPECT_LIST_LEN(list, 1);

    IXMLDOMNode_Release(rootNode);
    IXMLDOMNodeList_Release(list);
    IXMLDOMDocument_Release(doc2);

    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

static void test_cloneNode(void )
{
    IXMLDOMDocument *doc, *doc2;
    VARIANT_BOOL b;
    IXMLDOMNodeList *pList;
    IXMLDOMNamedNodeMap *mapAttr;
    LONG length, length1;
    LONG attr_cnt, attr_cnt1;
    IXMLDOMNode *node;
    IXMLDOMNode *node_clone;
    IXMLDOMNode *node_first;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    ole_check(IXMLDOMDocument_loadXML(doc, _bstr_(complete4A), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMNode_selectSingleNode(doc, _bstr_("lc/pr"), &node);
    ok( hr == S_OK, "ret %08x\n", hr );
    ok( node != NULL, "node %p\n", node );

    /* Check invalid parameter */
    hr = IXMLDOMNode_cloneNode(node, VARIANT_TRUE, NULL);
    ok( hr == E_INVALIDARG, "ret %08x\n", hr );

    /* All Children */
    hr = IXMLDOMNode_cloneNode(node, VARIANT_TRUE, &node_clone);
    ok( hr == S_OK, "ret %08x\n", hr );
    ok( node_clone != NULL, "node %p\n", node );

    hr = IXMLDOMNode_get_firstChild(node_clone, &node_first);
    ok( hr == S_OK, "ret %08x\n", hr );
    hr = IXMLDOMNode_get_ownerDocument(node_clone, &doc2);
    ok( hr == S_OK, "ret %08x\n", hr );
    IXMLDOMDocument_Release(doc2);
    IXMLDOMNode_Release(node_first);

    hr = IXMLDOMNode_get_childNodes(node, &pList);
    ok( hr == S_OK, "ret %08x\n", hr );
    length = 0;
    hr = IXMLDOMNodeList_get_length(pList, &length);
    ok( hr == S_OK, "ret %08x\n", hr );
    ok(length == 1, "got %d\n", length);
    IXMLDOMNodeList_Release(pList);

    hr = IXMLDOMNode_get_attributes(node, &mapAttr);
    ok( hr == S_OK, "ret %08x\n", hr );
    attr_cnt = 0;
    hr = IXMLDOMNamedNodeMap_get_length(mapAttr, &attr_cnt);
    ok( hr == S_OK, "ret %08x\n", hr );
    ok(attr_cnt == 3, "got %d\n", attr_cnt);
    IXMLDOMNamedNodeMap_Release(mapAttr);

    hr = IXMLDOMNode_get_childNodes(node_clone, &pList);
    ok( hr == S_OK, "ret %08x\n", hr );
    length1 = 0;
    hr = IXMLDOMNodeList_get_length(pList, &length1);
    ok(length1 == 1, "got %d\n", length1);
    ok( hr == S_OK, "ret %08x\n", hr );
    IXMLDOMNodeList_Release(pList);

    hr = IXMLDOMNode_get_attributes(node_clone, &mapAttr);
    ok( hr == S_OK, "ret %08x\n", hr );
    attr_cnt1 = 0;
    hr = IXMLDOMNamedNodeMap_get_length(mapAttr, &attr_cnt1);
    ok( hr == S_OK, "ret %08x\n", hr );
    ok(attr_cnt1 == 3, "got %d\n", attr_cnt1);
    IXMLDOMNamedNodeMap_Release(mapAttr);

    ok(length == length1, "wrong Child count (%d, %d)\n", length, length1);
    ok(attr_cnt == attr_cnt1, "wrong Attribute count (%d, %d)\n", attr_cnt, attr_cnt1);
    IXMLDOMNode_Release(node_clone);

    /* No Children */
    hr = IXMLDOMNode_cloneNode(node, VARIANT_FALSE, &node_clone);
    ok( hr == S_OK, "ret %08x\n", hr );
    ok( node_clone != NULL, "node %p\n", node );

    hr = IXMLDOMNode_get_firstChild(node_clone, &node_first);
    ok(hr == S_FALSE, "ret %08x\n", hr );

    hr = IXMLDOMNode_get_childNodes(node_clone, &pList);
    ok(hr == S_OK, "ret %08x\n", hr );
    hr = IXMLDOMNodeList_get_length(pList, &length1);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok( length1 == 0, "Length should be 0 (%d)\n", length1);
    IXMLDOMNodeList_Release(pList);

    hr = IXMLDOMNode_get_attributes(node_clone, &mapAttr);
    ok(hr == S_OK, "ret %08x\n", hr );
    hr = IXMLDOMNamedNodeMap_get_length(mapAttr, &attr_cnt1);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(attr_cnt1 == 3, "Attribute count should be 3 (%d)\n", attr_cnt1);
    IXMLDOMNamedNodeMap_Release(mapAttr);

    ok(length != length1, "wrong Child count (%d, %d)\n", length, length1);
    ok(attr_cnt == attr_cnt1, "wrong Attribute count (%d, %d)\n", attr_cnt, attr_cnt1);
    IXMLDOMNode_Release(node_clone);

    IXMLDOMNode_Release(node);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_xmlTypes(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *pRoot;
    HRESULT hr;
    IXMLDOMComment *pComment;
    IXMLDOMElement *pElement;
    IXMLDOMAttribute *pAttribute;
    IXMLDOMNamedNodeMap *pAttribs;
    IXMLDOMCDATASection *pCDataSec;
    IXMLDOMImplementation *pIXMLDOMImplementation = NULL;
    IXMLDOMDocumentFragment *pDocFrag = NULL;
    IXMLDOMEntityReference *pEntityRef = NULL;
    BSTR str;
    IXMLDOMNode *pNextChild;
    VARIANT v;
    LONG len = 0;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    pNextChild = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_nextSibling(doc, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    pNextChild = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_nextSibling(doc, &pNextChild);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok(pNextChild == NULL, "pDocChild not NULL\n");

    /* test previous Sibling */
    hr = IXMLDOMDocument_get_previousSibling(doc, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    pNextChild = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_previousSibling(doc, &pNextChild);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok(pNextChild == NULL, "pNextChild not NULL\n");

    /* test get_dataType */
    V_VT(&v) = VT_EMPTY;
    hr = IXMLDOMDocument_get_dataType(doc, &v);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
    VariantClear(&v);

    /* test implementation */
    hr = IXMLDOMDocument_get_implementation(doc, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    hr = IXMLDOMDocument_get_implementation(doc, &pIXMLDOMImplementation);
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        VARIANT_BOOL hasFeature = VARIANT_TRUE;
        BSTR sEmpty = SysAllocStringLen(NULL, 0);

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, NULL, sEmpty, &hasFeature);
        ok(hr == E_INVALIDARG, "ret %08x\n", hr );

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, sEmpty, sEmpty, NULL);
        ok(hr == E_INVALIDARG, "ret %08x\n", hr );

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), sEmpty, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, sEmpty, sEmpty, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), NULL, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), sEmpty, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("XML"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("MS-DOM"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("SSS"), NULL, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        SysFreeString(sEmpty);
        IXMLDOMImplementation_Release(pIXMLDOMImplementation);
    }

    pRoot = (IXMLDOMElement*)0x1;
    hr = IXMLDOMDocument_createElement(doc, NULL, &pRoot);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );
    ok(pRoot == (void*)0x1, "Expect same ptr, got %p\n", pRoot);

    pRoot = (IXMLDOMElement*)0x1;
    hr = IXMLDOMDocument_createElement(doc, _bstr_(""), &pRoot);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    ok(pRoot == (void*)0x1, "Expect same ptr, got %p\n", pRoot);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &pRoot);
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)pRoot, NULL);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            /* Comment */
            str = SysAllocString(szComment);
            hr = IXMLDOMDocument_createComment(doc, str, &pComment);
            SysFreeString(str);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pComment, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_nodeName(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szCommentNodeText ), "incorrect comment node Name\n");
                SysFreeString(str);

                hr = IXMLDOMComment_get_xml(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szCommentXML ), "incorrect comment xml\n");
                SysFreeString(str);

                /* put data Tests */
                hr = IXMLDOMComment_put_data(pComment, _bstr_("This &is a ; test <>\\"));
                ok(hr == S_OK, "ret %08x\n", hr );

                /* get data Tests */
                hr = IXMLDOMComment_get_data(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect get_data string\n");
                SysFreeString(str);

                /* get data Tests */
                hr = IXMLDOMComment_get_nodeValue(pComment, &v);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( V_VT(&v) == VT_BSTR, "incorrect dataType type\n");
                ok( !lstrcmpW( V_BSTR(&v), _bstr_("This &is a ; test <>\\") ), "incorrect get_nodeValue string\n");
                VariantClear(&v);

                /* Confirm XML text is good */
                hr = IXMLDOMComment_get_xml(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("<!--This &is a ; test <>\\-->") ), "incorrect xml string\n");
                SysFreeString(str);

                /* Confirm we get the put_data Text back */
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect xml string\n");
                SysFreeString(str);

                /* test length property */
                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 21, "expected 21 got %d\n", len);

                /* test substringData */
                hr = IXMLDOMComment_substringData(pComment, 0, 4, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                /* test substringData - Invalid offset */
                str = (BSTR)&szElement;
                hr = IXMLDOMComment_substringData(pComment, -1, 4, &str);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid offset */
                str = (BSTR)&szElement;
                hr = IXMLDOMComment_substringData(pComment, 30, 0, &str);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (BSTR)&szElement;
                hr = IXMLDOMComment_substringData(pComment, 0, -1, &str);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (BSTR)&szElement;
                hr = IXMLDOMComment_substringData(pComment, 2, 0, &str);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Start of string */
                hr = IXMLDOMComment_substringData(pComment, 0, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - Middle of string */
                hr = IXMLDOMComment_substringData(pComment, 13, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("test") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - End of string */
                hr = IXMLDOMComment_substringData(pComment, 20, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("\\") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test appendData */
                hr = IXMLDOMComment_appendData(pComment, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_appendData(pComment, _bstr_(""));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_appendData(pComment, _bstr_("Append"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\Append") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* test insertData */
                str = SysAllocStringLen(NULL, 0);
                hr = IXMLDOMComment_insertData(pComment, -1, str);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, -1, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, str);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, str);
                ok(hr == S_OK, "ret %08x\n", hr );
                SysFreeString(str);

                hr = IXMLDOMComment_insertData(pComment, -1, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, _bstr_("Begin "));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 17, _bstr_("Middle"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 39, _bstr_(" End"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("Begin This &is a Middle; test <>\\Append End") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* delete data */
                /* invalid arguments */
                hr = IXMLDOMComment_deleteData(pComment, -1, 1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, 0);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, -1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 43, "expected 43 got %d\n", len);

                hr = IXMLDOMComment_deleteData(pComment, len, 1);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, len+1, 1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                /* delete from start */
                hr = IXMLDOMComment_deleteData(pComment, 0, 5);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 38, "expected 38 got %d\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_(" This &is a Middle; test <>\\Append End") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* delete from end */
                hr = IXMLDOMComment_deleteData(pComment, 35, 3);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 35, "expected 35 got %d\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_(" This &is a Middle; test <>\\Append ") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* delete from inside */
                hr = IXMLDOMComment_deleteData(pComment, 1, 33);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 2, "expected 2 got %d\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("  ") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* delete whole data ... */
                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, len);
                ok(hr == S_OK, "ret %08x\n", hr );
                /* ... and try again with empty string */
                hr = IXMLDOMComment_deleteData(pComment, 0, len);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* ::replaceData() */
                V_VT(&v) = VT_BSTR;
                V_BSTR(&v) = SysAllocString(szstr1);
                hr = IXMLDOMComment_put_nodeValue(pComment, v);
                ok(hr == S_OK, "ret %08x\n", hr );
                VariantClear(&v);

                hr = IXMLDOMComment_replaceData(pComment, 6, 0, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                hr = IXMLDOMComment_replaceData(pComment, 0, 0, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* NULL pointer means delete */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("tr1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* empty string means delete */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, _bstr_(""));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("r1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* zero count means insert */
                hr = IXMLDOMComment_replaceData(pComment, 0, 0, _bstr_("a"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("ar1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                hr = IXMLDOMComment_replaceData(pComment, 0, 2, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, _bstr_("m"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("m1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* nonempty string, count greater than its length */
                hr = IXMLDOMComment_replaceData(pComment, 0, 2, _bstr_("a1.2"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("a1.2") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* nonempty string, count less than its length */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, _bstr_("wine"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("wine1.2") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                IXMLDOMComment_Release(pComment);
            }

            /* Element */
            str = SysAllocString(szElement);
            hr = IXMLDOMDocument_createElement(doc, str, &pElement);
            SysFreeString(str);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMElement_get_nodeName(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szElement ), "incorrect element node Name\n");
                SysFreeString(str);

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szElementXML ), "incorrect element xml\n");
                SysFreeString(str);

                /* Attribute */
                pAttribute = (IXMLDOMAttribute*)0x1;
                hr = IXMLDOMDocument_createAttribute(doc, NULL, &pAttribute);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok(pAttribute == (void*)0x1, "Expect same ptr, got %p\n", pAttribute);

                pAttribute = (IXMLDOMAttribute*)0x1;
                hr = IXMLDOMDocument_createAttribute(doc, _bstr_(""), &pAttribute);
                ok(hr == E_FAIL, "ret %08x\n", hr );
                ok(pAttribute == (void*)0x1, "Expect same ptr, got %p\n", pAttribute);

                str = SysAllocString(szAttribute);
                hr = IXMLDOMDocument_createAttribute(doc, str, &pAttribute);
                SysFreeString(str);
                ok(hr == S_OK, "ret %08x\n", hr );
                if(hr == S_OK)
                {
                    IXMLDOMNode *pNewChild = (IXMLDOMNode *)0x1;

                    hr = IXMLDOMAttribute_get_nextSibling(pAttribute, NULL);
                    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                    pNextChild = (IXMLDOMNode *)0x1;
                    hr = IXMLDOMAttribute_get_nextSibling(pAttribute, &pNextChild);
                    ok(hr == S_FALSE, "ret %08x\n", hr );
                    ok(pNextChild == NULL, "pNextChild not NULL\n");

                    /* test Previous Sibling*/
                    hr = IXMLDOMAttribute_get_previousSibling(pAttribute, NULL);
                    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                    pNextChild = (IXMLDOMNode *)0x1;
                    hr = IXMLDOMAttribute_get_previousSibling(pAttribute, &pNextChild);
                    ok(hr == S_FALSE, "ret %08x\n", hr );
                    ok(pNextChild == NULL, "pNextChild not NULL\n");

                    hr = IXMLDOMElement_appendChild(pElement, (IXMLDOMNode*)pAttribute, &pNewChild);
                    ok(hr == E_FAIL, "ret %08x\n", hr );
                    ok(pNewChild == NULL, "pNewChild not NULL\n");

                    hr = IXMLDOMElement_get_attributes(pElement, &pAttribs);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    if ( hr == S_OK )
                    {
                        hr = IXMLDOMNamedNodeMap_setNamedItem(pAttribs, (IXMLDOMNode*)pAttribute, NULL );
                        ok(hr == S_OK, "ret %08x\n", hr );

                        IXMLDOMNamedNodeMap_Release(pAttribs);
                    }

                    hr = IXMLDOMAttribute_get_nodeName(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szAttribute ), "incorrect attribute node Name\n");
                    SysFreeString(str);

                    /* test nodeName */
                    hr = IXMLDOMAttribute_get_nodeName(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szAttribute ), "incorrect nodeName string\n");
                    SysFreeString(str);

                    /* test name property */
                    hr = IXMLDOMAttribute_get_name(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szAttribute ), "incorrect name string\n");
                    SysFreeString(str);

                    hr = IXMLDOMAttribute_get_xml(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szAttributeXML ), "incorrect attribute xml\n");
                    SysFreeString(str);

                    IXMLDOMAttribute_Release(pAttribute);

                    /* Check Element again with the Add Attribute*/
                    hr = IXMLDOMElement_get_xml(pElement, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szElementXML2 ), "incorrect element xml\n");
                    SysFreeString(str);
                }

                hr = IXMLDOMElement_put_text(pElement, _bstr_("TestingNode"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szElementXML3 ), "incorrect element xml\n");
                SysFreeString(str);

                /* Test for reversible escaping */
                str = SysAllocString( szStrangeChars );
                hr = IXMLDOMElement_put_text(pElement, str);
                ok(hr == S_OK, "ret %08x\n", hr );
                SysFreeString( str );

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szElementXML4 ), "incorrect element xml\n");
                SysFreeString(str);

                hr = IXMLDOMElement_get_text(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szStrangeChars ), "incorrect element text\n");
                SysFreeString(str);

                IXMLDOMElement_Release(pElement);
            }

            /* CData Section */
            str = SysAllocString(szCData);
            hr = IXMLDOMDocument_createCDATASection(doc, str, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            hr = IXMLDOMDocument_createCDATASection(doc, str, &pCDataSec);
            SysFreeString(str);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMNode *pNextChild = (IXMLDOMNode *)0x1;
                VARIANT var;

                VariantInit(&var);

                hr = IXMLDOMCDATASection_QueryInterface(pCDataSec, &IID_IXMLDOMElement, (void**)&pElement);
                ok(hr == E_NOINTERFACE, "ret %08x\n", hr);

                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pCDataSec, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_nodeName(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szCDataNodeText ), "incorrect cdata node Name\n");
                SysFreeString(str);

                hr = IXMLDOMCDATASection_get_xml(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szCDataXML ), "incorrect cdata xml\n");
                SysFreeString(str);

                /* test lastChild */
                pNextChild = (IXMLDOMNode*)0x1;
                hr = IXMLDOMCDATASection_get_lastChild(pCDataSec, &pNextChild);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(pNextChild == NULL, "pNextChild not NULL\n");

                /* put data Tests */
                hr = IXMLDOMCDATASection_put_data(pCDataSec, _bstr_("This &is a ; test <>\\"));
                ok(hr == S_OK, "ret %08x\n", hr );

                /* Confirm XML text is good */
                hr = IXMLDOMCDATASection_get_xml(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("<![CDATA[This &is a ; test <>\\]]>") ), "incorrect xml string\n");
                SysFreeString(str);

                /* Confirm we get the put_data Text back */
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect text string\n");
                SysFreeString(str);

                /* test length property */
                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 21, "expected 21 got %d\n", len);

                /* test get nodeValue */
                hr = IXMLDOMCDATASection_get_nodeValue(pCDataSec, &var);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(V_VT(&var) == VT_BSTR, "got vt %04x\n", V_VT(&var));
                ok( !lstrcmpW( V_BSTR(&var), _bstr_("This &is a ; test <>\\") ), "incorrect text string\n");
                VariantClear(&var);

                /* test get data */
                hr = IXMLDOMCDATASection_get_data(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect text string\n");
                SysFreeString(str);

                /* test substringData */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, 4, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                /* test substringData - Invalid offset */
                str = (BSTR)&szElement;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, -1, 4, &str);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid offset */
                str = (BSTR)&szElement;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 30, 0, &str);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (BSTR)&szElement;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, -1, &str);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (BSTR)&szElement;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 2, 0, &str);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Start of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - Middle of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 13, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("test") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - End of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 20, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("\\") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test appendData */
                hr = IXMLDOMCDATASection_appendData(pCDataSec, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_appendData(pCDataSec, _bstr_(""));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_appendData(pCDataSec, _bstr_("Append"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\Append") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* test insertData */
                str = SysAllocStringLen(NULL, 0);
                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, str);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, str);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, str);
                ok(hr == S_OK, "ret %08x\n", hr );
                SysFreeString(str);

                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, _bstr_("Begin "));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 17, _bstr_("Middle"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 39, _bstr_(" End"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("Begin This &is a Middle; test <>\\Append End") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* delete data */
                /* invalid arguments */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, -1, 1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, 0);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, -1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 43, "expected 43 got %d\n", len);

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, len, 1);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, len+1, 1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                /* delete from start */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, 5);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 38, "expected 38 got %d\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_(" This &is a Middle; test <>\\Append End") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* delete from end */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 35, 3);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 35, "expected 35 got %d\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_(" This &is a Middle; test <>\\Append ") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* delete from inside */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 1, 33);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 2, "expected 2 got %d\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("  ") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* delete whole data ... */
                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, len);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* ... and try again with empty string */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, len);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* ::replaceData() */
                V_VT(&v) = VT_BSTR;
                V_BSTR(&v) = SysAllocString(szstr1);
                hr = IXMLDOMCDATASection_put_nodeValue(pCDataSec, v);
                ok(hr == S_OK, "ret %08x\n", hr );
                VariantClear(&v);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 6, 0, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 0, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* NULL pointer means delete */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("tr1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* empty string means delete */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, _bstr_(""));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("r1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* zero count means insert */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 0, _bstr_("a"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("ar1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 2, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, _bstr_("m"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("m1") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* nonempty string, count greater than its length */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 2, _bstr_("a1.2"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("a1.2") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                /* nonempty string, count less than its length */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, _bstr_("wine"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("wine1.2") ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
                SysFreeString(str);

                IXMLDOMCDATASection_Release(pCDataSec);
            }

            /* Document Fragments */
            hr = IXMLDOMDocument_createDocumentFragment(doc, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            hr = IXMLDOMDocument_createDocumentFragment(doc, &pDocFrag);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMNode *node;

                hr = IXMLDOMDocumentFragment_get_parentNode(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_parentNode(pDocFrag, &node);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(node == NULL, "expected NULL, got %p\n", node);

                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pDocFrag, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMDocumentFragment_get_nodeName(pDocFrag, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szDocFragmentText ), "incorrect docfragment node Name\n");
                SysFreeString(str);

                /* test next Sibling*/
                hr = IXMLDOMDocumentFragment_get_nextSibling(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_nextSibling(pDocFrag, &node);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(node == NULL, "next sibling not NULL\n");

                /* test Previous Sibling*/
                hr = IXMLDOMDocumentFragment_get_previousSibling(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_previousSibling(pDocFrag, &node);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(node == NULL, "previous sibling not NULL\n");

                IXMLDOMDocumentFragment_Release(pDocFrag);
            }

            /* Entity References */
            hr = IXMLDOMDocument_createEntityReference(doc, NULL, &pEntityRef);
            ok(hr == E_FAIL, "ret %08x\n", hr );
            hr = IXMLDOMDocument_createEntityReference(doc, _bstr_(""), &pEntityRef);
            ok(hr == E_FAIL, "ret %08x\n", hr );

            str = SysAllocString(szEntityRef);
            hr = IXMLDOMDocument_createEntityReference(doc, str, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            hr = IXMLDOMDocument_createEntityReference(doc, str, &pEntityRef);
            SysFreeString(str);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pEntityRef, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* test get_xml*/
                hr = IXMLDOMEntityReference_get_xml(pEntityRef, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szEntityRefXML ), "incorrect xml string\n");
                SysFreeString(str);

                IXMLDOMEntityReference_Release(pEntityRef);
            }

            IXMLDOMElement_Release( pRoot );
        }
    }

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_nodeTypeTests( void )
{
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *pRoot;
    IXMLDOMElement *pElement;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &pRoot);
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)pRoot, NULL);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMElement_put_dataType(pRoot, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            /* Invalid Value */
            hr = IXMLDOMElement_put_dataType(pRoot, _bstr_("abcdefg") );
            ok(hr == E_FAIL, "ret %08x\n", hr );

            /* NOTE:
             *   The name passed into put_dataType is case-insensitive. So many of the names
             *     have been changed to reflect this.
             */
            /* Boolean */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Boolean"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("Boolean") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* String */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_String"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("String") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Number */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Number"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("number") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Int */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Int"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("InT") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Fixed */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Fixed"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("fixed.14.4") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* DateTime */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_DateTime"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("DateTime") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* DateTime TZ */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_DateTime_tz"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("DateTime.tz") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Date */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Date"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("Date") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Time */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Time"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("Time") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Time.tz */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Time_TZ"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("Time.tz") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* I1 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_I1"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("I1") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* I2 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_I2"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("I2") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* I4 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_I4"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("I4") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* UI1 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_UI1"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("UI1") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* UI2 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_UI2"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("UI2") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* UI4 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_UI4"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("UI4") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* r4 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_r4"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("r4") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* r8 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_r8"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("r8") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* float */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_float"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("float") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* uuid */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_uuid"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("UuId") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* bin.hex */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_bin_hex"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("bin.hex") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* bin.base64 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_bin_base64"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("bin.base64") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Check changing types */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Change"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("DateTime.tz") );
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("string") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            IXMLDOMElement_Release(pRoot);
        }
    }

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_save(void)
{
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMElement *root;
    VARIANT file, vDoc;
    BSTR sOrig, sNew, filename;
    char buffer[100];
    DWORD read = 0;
    HANDLE hfile;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    doc2 = create_document(&IID_IXMLDOMDocument);
    if (!doc2)
    {
        IXMLDOMDocument_Release(doc);
        return;
    }

    /* save to IXMLDOMDocument */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &root);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)root, NULL);
    EXPECT_HR(hr, S_OK);

    V_VT(&vDoc) = VT_UNKNOWN;
    V_UNKNOWN(&vDoc) = (IUnknown*)doc2;

    hr = IXMLDOMDocument_save(doc, vDoc);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_get_xml(doc, &sOrig);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_get_xml(doc2, &sNew);
    EXPECT_HR(hr, S_OK);

    ok( !lstrcmpW( sOrig, sNew ), "New document is not the same as original\n");

    SysFreeString(sOrig);
    SysFreeString(sNew);

    IXMLDOMElement_Release(root);
    IXMLDOMDocument_Release(doc2);

    /* save to path */
    V_VT(&file) = VT_BSTR;
    V_BSTR(&file) = _bstr_("test.xml");

    hr = IXMLDOMDocument_save(doc, file);
    EXPECT_HR(hr, S_OK);

    hfile = CreateFileA("test.xml", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(hfile != INVALID_HANDLE_VALUE, "Could not open file: %u\n", GetLastError());
    if(hfile == INVALID_HANDLE_VALUE) return;

    ReadFile(hfile, buffer, sizeof(buffer), &read, NULL);
    ok(read != 0, "could not read file\n");
    ok(buffer[0] != '<' || buffer[1] != '?', "File contains processing instruction\n");

    CloseHandle(hfile);
    DeleteFile("test.xml");

    /* save to path VT_BSTR | VT_BYREF */
    filename = _bstr_("test.xml");
    V_VT(&file) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&file) = &filename;

    hr = IXMLDOMDocument_save(doc, file);
    EXPECT_HR(hr, S_OK);

    IXMLDOMDocument_Release(doc);

    hfile = CreateFileA("test.xml", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(hfile != INVALID_HANDLE_VALUE, "Could not open file: %u\n", GetLastError());
    if(hfile == INVALID_HANDLE_VALUE) return;

    ReadFile(hfile, buffer, sizeof(buffer), &read, NULL);
    ok(read != 0, "could not read file\n");
    ok(buffer[0] != '<' || buffer[1] != '?', "File contains processing instruction\n");

    CloseHandle(hfile);
    DeleteFile("test.xml");
    free_bstrs();
}

static void test_testTransforms(void)
{
    IXMLDOMDocument *doc, *docSS;
    IXMLDOMNode *pNode;
    VARIANT_BOOL bSucc;

    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    docSS = create_document(&IID_IXMLDOMDocument);
    if (!docSS)
    {
        IXMLDOMDocument_Release(doc);
        return;
    }

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTransformXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );

    hr = IXMLDOMDocument_loadXML(docSS, _bstr_(szTransformSSXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );

    hr = IXMLDOMDocument_QueryInterface(docSS, &IID_IXMLDOMNode, (void**)&pNode );
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        BSTR bOut;

        hr = IXMLDOMDocument_transformNode(doc, pNode, &bOut);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            ok( compareIgnoreReturns( bOut, _bstr_(szTransformOutput)), "Stylesheet output not correct\n");
            SysFreeString(bOut);
        }

        IXMLDOMNode_Release(pNode);
    }

    IXMLDOMDocument_Release(docSS);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_namespaces(void)
{
    static const CHAR namespaces_xmlA[] =
        "<?xml version=\"1.0\"?>\n"
        "<XMI xmi.version=\"1.1\" xmlns:Model=\"http://omg.org/mof.Model/1.3\">"
        "  <XMI.content>"
        "    <Model:Package name=\"WinePackage\" Model:name2=\"name2 attr\" />"
        "  </XMI.content>"
        "</XMI>";

    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    IXMLDOMNode *node;

    VARIANT_BOOL b;
    VARIANT var;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(namespaces_xmlA), &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("//XMI.content"), &node );
    EXPECT_HR(hr, S_OK);
    if(hr == S_OK)
    {
        IXMLDOMAttribute *attr;
        IXMLDOMNode *node2;

        hr = IXMLDOMNode_get_firstChild(node, &node2);
        EXPECT_HR(hr, S_OK);
        ok(node2 != NULL, "got %p\n", node2);

        /* Test get_prefix */
        hr = IXMLDOMNode_get_prefix(node2, NULL);
        EXPECT_HR(hr, E_INVALIDARG);
        /* NOTE: Need to test that arg2 gets cleared on Error. */

        hr = IXMLDOMNode_get_prefix(node2, &str);
        EXPECT_HR(hr, S_OK);
        ok( !lstrcmpW( str, _bstr_("Model")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_get_nodeName(node2, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW( str, _bstr_("Model:Package")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* Test get_namespaceURI */
        hr = IXMLDOMNode_get_namespaceURI(node2, NULL);
        EXPECT_HR(hr, E_INVALIDARG);
        /* NOTE: Need to test that arg2 gets cleared on Error. */

        hr = IXMLDOMNode_get_namespaceURI(node2, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW( str, _bstr_("http://omg.org/mof.Model/1.3")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_QueryInterface(node2, &IID_IXMLDOMElement, (void**)&elem);
        EXPECT_HR(hr, S_OK);

        hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("Model:name2"), &attr);
        EXPECT_HR(hr, S_OK);

        hr = IXMLDOMAttribute_get_nodeName(attr, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW( str, _bstr_("Model:name2")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMAttribute_get_prefix(attr, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW( str, _bstr_("Model")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMAttribute_Release(attr);
        IXMLDOMElement_Release(elem);

        IXMLDOMNode_Release(node2);
        IXMLDOMNode_Release(node);
    }

    IXMLDOMDocument_Release(doc);

    /* create on element and try to alter namespace after that */
    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    V_VT(&var) = VT_I2;
    V_I2(&var) = NODE_ELEMENT;

    hr = IXMLDOMDocument_createNode(doc, var, _bstr_("ns:elem"), _bstr_("ns/uri"), &node);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_appendChild(doc, node, NULL);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    EXPECT_HR(hr, S_OK);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = _bstr_("ns/uri2");

    hr = IXMLDOMElement_setAttribute(elem, _bstr_("xmlns:ns"), var);
    EXPECT_HR(hr, E_INVALIDARG);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = _bstr_("ns/uri");

    hr = IXMLDOMElement_setAttribute(elem, _bstr_("xmlns:ns"), var);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMElement_get_xml(elem, &str);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(str, _bstr_("<ns:elem xmlns:ns=\"ns/uri\"/>")), "got element %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);

    /* create on element and try to alter namespace after that */
    doc = create_document_version(60, &IID_IXMLDOMDocument);
    if (!doc) return;

    V_VT(&var) = VT_I2;
    V_I2(&var) = NODE_ELEMENT;

    hr = IXMLDOMDocument_createNode(doc, var, _bstr_("ns:elem"), _bstr_("ns/uri"), &node);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_appendChild(doc, node, NULL);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    EXPECT_HR(hr, S_OK);

    /* try same prefix, different uri */
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = _bstr_("ns/uri2");

    hr = IXMLDOMElement_setAttribute(elem, _bstr_("xmlns:ns"), var);
    EXPECT_HR(hr, E_INVALIDARG);

    /* try same prefix and uri */
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = _bstr_("ns/uri");

    hr = IXMLDOMElement_setAttribute(elem, _bstr_("xmlns:ns"), var);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMElement_get_xml(elem, &str);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(str, _bstr_("<ns:elem xmlns:ns=\"ns/uri\"/>")), "got element %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_FormattingXML(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *pElement;
    VARIANT_BOOL bSucc;
    HRESULT hr;
    BSTR str;
    static const CHAR szLinefeedXML[] = "<?xml version=\"1.0\"?>\n<Root>\n\t<Sub val=\"A\" />\n</Root>";
    static const CHAR szLinefeedRootXML[] = "<Root>\r\n\t<Sub val=\"A\"/>\r\n</Root>";

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szLinefeedXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");

    if(bSucc == VARIANT_TRUE)
    {
        hr = IXMLDOMDocument_get_documentElement(doc, &pElement);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMElement_get_xml(pElement, &str);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok( !lstrcmpW( str, _bstr_(szLinefeedRootXML) ), "incorrect element xml\n");
            SysFreeString(str);

            IXMLDOMElement_Release(pElement);
        }
    }

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

typedef struct _nodetypedvalue_t {
    const char *name;
    VARTYPE type;
    const char *value; /* value in string format */
} nodetypedvalue_t;

static const nodetypedvalue_t get_nodetypedvalue[] = {
    { "root/string",    VT_BSTR, "Wine" },
    { "root/string2",   VT_BSTR, "String" },
    { "root/number",    VT_BSTR, "12.44" },
    { "root/number2",   VT_BSTR, "-3.71e3" },
    { "root/int",       VT_I4,   "-13" },
    { "root/fixed",     VT_CY,   "7322.9371" },
    { "root/bool",      VT_BOOL, "-1" },
    { "root/datetime",  VT_DATE, "40135.14" },
    { "root/datetimetz",VT_DATE, "37813.59" },
    { "root/date",      VT_DATE, "665413" },
    { "root/time",      VT_DATE, "0.5813889" },
    { "root/timetz",    VT_DATE, "1.112512" },
    { "root/i1",        VT_I1,   "-13" },
    { "root/i2",        VT_I2,   "31915" },
    { "root/i4",        VT_I4,   "-312232" },
    { "root/ui1",       VT_UI1,  "123" },
    { "root/ui2",       VT_UI2,  "48282" },
    { "root/ui4",       VT_UI4,  "949281" },
    { "root/r4",        VT_R4,   "213124" },
    { "root/r8",        VT_R8,   "0.412" },
    { "root/float",     VT_R8,   "41221.421" },
    { "root/uuid",      VT_BSTR, "333C7BC4-460F-11D0-BC04-0080C7055a83" },
    { "root/binbase64", VT_ARRAY|VT_UI1, "base64 test" },
    { "root/binbase64_1", VT_ARRAY|VT_UI1, "base64 test" },
    { "root/binbase64_2", VT_ARRAY|VT_UI1, "base64 test" },
    { 0 }
};

static void test_nodeTypedValue(void)
{
    const nodetypedvalue_t *entry = get_nodetypedvalue;
    IXMLDOMDocumentType *doctype, *doctype2;
    IXMLDOMProcessingInstruction *pi;
    IXMLDOMDocumentFragment *frag;
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMCDATASection *cdata;
    IXMLDOMComment *comment;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    VARIANT value;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTypeValueXML), &b);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_get_nodeValue(doc, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = NULL;
    hr = IXMLDOMDocument_get_nodeValue(doc, &value);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok(V_VT(&value) == VT_NULL, "expect VT_NULL got %d\n", V_VT(&value));

    hr = IXMLDOMDocument_get_nodeTypedValue(doc, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    V_VT(&value) = VT_EMPTY;
    hr = IXMLDOMDocument_get_nodeTypedValue(doc, &value);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok(V_VT(&value) == VT_NULL, "got %d\n", V_VT(&value));

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("root/string"), &node);
    ok(hr == S_OK, "ret %08x\n", hr );

    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = NULL;
    hr = IXMLDOMNode_get_nodeValue(node, &value);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok(V_VT(&value) == VT_NULL, "expect VT_NULL got %d\n", V_VT(&value));

    hr = IXMLDOMNode_get_nodeTypedValue(node, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    IXMLDOMNode_Release(node);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("root/binhex"), &node);
    ok(hr == S_OK, "ret %08x\n", hr );
    {
        BYTE bytes[] = {0xff,0xfc,0xa0,0x12,0x00,0x3c};

        hr = IXMLDOMNode_get_nodeTypedValue(node, &value);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(V_VT(&value) == (VT_ARRAY|VT_UI1), "incorrect type\n");
        ok(V_ARRAY(&value)->rgsabound[0].cElements == 6, "incorrect array size\n");
        if(V_ARRAY(&value)->rgsabound[0].cElements == 6)
            ok(!memcmp(bytes, V_ARRAY(&value)->pvData, sizeof(bytes)), "incorrect value\n");
        VariantClear(&value);
        IXMLDOMNode_Release(node);
    }

    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("foo"), _bstr_("value"), &pi);
    ok(hr == S_OK, "ret %08x\n", hr );
    {
        V_VT(&value) = VT_NULL;
        V_BSTR(&value) = (void*)0xdeadbeef;
        hr = IXMLDOMProcessingInstruction_get_nodeTypedValue(pi, &value);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(V_VT(&value) == VT_BSTR, "got %d\n", V_VT(&value));
        ok(!lstrcmpW(V_BSTR(&value), _bstr_("value")), "got wrong value\n");
        IXMLDOMProcessingInstruction_Release(pi);
        VariantClear(&value);
    }

    hr = IXMLDOMDocument_createCDATASection(doc, _bstr_("[1]*2=3; &gee that's not right!"), &cdata);
    ok(hr == S_OK, "ret %08x\n", hr );
    {
        V_VT(&value) = VT_NULL;
        V_BSTR(&value) = (void*)0xdeadbeef;
        hr = IXMLDOMCDATASection_get_nodeTypedValue(cdata, &value);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(V_VT(&value) == VT_BSTR, "got %d\n", V_VT(&value));
        ok(!lstrcmpW(V_BSTR(&value), _bstr_("[1]*2=3; &gee that's not right!")), "got wrong value\n");
        IXMLDOMCDATASection_Release(cdata);
        VariantClear(&value);
    }

    hr = IXMLDOMDocument_createComment(doc, _bstr_("comment"), &comment);
    ok(hr == S_OK, "ret %08x\n", hr );
    {
        V_VT(&value) = VT_NULL;
        V_BSTR(&value) = (void*)0xdeadbeef;
        hr = IXMLDOMComment_get_nodeTypedValue(comment, &value);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(V_VT(&value) == VT_BSTR, "got %d\n", V_VT(&value));
        ok(!lstrcmpW(V_BSTR(&value), _bstr_("comment")), "got wrong value\n");
        IXMLDOMComment_Release(comment);
        VariantClear(&value);
    }

    hr = IXMLDOMDocument_createDocumentFragment(doc, &frag);
    ok(hr == S_OK, "ret %08x\n", hr );
    {
        V_VT(&value) = VT_EMPTY;
        hr = IXMLDOMDocumentFragment_get_nodeTypedValue(frag, &value);
        ok(hr == S_FALSE, "ret %08x\n", hr );
        ok(V_VT(&value) == VT_NULL, "got %d\n", V_VT(&value));
        IXMLDOMDocumentFragment_Release(frag);
    }

    doc2 = create_document(&IID_IXMLDOMDocument);

    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML(doc2, _bstr_(szEmailXML), &b);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(b == VARIANT_TRUE, "got %d\n", b);

    EXPECT_REF(doc2, 1);

    hr = IXMLDOMDocument_get_doctype(doc2, &doctype);
    ok(hr == S_OK, "ret %08x\n", hr );

    EXPECT_REF(doc2, 1);
    todo_wine EXPECT_REF(doctype, 2);

    {
        V_VT(&value) = VT_EMPTY;
        hr = IXMLDOMDocumentType_get_nodeTypedValue(doctype, &value);
        ok(hr == S_FALSE, "ret %08x\n", hr );
        ok(V_VT(&value) == VT_NULL, "got %d\n", V_VT(&value));
    }

    hr = IXMLDOMDocument_get_doctype(doc2, &doctype2);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(doctype != doctype2, "got %p, was %p\n", doctype2, doctype);

    IXMLDOMDocumentType_Release(doctype2);
    IXMLDOMDocumentType_Release(doctype);

    IXMLDOMDocument_Release(doc2);

    while (entry->name)
    {
        hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_(entry->name), &node);
        ok(hr == S_OK, "ret %08x\n", hr );

        hr = IXMLDOMNode_get_nodeTypedValue(node, &value);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(V_VT(&value) == entry->type, "incorrect type, expected %d, got %d\n", entry->type, V_VT(&value));

        if (entry->type == (VT_ARRAY|VT_UI1))
        {
            ok(V_ARRAY(&value)->rgsabound[0].cElements == strlen(entry->value),
               "incorrect array size %d\n", V_ARRAY(&value)->rgsabound[0].cElements);
        }

        if (entry->type != VT_BSTR)
        {
           if (entry->type == VT_DATE ||
               entry->type == VT_R8 ||
               entry->type == VT_CY)
           {
               if (entry->type == VT_DATE)
               {
                   hr = VariantChangeType(&value, &value, 0, VT_R4);
                   ok(hr == S_OK, "ret %08x\n", hr );
               }
               hr = VariantChangeTypeEx(&value, &value,
                                        MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT),
                                        VARIANT_NOUSEROVERRIDE, VT_BSTR);
               ok(hr == S_OK, "ret %08x\n", hr );
           }
           else
           {
               hr = VariantChangeType(&value, &value, 0, VT_BSTR);
               ok(hr == S_OK, "ret %08x\n", hr );
           }

           /* for byte array from VT_ARRAY|VT_UI1 it's not a WCHAR buffer */
           if (entry->type == (VT_ARRAY|VT_UI1))
           {
               ok(!memcmp( V_BSTR(&value), entry->value, strlen(entry->value)),
                  "expected %s\n", entry->value);
           }
           else
               ok(lstrcmpW( V_BSTR(&value), _bstr_(entry->value)) == 0,
                  "expected %s, got %s\n", entry->value, wine_dbgstr_w(V_BSTR(&value)));
        }
        else
           ok(lstrcmpW( V_BSTR(&value), _bstr_(entry->value)) == 0,
               "expected %s, got %s\n", entry->value, wine_dbgstr_w(V_BSTR(&value)));

        VariantClear( &value );
        IXMLDOMNode_Release(node);

        entry++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_TransformWithLoadingLocalFile(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMDocument *xsl;
    IXMLDOMNode *pNode;
    VARIANT_BOOL bSucc;
    HRESULT hr;
    HANDLE file;
    DWORD dwWritten;
    char lpPathBuffer[MAX_PATH];
    int i;

    /* Create a Temp File. */
    GetTempPathA(MAX_PATH, lpPathBuffer);
    strcat(lpPathBuffer, "customers.xml" );

    file = CreateFileA(lpPathBuffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, szBasicTransformXML, strlen(szBasicTransformXML), &dwWritten, NULL);
    CloseHandle(file);

    /* Correct path to not include a escape character. */
    for(i=0; i < strlen(lpPathBuffer); i++)
    {
        if(lpPathBuffer[i] == '\\')
            lpPathBuffer[i] = '/';
    }

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    xsl = create_document(&IID_IXMLDOMDocument);
    if (!xsl)
    {
        IXMLDOMDocument2_Release(doc);
        return;
    }

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTypeValueXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");
    if(bSucc == VARIANT_TRUE)
    {
        BSTR sXSL;
        BSTR sPart1 = _bstr_(szBasicTransformSSXMLPart1);
        BSTR sPart2 = _bstr_(szBasicTransformSSXMLPart2);
        BSTR sFileName = _bstr_(lpPathBuffer);
        int nLegnth = lstrlenW(sPart1) + lstrlenW(sPart2) + lstrlenW(sFileName) + 1;

        sXSL = SysAllocStringLen(NULL, nLegnth);
        lstrcpyW(sXSL, sPart1);
        lstrcatW(sXSL, sFileName);
        lstrcatW(sXSL, sPart2);

        hr = IXMLDOMDocument_loadXML(xsl, sXSL, &bSucc);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");
        if(bSucc == VARIANT_TRUE)
        {
            BSTR sResult;

            hr = IXMLDOMDocument_QueryInterface(xsl, &IID_IXMLDOMNode, (void**)&pNode );
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                /* This will load the temp file via the XSL */
                hr = IXMLDOMDocument_transformNode(doc, pNode, &sResult);
                ok(hr == S_OK, "ret %08x\n", hr );
                if(hr == S_OK)
                {
                    ok( compareIgnoreReturns( sResult, _bstr_(szBasicTransformOutput)), "Stylesheet output not correct\n");
                    SysFreeString(sResult);
                }

                IXMLDOMNode_Release(pNode);
            }
        }

        SysFreeString(sXSL);
    }

    IXMLDOMDocument_Release(doc);
    IXMLDOMDocument_Release(xsl);

    DeleteFile(lpPathBuffer);
    free_bstrs();
}

static void test_put_nodeValue(void)
{
    static const WCHAR jeevesW[] = {'J','e','e','v','e','s',' ','&',' ','W','o','o','s','t','e','r',0};
    IXMLDOMDocument *doc;
    IXMLDOMText *text;
    IXMLDOMEntityReference *entityref;
    IXMLDOMAttribute *attr;
    IXMLDOMNode *node;
    HRESULT hr;
    VARIANT data, type;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    /* test for unsupported types */
    /* NODE_DOCUMENT */
    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_DOCUMENT_FRAGMENT */
    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_DOCUMENT_FRAGMENT;
    hr = IXMLDOMDocument_createNode(doc, type, _bstr_("test"), NULL, &node);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_ELEMENT */
    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ELEMENT;
    hr = IXMLDOMDocument_createNode(doc, type, _bstr_("test"), NULL, &node);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_ENTITY_REFERENCE */
    hr = IXMLDOMDocument_createEntityReference(doc, _bstr_("ref"), &entityref);
    ok(hr == S_OK, "ret %08x\n", hr );

    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMEntityReference_put_nodeValue(entityref, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );

    hr = IXMLDOMEntityReference_QueryInterface(entityref, &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    IXMLDOMNode_Release(node);
    IXMLDOMEntityReference_Release(entityref);

    /* supported types */
    hr = IXMLDOMDocument_createTextNode(doc, _bstr_(""), &text);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("Jeeves & Wooster");
    hr = IXMLDOMText_put_nodeValue(text, data);
    ok(hr == S_OK, "ret %08x\n", hr );
    IXMLDOMText_Release(text);

    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("attr"), &attr);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("Jeeves & Wooster");
    hr = IXMLDOMAttribute_put_nodeValue(attr, data);
    ok(hr == S_OK, "ret %08x\n", hr );
    hr = IXMLDOMAttribute_get_nodeValue(attr, &data);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(memcmp(V_BSTR(&data), jeevesW, sizeof(jeevesW)) == 0, "got %s\n",
        wine_dbgstr_w(V_BSTR(&data)));
    VariantClear(&data);
    IXMLDOMAttribute_Release(attr);

    free_bstrs();

    IXMLDOMDocument_Release(doc);
}

static void test_document_IObjectSafety(void)
{
    IXMLDOMDocument *doc;
    IObjectSafety *safety;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IObjectSafety, (void**)&safety);
    ok(hr == S_OK, "ret %08x\n", hr );

    test_IObjectSafety_common(safety);

    IObjectSafety_Release(safety);

    IXMLDOMDocument_Release(doc);
}

typedef struct _property_test_t {
    const GUID *guid;
    const char *clsid;
    const char *property;
    const char *value;
} property_test_t;

static const property_test_t properties_test_data[] = {
    { &CLSID_DOMDocument,  "CLSID_DOMDocument" , "SelectionLanguage", "XSLPattern" },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2" , "SelectionLanguage", "XSLPattern" },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30", "SelectionLanguage", "XSLPattern" },
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40", "SelectionLanguage", "XPath" },
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60", "SelectionLanguage", "XPath" },
    { 0 }
};

static void test_default_properties(void)
{
    const property_test_t *entry = properties_test_data;

    while (entry->guid)
    {
        IXMLDOMDocument2 *doc;
        VARIANT var;
        HRESULT hr;

        hr = CoCreateInstance(entry->guid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void**)&doc);
        if (hr != S_OK)
        {
            win_skip("can't create %s instance\n", entry->clsid);
            entry++;
            continue;
        }

        hr = IXMLDOMDocument2_getProperty(doc, _bstr_(entry->property), &var);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(lstrcmpW(V_BSTR(&var), _bstr_(entry->value)) == 0, "expected %s, for %s\n",
           entry->value, entry->clsid);
        VariantClear(&var);

        IXMLDOMDocument2_Release(doc);

        entry++;
    }
}

typedef struct {
    const char *query;
    const char *list;
} xslpattern_test_t;

static const xslpattern_test_t xslpattern_test[] = {
    { "root//elem[0]", "E1.E2.D1" },
    { "root//elem[index()=1]", "E2.E2.D1" },
    { "root//elem[index() $eq$ 1]", "E2.E2.D1" },
    { "root//elem[end()]", "E4.E2.D1" },
    { "root//elem[$not$ end()]", "E1.E2.D1 E2.E2.D1 E3.E2.D1" },
    { "root//elem[index() != 0]", "E2.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() $ne$ 0]", "E2.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() < 2]", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[index() $lt$ 2]", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[index() <= 1]", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[index() $le$ 1]", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[index() > 1]", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() $gt$ 1]", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() >= 2]", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() $ge$ 2]", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[a $ieq$ 'a2 field']", "E2.E2.D1" },
    { "root//elem[a $ine$ 'a2 field']", "E1.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root//elem[a $ilt$ 'a3 field']", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[a $ile$ 'a2 field']", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[a $igt$ 'a2 field']", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[a $ige$ 'a3 field']", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[$any$ *='B2 field']", "E2.E2.D1" },
    { "root//elem[$all$ *!='B2 field']", "E1.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root//elem[index()=0 or end()]", "E1.E2.D1 E4.E2.D1" },
    { "root//elem[index()=0 $or$ end()]", "E1.E2.D1 E4.E2.D1" },
    { "root//elem[index()=0 || end()]", "E1.E2.D1 E4.E2.D1" },
    { "root//elem[index()>0 and $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { "root//elem[index()>0 $and$ $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { "root//elem[index()>0 && $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { NULL }
};

static const xslpattern_test_t xslpattern_test_no_ns[] = {
    /* prefixes don't need to be registered, you may use them as they are in the doc */
    { "//bar:x", "E5.E1.E4.E1.E2.D1 E6.E2.E4.E1.E2.D1" },
    /* prefixes must be explicitly specified in the name */
    { "//foo:elem", "" },
    { "//foo:c", "E3.E4.E2.D1" },
    { NULL }
};

static const xslpattern_test_t xslpattern_test_func[] = {
    { "attribute()", "" },
    { "attribute('depth')", "" },
    { "root/attribute('depth')", "A'depth'.E3.D1" },
    { "//x/attribute()", "A'id'.E3.E3.D1 A'depth'.E3.E3.D1" },
    { "//x//attribute(id)", NULL },
    { "//x//attribute('id')", "A'id'.E3.E3.D1 A'id'.E4.E3.E3.D1 A'id'.E5.E3.E3.D1 A'id'.E6.E3.E3.D1" },
    { "comment()", "C2.D1" },
    { "//comment()", "C2.D1 C1.E3.D1 C2.E3.E3.D1 C2.E4.E3.D1" },
    { "element()", "E3.D1" },
    { "root/y/element()", "E4.E4.E3.D1 E5.E4.E3.D1 E6.E4.E3.D1" },
    { "//element(a)", NULL },
    { "//element('a')", "E4.E3.E3.D1 E4.E4.E3.D1" },
    { "node()", "P1.D1 C2.D1 E3.D1" },
    { "//x/node()", "P1.E3.E3.D1 C2.E3.E3.D1 T3.E3.E3.D1 E4.E3.E3.D1 E5.E3.E3.D1 E6.E3.E3.D1" },
    { "//x/node()[nodeType()=1]", "E4.E3.E3.D1 E5.E3.E3.D1 E6.E3.E3.D1" },
    { "//x/node()[nodeType()=3]", "T3.E3.E3.D1" },
    { "//x/node()[nodeType()=7]", "P1.E3.E3.D1" },
    { "//x/node()[nodeType()=8]", "C2.E3.E3.D1" },
    { "pi()", "P1.D1" },
    { "//y/pi()", "P1.E4.E3.D1" },
    { "root/textnode()", "T2.E3.D1" },
    { "root/element()/textnode()", "T3.E3.E3.D1 T3.E4.E3.D1" },
    { NULL }
};

static void test_XSLPattern(void)
{
    const xslpattern_test_t *ptr = xslpattern_test;
    IXMLDOMDocument2 *doc;
    IXMLDOMNodeList *list;
    VARIANT_BOOL b;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument2);
    if (!doc) return;

    b = VARIANT_FALSE;
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* switch to XSLPattern */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern"));
    EXPECT_HR(hr, S_OK);

    /* XPath doesn't select elements with non-null default namespace with unqualified selectors, XSLPattern does */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("//elem/c"), &list);
    EXPECT_HR(hr, S_OK);

    len = 0;
    hr = IXMLDOMNodeList_get_length(list, &len);
    EXPECT_HR(hr, S_OK);
    /* should select <elem><c> and <elem xmlns='...'><c> but not <elem><foo:c> */
    ok(len == 3, "expected 3 entries in list, got %d\n", len);
    IXMLDOMNodeList_Release(list);

    while (ptr->query)
    {
        list = NULL;
        hr = IXMLDOMDocument2_selectNodes(doc, _bstr_(ptr->query), &list);
        ok(hr == S_OK, "query=%s, failed with 0x%08x\n", ptr->query, hr);
        len = 0;
        hr = IXMLDOMNodeList_get_length(list, &len);
        ok(len != 0, "query=%s, empty list\n", ptr->query);
        if (len)
            expect_list_and_release(list, ptr->list);

        ptr++;
    }

    /* namespace handling */
    /* no registered namespaces */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_(""));
    EXPECT_HR(hr, S_OK);

    ptr = xslpattern_test_no_ns;
    while (ptr->query)
    {
        list = NULL;
        hr = IXMLDOMDocument2_selectNodes(doc, _bstr_(ptr->query), &list);
        ok(hr == S_OK, "query=%s, failed with 0x%08x\n", ptr->query, hr);

        if (*ptr->list)
        {
            len = 0;
            hr = IXMLDOMNodeList_get_length(list, &len);
            EXPECT_HR(hr, S_OK);
            ok(len != 0, "query=%s, empty list\n", ptr->query);
        }
        else
        {
            len = 1;
            hr = IXMLDOMNodeList_get_length(list, &len);
            EXPECT_HR(hr, S_OK);
            ok(len == 0, "query=%s, empty list\n", ptr->query);
        }
        if (len)
            expect_list_and_release(list, ptr->list);

        ptr++;
    }

    /* explicitly register prefix foo */
    ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_("xmlns:foo='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'")));

    /* now we get the same behavior as XPath */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("//foo:c"), &list);
    EXPECT_HR(hr, S_OK);
    len = 0;
    hr = IXMLDOMNodeList_get_length(list, &len);
    EXPECT_HR(hr, S_OK);
    ok(len != 0, "expected filled list\n");
    if (len)
        expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");

    /* set prefix foo to some nonexistent namespace */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_("xmlns:foo='urn:nonexistent-foo'"));
    EXPECT_HR(hr, S_OK);

    /* the registered prefix takes precedence */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("//foo:c"), &list);
    EXPECT_HR(hr, S_OK);
    len = 0;
    hr = IXMLDOMNodeList_get_length(list, &len);
    EXPECT_HR(hr, S_OK);
    ok(len == 0, "expected empty list\n");
    IXMLDOMNodeList_Release(list);

    IXMLDOMDocument2_Release(doc);

    doc = create_document(&IID_IXMLDOMDocument2);
    if (!doc) return;

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szNodeTypesXML), &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    ptr = xslpattern_test_func;
    while (ptr->query)
    {
        list = NULL;
        hr = IXMLDOMDocument2_selectNodes(doc, _bstr_(ptr->query), &list);
        if (ptr->list)
        {
            ok(hr == S_OK, "query=%s, failed with 0x%08x\n", ptr->query, hr);
            len = 0;
            hr = IXMLDOMNodeList_get_length(list, &len);
            if (*ptr->list)
            {
                ok(len != 0, "query=%s, empty list\n", ptr->query);
                if (len)
                    expect_list_and_release(list, ptr->list);
            }
            else
                ok(len == 0, "query=%s, filled list\n", ptr->query);
        }
        else
            ok(hr == E_FAIL, "query=%s, failed with 0x%08x\n", ptr->query, hr);

        ptr++;
    }

    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

static void test_splitText(void)
{
    IXMLDOMCDATASection *cdata;
    IXMLDOMElement *root;
    IXMLDOMDocument *doc;
    IXMLDOMText *text, *text2;
    IXMLDOMNode *node;
    VARIANT var;
    VARIANT_BOOL success;
    LONG length;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_loadXML(doc, _bstr_("<root></root>"), &success);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &root);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_createCDATASection(doc, _bstr_("beautiful plumage"), &cdata);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    V_VT(&var) = VT_EMPTY;
    hr = IXMLDOMElement_appendChild(root, (IXMLDOMNode*)cdata, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    length = 0;
    hr = IXMLDOMCDATASection_get_length(cdata, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(length > 0, "got %d\n", length);

    hr = IXMLDOMCDATASection_splitText(cdata, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    text = (void*)0xdeadbeef;
    /* negative offset */
    hr = IXMLDOMCDATASection_splitText(cdata, -1, &text);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(text == (void*)0xdeadbeef, "got %p\n", text);

    text = (void*)0xdeadbeef;
    /* offset outside data */
    hr = IXMLDOMCDATASection_splitText(cdata, length + 1, &text);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(text == 0, "got %p\n", text);

    text = (void*)0xdeadbeef;
    /* offset outside data */
    hr = IXMLDOMCDATASection_splitText(cdata, length, &text);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(text == 0, "got %p\n", text);

    /* no empty node created */
    node = (void*)0xdeadbeef;
    hr = IXMLDOMCDATASection_get_nextSibling(cdata, &node);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(node == 0, "got %p\n", text);

    hr = IXMLDOMCDATASection_splitText(cdata, 10, &text);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    length = 0;
    hr = IXMLDOMText_get_length(text, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(length == 7, "got %d\n", length);

    hr = IXMLDOMCDATASection_get_nextSibling(cdata, &node);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IXMLDOMNode_Release(node);

    /* split new text node */
    hr = IXMLDOMText_get_length(text, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMText_get_nextSibling(text, &node);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(node == 0, "got %p\n", text);

    hr = IXMLDOMText_splitText(text, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    text2 = (void*)0xdeadbeef;
    /* negative offset */
    hr = IXMLDOMText_splitText(text, -1, &text2);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(text2 == (void*)0xdeadbeef, "got %p\n", text2);

    text2 = (void*)0xdeadbeef;
    /* offset outside data */
    hr = IXMLDOMText_splitText(text, length + 1, &text2);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(text2 == 0, "got %p\n", text2);

    text2 = (void*)0xdeadbeef;
    /* offset outside data */
    hr = IXMLDOMText_splitText(text, length, &text2);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(text2 == 0, "got %p\n", text);

    text2 = 0;
    hr = IXMLDOMText_splitText(text, 4, &text2);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    if (text2) IXMLDOMText_Release(text2);

    node = 0;
    hr = IXMLDOMText_get_nextSibling(text, &node);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    if (node) IXMLDOMNode_Release(node);

    IXMLDOMText_Release(text);
    IXMLDOMElement_Release(root);
    IXMLDOMCDATASection_Release(cdata);
    free_bstrs();
}

typedef struct {
    const char *name;
    const char *uri;
    HRESULT hr;
} ns_item_t;

/* default_ns_doc used */
static const ns_item_t qualified_item_tests[] = {
    { "xml:lang", NULL, S_FALSE },
    { "xml:lang", "http://www.w3.org/XML/1998/namespace", S_FALSE },
    { "lang", "http://www.w3.org/XML/1998/namespace", S_OK },
    { "ns:b", NULL, S_FALSE },
    { "ns:b", "nshref", S_FALSE },
    { "b", "nshref", S_OK },
    { "d", NULL, S_OK },
    { NULL }
};

static const ns_item_t named_item_tests[] = {
    { "xml:lang", NULL, S_OK },
    { "lang", NULL, S_FALSE },
    { "ns:b", NULL, S_OK },
    { "b", NULL, S_FALSE },
    { "d", NULL, S_OK },
    { NULL }
};

static void test_getQualifiedItem(void)
{
    IXMLDOMNode *pr_node, *node;
    IXMLDOMNodeList *root_list;
    IXMLDOMNamedNodeMap *map;
    IXMLDOMElement *element;
    const ns_item_t* ptr;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    EXPECT_HR(hr, S_OK);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMElement_get_childNodes(element, &root_list);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNodeList_get_item(root_list, 1, &pr_node);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNodeList_Release(root_list);

    hr = IXMLDOMNode_get_attributes(pr_node, &map);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNode_Release(pr_node);

    len = 0;
    hr = IXMLDOMNamedNodeMap_get_length(map, &len);
    EXPECT_HR(hr, S_OK);
    ok( len == 3, "length %d\n", len);

    hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, NULL, NULL, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, NULL, NULL, &node);
    EXPECT_HR(hr, E_INVALIDARG);
    ok( node == (void*)0xdeadbeef, "got %p\n", node);

    hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, _bstr_("id"), NULL, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, _bstr_("id"), NULL, &node);
    EXPECT_HR(hr, S_OK);

    IXMLDOMNode_Release(node);
    IXMLDOMNamedNodeMap_Release(map);
    IXMLDOMElement_Release(element);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(default_ns_doc), &b);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("a"), &node);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&element);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMElement_get_attributes(element, &map);
    EXPECT_HR(hr, S_OK);

    ptr = qualified_item_tests;
    while (ptr->name)
    {
       node = (void*)0xdeadbeef;
       hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, _bstr_(ptr->name), _bstr_(ptr->uri), &node);
       ok(hr == ptr->hr, "%s, %s: got 0x%08x, expected 0x%08x\n", ptr->name, ptr->uri, hr, ptr->hr);
       if (hr == S_OK)
           IXMLDOMNode_Release(node);
       else
           ok(node == NULL, "%s, %s: got %p\n", ptr->name, ptr->uri, node);
       ptr++;
    }

    ptr = named_item_tests;
    while (ptr->name)
    {
       node = (void*)0xdeadbeef;
       hr = IXMLDOMNamedNodeMap_getNamedItem(map, _bstr_(ptr->name), &node);
       ok(hr == ptr->hr, "%s: got 0x%08x, expected 0x%08x\n", ptr->name, hr, ptr->hr);
       if (hr == S_OK)
           IXMLDOMNode_Release(node);
       else
           ok(node == NULL, "%s: got %p\n", ptr->name, node);
       ptr++;
    }

    IXMLDOMNamedNodeMap_Release(map);

    IXMLDOMElement_Release(element);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_removeQualifiedItem(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *pr_node, *node;
    IXMLDOMNodeList *root_list;
    IXMLDOMNamedNodeMap *map;
    VARIANT_BOOL b;
    LONG len;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    ok( hr == S_OK, "ret %08x\n", hr);

    hr = IXMLDOMElement_get_childNodes(element, &root_list);
    ok( hr == S_OK, "ret %08x\n", hr);

    hr = IXMLDOMNodeList_get_item(root_list, 1, &pr_node);
    ok( hr == S_OK, "ret %08x\n", hr);
    IXMLDOMNodeList_Release(root_list);

    hr = IXMLDOMNode_get_attributes(pr_node, &map);
    ok( hr == S_OK, "ret %08x\n", hr);
    IXMLDOMNode_Release(pr_node);

    hr = IXMLDOMNamedNodeMap_get_length(map, &len);
    ok( hr == S_OK, "ret %08x\n", hr);
    ok( len == 3, "length %d\n", len);

    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, NULL, NULL, NULL);
    ok( hr == E_INVALIDARG, "ret %08x\n", hr);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, NULL, NULL, &node);
    ok( hr == E_INVALIDARG, "ret %08x\n", hr);
    ok( node == (void*)0xdeadbeef, "got %p\n", node);

    /* out pointer is optional */
    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, _bstr_("id"), NULL, NULL);
    ok( hr == S_OK, "ret %08x\n", hr);

    /* already removed */
    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, _bstr_("id"), NULL, NULL);
    ok( hr == S_FALSE, "ret %08x\n", hr);

    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, _bstr_("vr"), NULL, &node);
    ok( hr == S_OK, "ret %08x\n", hr);
    IXMLDOMNode_Release(node);

    IXMLDOMNamedNodeMap_Release( map );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
    free_bstrs();
}

#define check_default_props(doc) _check_default_props(__LINE__, doc)
static inline void _check_default_props(int line, IXMLDOMDocument2* doc)
{
    VARIANT_BOOL b;
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    helper_ole_check(IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionLanguage"), &var));
    ok_(__FILE__, line)(lstrcmpW(V_BSTR(&var), _bstr_("XSLPattern")) == 0, "expected XSLPattern\n");
    VariantClear(&var);

    helper_ole_check(IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var));
    ok_(__FILE__, line)(lstrcmpW(V_BSTR(&var), _bstr_("")) == 0, "expected empty string\n");
    VariantClear(&var);

    helper_ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc, &b));
    ok_(__FILE__, line)(b == VARIANT_FALSE, "expected FALSE\n");

    hr = IXMLDOMDocument2_get_schemas(doc, &var);
    ok_(__FILE__, line)(hr == S_FALSE, "got %08x\n", hr);
    VariantClear(&var);
}

#define check_set_props(doc) _check_set_props(__LINE__, doc)
static inline void _check_set_props(int line, IXMLDOMDocument2* doc)
{
    VARIANT_BOOL b;
    VARIANT var;

    VariantInit(&var);
    helper_ole_check(IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionLanguage"), &var));
    ok_(__FILE__, line)(lstrcmpW(V_BSTR(&var), _bstr_("XPath")) == 0, "expected XPath\n");
    VariantClear(&var);

    helper_ole_check(IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var));
    ok_(__FILE__, line)(lstrcmpW(V_BSTR(&var), _bstr_("xmlns:wi=\'www.winehq.org\'")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);

    helper_ole_check(IXMLDOMDocument2_get_preserveWhiteSpace(doc, &b));
    ok_(__FILE__, line)(b == VARIANT_TRUE, "expected TRUE\n");

    helper_ole_check(IXMLDOMDocument2_get_schemas(doc, &var));
    ok_(__FILE__, line)(V_VT(&var) != VT_NULL, "expected pointer\n");
    VariantClear(&var);
}

#define set_props(doc, cache) _set_props(__LINE__, doc, cache)
static inline void _set_props(int line, IXMLDOMDocument2* doc, IXMLDOMSchemaCollection* cache)
{
    VARIANT var;

    VariantInit(&var);
    helper_ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));
    helper_ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_("xmlns:wi=\'www.winehq.org\'")));
    helper_ole_check(IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_TRUE));
    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = NULL;
    helper_ole_check(IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&var)));
    ok_(__FILE__, line)(V_DISPATCH(&var) != NULL, "expected pointer\n");
    helper_ole_check(IXMLDOMDocument2_putref_schemas(doc, var));
    VariantClear(&var);
}

#define unset_props(doc) _unset_props(__LINE__, doc)
static inline void _unset_props(int line, IXMLDOMDocument2* doc)
{
    VARIANT var;

    VariantInit(&var);
    helper_ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern")));
    helper_ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_("")));
    helper_ole_check(IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_FALSE));
    V_VT(&var) = VT_NULL;
    helper_ole_check(IXMLDOMDocument2_putref_schemas(doc, var));
    VariantClear(&var);
}

static void test_get_ownerDocument(void)
{
    IXMLDOMDocument *doc1, *doc2, *doc3;
    IXMLDOMDocument2 *doc, *doc_owner;
    IXMLDOMNode *node;
    IXMLDOMSchemaCollection *cache;
    VARIANT_BOOL b;
    VARIANT var;

    doc = create_document(&IID_IXMLDOMDocument2);
    cache = create_cache(&IID_IXMLDOMSchemaCollection);
    if (!doc || !cache)
    {
        if (doc) IXMLDOMDocument2_Release(doc);
        if (cache) IXMLDOMSchemaCollection_Release(cache);
        return;
    }

    VariantInit(&var);

    ole_check(IXMLDOMDocument_loadXML(doc, _bstr_(complete4A), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    check_default_props(doc);

    /* set properties and check that new instances use them */
    set_props(doc, cache);
    check_set_props(doc);

    ole_check(IXMLDOMDocument2_get_firstChild(doc, &node));
    ole_check(IXMLDOMNode_get_ownerDocument(node, &doc1));

    /* new interface keeps props */
    ole_check(IXMLDOMDocument_QueryInterface(doc1, &IID_IXMLDOMDocument2, (void**)&doc_owner));
    ok( doc_owner != doc, "got %p, doc %p\n", doc_owner, doc);
    check_set_props(doc_owner);
    IXMLDOMDocument2_Release(doc_owner);

    ole_check(IXMLDOMNode_get_ownerDocument(node, &doc2));
    IXMLDOMNode_Release(node);

    ok(doc1 != doc2, "got %p, expected %p. original %p\n", doc2, doc1, doc);

    /* reload */
    ole_check(IXMLDOMDocument2_loadXML(doc, _bstr_(complete4A), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* properties retained even after reload */
    check_set_props(doc);

    ole_check(IXMLDOMDocument2_get_firstChild(doc, &node));
    ole_check(IXMLDOMNode_get_ownerDocument(node, &doc3));
    IXMLDOMNode_Release(node);

    ole_check(IXMLDOMDocument_QueryInterface(doc3, &IID_IXMLDOMDocument2, (void**)&doc_owner));
    ok(doc3 != doc1 && doc3 != doc2 && doc_owner != doc, "got %p, (%p, %p, %p)\n", doc3, doc, doc1, doc2);
    check_set_props(doc_owner);

    /* changing properties for one instance changes them for all */
    unset_props(doc_owner);
    check_default_props(doc_owner);
    check_default_props(doc);

    IXMLDOMSchemaCollection_Release(cache);
    IXMLDOMDocument_Release(doc1);
    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc3);
    IXMLDOMDocument2_Release(doc);
    IXMLDOMDocument2_Release(doc_owner);
    free_bstrs();
}

static void test_setAttributeNode(void)
{
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMElement *elem, *elem2;
    IXMLDOMAttribute *attr, *attr2, *ret_attr;
    VARIANT_BOOL b;
    HRESULT hr;
    VARIANT v;
    BSTR str;
    ULONG ref1, ref2;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument2_loadXML( doc, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem2);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok( elem2 != elem, "got same instance\n");

    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, NULL, &ret_attr);
    ok( hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok( ret_attr == (void*)0xdeadbeef, "got %p\n", ret_attr);

    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("attr"), &attr);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    ref1 = IXMLDOMElement_AddRef(elem);
    IXMLDOMElement_Release(elem);

    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, attr, &ret_attr);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok( ret_attr == NULL, "got %p\n", ret_attr);

    /* no reference added */
    ref2 = IXMLDOMElement_AddRef(elem);
    IXMLDOMElement_Release(elem);
    ok(ref2 == ref1, "got %d, expected %d\n", ref2, ref1);

    EXPECT_CHILDREN(elem);
    EXPECT_CHILDREN(elem2);

    IXMLDOMElement_Release(elem2);

    attr2 = NULL;
    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("attr"), &attr2);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok( attr2 != attr, "got same instance %p\n", attr2);
    IXMLDOMAttribute_Release(attr2);

    /* try to add it another time */
    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, attr, &ret_attr);
    ok( hr == E_FAIL, "got 0x%08x\n", hr);
    ok( ret_attr == (void*)0xdeadbeef, "got %p\n", ret_attr);

    IXMLDOMElement_Release(elem);

    /* initially used element is released, attribute still 'has' a container */
    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, attr, &ret_attr);
    ok( hr == E_FAIL, "got 0x%08x\n", hr);
    ok( ret_attr == (void*)0xdeadbeef, "got %p\n", ret_attr);
    IXMLDOMElement_Release(elem);

    /* add attribute already attached to another document */
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc2, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc2, &elem);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    hr = IXMLDOMElement_setAttributeNode(elem, attr, NULL);
    ok( hr == E_FAIL, "got 0x%08x\n", hr);
    IXMLDOMElement_Release(elem);

    IXMLDOMAttribute_Release(attr);

    /* create element, add attribute, see if it's copied or linked */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("test"), &elem);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    attr = NULL;
    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("attr"), &attr);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(attr != NULL, "got %p\n", attr);

    ref1 = IXMLDOMAttribute_AddRef(attr);
    IXMLDOMAttribute_Release(attr);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_("attrvalue1");
    hr = IXMLDOMAttribute_put_nodeValue(attr, v);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    str = NULL;
    hr = IXMLDOMAttribute_get_xml(attr, &str);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok( lstrcmpW(str, _bstr_("attr=\"attrvalue1\"")) == 0,
        "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, attr, &ret_attr);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret_attr == NULL, "got %p\n", ret_attr);

    /* attribute reference increased */
    ref2 = IXMLDOMAttribute_AddRef(attr);
    IXMLDOMAttribute_Release(attr);
    ok(ref1 == ref2, "got %d, expected %d\n", ref2, ref1);

    hr = IXMLDOMElement_get_xml(elem, &str);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok( lstrcmpW(str, _bstr_("<test attr=\"attrvalue1\"/>")) == 0,
        "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_("attrvalue2");
    hr = IXMLDOMAttribute_put_nodeValue(attr, v);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMElement_get_xml(elem, &str);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    todo_wine ok( lstrcmpW(str, _bstr_("<test attr=\"attrvalue2\"/>")) == 0,
        "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMAttribute_Release(attr);
    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_put_dataType(void)
{
    IXMLDOMCDATASection *cdata;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_createCDATASection(doc, _bstr_("test"), &cdata);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    hr = IXMLDOMCDATASection_put_dataType(cdata, _bstr_("number"));
    ok( hr == E_FAIL, "got 0x%08x\n", hr);
    hr = IXMLDOMCDATASection_put_dataType(cdata, _bstr_("string"));
    ok( hr == E_FAIL, "got 0x%08x\n", hr);
    IXMLDOMCDATASection_Release(cdata);

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_createNode(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    IXMLDOMNode *node;
    VARIANT v, var;
    BSTR prefix, str;
    HRESULT hr;
    ULONG ref;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    EXPECT_REF(doc, 1);

    /* reference count tests */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &elem);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    /* initial reference is 2 */
todo_wine {
    EXPECT_REF(elem, 2);
    ref = IXMLDOMElement_Release(elem);
    ok(ref == 1, "got %d\n", ref);
    /* it's released already, attempt to release now will crash it */
}

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &elem);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    todo_wine EXPECT_REF(elem, 2);
    IXMLDOMDocument_Release(doc);
    todo_wine EXPECT_REF(elem, 2);
    IXMLDOMElement_Release(elem);

    doc = create_document(&IID_IXMLDOMDocument);

    /* NODE_ELEMENT nodes */
    /* 1. specified namespace */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;

    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("ns1:test"), _bstr_("http://winehq.org"), &node);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    prefix = NULL;
    hr = IXMLDOMNode_get_prefix(node, &prefix);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok(lstrcmpW(prefix, _bstr_("ns1")) == 0, "wrong prefix\n");
    SysFreeString(prefix);
    IXMLDOMNode_Release(node);

    /* 2. default namespace */
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("test"), _bstr_("http://winehq.org/default"), &node);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    prefix = (void*)0xdeadbeef;
    hr = IXMLDOMNode_get_prefix(node, &prefix);
    ok( hr == S_FALSE, "got 0x%08x\n", hr);
    ok(prefix == 0, "expected empty prefix, got %p\n", prefix);
    /* check dump */
    hr = IXMLDOMNode_get_xml(node, &str);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok( lstrcmpW(str, _bstr_("<test xmlns=\"http://winehq.org/default\"/>")) == 0,
        "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    V_VT(&var) = VT_BSTR;
    hr = IXMLDOMElement_getAttribute(elem, _bstr_("xmlns"), &var);
    ok( hr == S_FALSE, "got 0x%08x\n", hr);
    ok( V_VT(&var) == VT_NULL, "got %d\n", V_VT(&var));

    str = NULL;
    hr = IXMLDOMElement_get_namespaceURI(elem, &str);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok( lstrcmpW(str, _bstr_("http://winehq.org/default")) == 0, "expected default namespace\n");
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMNode_Release(node);

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static const char get_prefix_doc[] =
    "<?xml version=\"1.0\" ?>"
    "<a xmlns:ns1=\"ns1 href\" />";

static void test_get_prefix(void)
{
    IXMLDOMDocumentFragment *fragment;
    IXMLDOMCDATASection *cdata;
    IXMLDOMElement *element;
    IXMLDOMComment *comment;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    /* nodes that can't support prefix */
    /* 1. document */
    str = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_prefix(doc, &str);
    ok( hr == S_FALSE, "got 0x%08x\n", hr);
    ok( str == 0, "got %p\n", str);

    hr = IXMLDOMDocument_get_prefix(doc, NULL);
    ok( hr == E_INVALIDARG, "got 0x%08x\n", hr);

    /* 2. cdata */
    hr = IXMLDOMDocument_createCDATASection(doc, NULL, &cdata);
    ok(hr == S_OK, "got %08x\n", hr );

    str = (void*)0xdeadbeef;
    hr = IXMLDOMCDATASection_get_prefix(cdata, &str);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok( str == 0, "got %p\n", str);

    hr = IXMLDOMCDATASection_get_prefix(cdata, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);
    IXMLDOMCDATASection_Release(cdata);

    /* 3. comment */
    hr = IXMLDOMDocument_createComment(doc, NULL, &comment);
    ok(hr == S_OK, "got %08x\n", hr );

    str = (void*)0xdeadbeef;
    hr = IXMLDOMComment_get_prefix(comment, &str);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok( str == 0, "got %p\n", str);

    hr = IXMLDOMComment_get_prefix(comment, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);
    IXMLDOMComment_Release(comment);

    /* 4. fragment */
    hr = IXMLDOMDocument_createDocumentFragment(doc, &fragment);
    ok(hr == S_OK, "got %08x\n", hr );

    str = (void*)0xdeadbeef;
    hr = IXMLDOMDocumentFragment_get_prefix(fragment, &str);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok( str == 0, "got %p\n", str);

    hr = IXMLDOMDocumentFragment_get_prefix(fragment, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);
    IXMLDOMDocumentFragment_Release(fragment);

    /* no prefix */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &element);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMElement_get_prefix(element, NULL);
    ok( hr == E_INVALIDARG, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_prefix(element, &str);
    ok( hr == S_FALSE, "got 0x%08x\n", hr);
    ok( str == 0, "got %p\n", str);

    IXMLDOMElement_Release(element);

    /* with prefix */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("a:elem"), &element);
    ok( hr == S_OK, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_prefix(element, &str);
    ok( hr == S_OK, "got 0x%08x\n", hr);
    ok( lstrcmpW(str, _bstr_("a")) == 0, "expected prefix \"a\"\n");
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_namespaceURI(element, &str);
    ok( hr == S_FALSE, "got 0x%08x\n", hr);
    ok( str == 0, "got %p\n", str);

    IXMLDOMElement_Release(element);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(get_prefix_doc), &b);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    EXPECT_HR(hr, S_OK);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_prefix(element, &str);
    EXPECT_HR(hr, S_FALSE);
    ok(str == NULL, "got %p\n", str);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_namespaceURI(element, &str);
    EXPECT_HR(hr, S_FALSE);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_selectSingleNode(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMNodeList *list;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_selectSingleNode(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_selectSingleNode(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("lc"), NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("lc"), NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("lc"), &node);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("lc"), &list);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IXMLDOMNodeList_Release(list);

    list = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_selectNodes(doc, NULL, &list);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(list == (void*)0xdeadbeef, "got %p\n", list);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("nonexistent"), &node);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(node == 0, "got %p\n", node);

    list = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("nonexistent"), &list);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    len = 1;
    hr = IXMLDOMNodeList_get_length(list, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(len == 0, "got %d\n", len);
    IXMLDOMNodeList_Release(list);

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_events(void)
{
    IConnectionPointContainer *conn;
    IConnectionPoint *point;
    IXMLDOMDocument *doc;
    HRESULT hr;
    VARIANT v;
    IDispatch *event;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IConnectionPointContainer, (void**)&conn);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IConnectionPointContainer_FindConnectionPoint(conn, &IID_IDispatch, &point);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IConnectionPoint_Release(point);
    hr = IConnectionPointContainer_FindConnectionPoint(conn, &IID_IPropertyNotifySink, &point);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IConnectionPoint_Release(point);
    hr = IConnectionPointContainer_FindConnectionPoint(conn, &DIID_XMLDOMDocumentEvents, &point);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IConnectionPoint_Release(point);

    IConnectionPointContainer_Release(conn);

    /* ready state callback */
    VariantInit(&v);
    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == DISP_E_TYPEMISMATCH, "got 0x%08x\n", hr);

    event = create_dispevent();
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)event;

    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(event, 2);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = event;

    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(event, 2);

    /* VT_NULL doesn't reset event handler */
    V_VT(&v) = VT_NULL;
    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == DISP_E_TYPEMISMATCH, "got 0x%08x\n", hr);
    EXPECT_REF(event, 2);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;

    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(event, 1);

    V_VT(&v) = VT_UNKNOWN;
    V_DISPATCH(&v) = NULL;
    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDispatch_Release(event);

    IXMLDOMDocument_Release(doc);
}

static void test_createProcessingInstruction(void)
{
    static const WCHAR bodyW[] = {'t','e','s','t',0};
    IXMLDOMProcessingInstruction *pi;
    IXMLDOMDocument *doc;
    WCHAR buff[10];
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    /* test for BSTR handling, pass broken BSTR */
    memcpy(&buff[2], bodyW, sizeof(bodyW));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("test"), &buff[2], &pi);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IXMLDOMProcessingInstruction_Release(pi);
    IXMLDOMDocument_Release(doc);
}

static void test_put_nodeTypedValue(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    VARIANT type;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Element"), &elem);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    V_VT(&type) = VT_EMPTY;
    hr = IXMLDOMElement_get_dataType(elem, &type);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(V_VT(&type) == VT_NULL, "got %d, expected VT_NULL\n", V_VT(&type));

    /* set typed value for untyped node */
    V_VT(&type) = VT_I1;
    V_I1(&type) = 1;
    hr = IXMLDOMElement_put_nodeTypedValue(elem, type);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    V_VT(&type) = VT_EMPTY;
    hr = IXMLDOMElement_get_dataType(elem, &type);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(V_VT(&type) == VT_NULL, "got %d, expected VT_NULL\n", V_VT(&type));

    /* no type info stored */
    V_VT(&type) = VT_EMPTY;
    hr = IXMLDOMElement_get_nodeTypedValue(elem, &type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&type) == VT_BSTR, "got %d, expected VT_BSTR\n", V_VT(&type));
    ok(memcmp(V_BSTR(&type), _bstr_("1"), 2*sizeof(WCHAR)) == 0,
       "got %s, expected \"1\"\n", wine_dbgstr_w(V_BSTR(&type)));
    VariantClear(&type);

    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_get_xml(void)
{
    static const char xmlA[] = "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n<a>test</a>\r\n";
    static const char fooA[] = "<foo/>";
    IXMLDOMProcessingInstruction *pi;
    IXMLDOMNode *first;
    IXMLDOMElement *elem = NULL;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    VARIANT v;
    BSTR xml;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_("<a>test</a>"), &b );
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"),
                             _bstr_("version=\"1.0\" encoding=\"UTF-16\""), &pi);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_get_firstChild(doc, &first);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    V_UNKNOWN(&v) = (IUnknown*)first;
    V_VT(&v) = VT_UNKNOWN;

    hr = IXMLDOMDocument_insertBefore(doc, (IXMLDOMNode*)pi, v, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IXMLDOMProcessingInstruction_Release(pi);
    IXMLDOMNode_Release(first);

    hr = IXMLDOMDocument_get_xml(doc, &xml);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ok(memcmp(xml, _bstr_(xmlA), sizeof(xmlA)*sizeof(WCHAR)) == 0,
        "got %s, expected %s\n", wine_dbgstr_w(xml), xmlA);
    SysFreeString(xml);

    IXMLDOMDocument_Release(doc);

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("foo"), &elem);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_putref_documentElement(doc, elem);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_get_xml(doc, &xml);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ok(memcmp(xml, _bstr_(fooA), (sizeof(fooA)-1)*sizeof(WCHAR)) == 0,
        "got %s, expected %s\n", wine_dbgstr_w(xml), fooA);
    SysFreeString(xml);

    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_xsltemplate(void)
{
    IXSLTemplate *template;
    IXSLProcessor *processor;
    IXMLDOMDocument *doc, *doc2;
    IStream *stream;
    VARIANT_BOOL b;
    HRESULT hr;
    ULONG ref1, ref2;
    VARIANT v;

    template = create_xsltemplate(&IID_IXSLTemplate);
    if (!template) return;

    /* works as reset */
    hr = IXSLTemplate_putref_stylesheet(template, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    doc = create_document(&IID_IXMLDOMDocument);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_("<a>test</a>"), &b );
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    /* putref with non-xsl document */
    hr = IXSLTemplate_putref_stylesheet(template, (IXMLDOMNode*)doc);
    todo_wine ok(hr == E_FAIL, "got 0x%08x\n", hr);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_(szTransformSSXML), &b );
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    /* not a freethreaded document */
    hr = IXSLTemplate_putref_stylesheet(template, (IXMLDOMNode*)doc);
    todo_wine ok(hr == E_FAIL, "got 0x%08x\n", hr);

    IXMLDOMDocument_Release(doc);

    hr = CoCreateInstance(&CLSID_FreeThreadedDOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc);
    if (hr != S_OK)
    {
        win_skip("failed to create free threaded document instance: 0x%08x\n", hr);
        IXSLTemplate_Release(template);
        return;
    }

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_(szTransformSSXML), &b );
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    /* freethreaded document */
    ref1 = IXMLDOMDocument_AddRef(doc);
    IXMLDOMDocument_Release(doc);
    hr = IXSLTemplate_putref_stylesheet(template, (IXMLDOMNode*)doc);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ref2 = IXMLDOMDocument_AddRef(doc);
    IXMLDOMDocument_Release(doc);
    ok(ref2 > ref1, "got %d\n", ref2);

    /* processor */
    hr = IXSLTemplate_createProcessor(template, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    EXPECT_REF(template, 1);
    hr = IXSLTemplate_createProcessor(template, &processor);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(template, 2);

    /* input no set yet */
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = NULL;
    hr = IXSLProcessor_get_input(processor, &v);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&v) == VT_EMPTY, "got %d\n", V_VT(&v));
}

    hr = IXSLProcessor_get_output(processor, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    /* reset before it was set */
    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CreateStreamOnHGlobal(NULL, TRUE, &stream);
    EXPECT_REF(stream, 1);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)stream;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* it seems processor grabs 2 references */
    todo_wine EXPECT_REF(stream, 3);

    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_get_output(processor, &v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "got type %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == (IUnknown*)stream, "got %p\n", V_UNKNOWN(&v));

    todo_wine EXPECT_REF(stream, 4);
    VariantClear(&v);

    hr = IXSLProcessor_transform(processor, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    /* reset and check stream refcount */
    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(stream, 1);

    IStream_Release(stream);

    /* no output interface set, check output */
    doc2 = create_document(&IID_IXMLDOMDocument);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc2, _bstr_("<a>test</a>"), &b );
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)doc2;
    hr = IXSLProcessor_put_input(processor, v);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXSLProcessor_transform(processor, &b);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_get_output(processor, &v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&v) == VT_BSTR, "got type %d\n", V_VT(&v));
    /* we currently output one '\n' instead of empty string */
    todo_wine ok(lstrcmpW(V_BSTR(&v), _bstr_("")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    IXMLDOMDocument_Release(doc2);
    VariantClear(&v);

    IXSLProcessor_Release(processor);

    /* drop reference */
    hr = IXSLTemplate_putref_stylesheet(template, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ref2 = IXMLDOMDocument_AddRef(doc);
    IXMLDOMDocument_Release(doc);
    ok(ref2 == ref1, "got %d\n", ref2);

    IXMLDOMDocument_Release(doc);
    IXSLTemplate_Release(template);
    free_bstrs();
}

static void test_insertBefore(void)
{
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMAttribute *attr;
    IXMLDOMElement *elem1, *elem2, *elem3, *elem4, *elem5;
    IXMLDOMNode *node, *newnode;
    HRESULT hr;
    VARIANT v;
    BSTR p;

    doc = create_document(&IID_IXMLDOMDocument);

    /* insertBefore behaviour for attribute node */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ATTRIBUTE;

    attr = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("attr"), NULL, (IXMLDOMNode**)&attr);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(attr != NULL, "got %p\n", attr);

    /* attribute to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ATTRIBUTE;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("attr2"), NULL, &newnode);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(node == NULL, "got %p\n", node);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)attr;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    todo_wine ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);

    /* cdata to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_CDATA_SECTION;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("cdata"), NULL, &newnode);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);

    /* comment to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_COMMENT;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("cdata"), NULL, &newnode);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);

    /* element to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("cdata"), NULL, &newnode);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);

    /* pi to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_PROCESSING_INSTRUCTION;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("cdata"), NULL, &newnode);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);
    IXMLDOMAttribute_Release(attr);

    /* insertBefore for elements */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &elem1);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem2"), &elem2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem3"), &elem3);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem3"), &elem3);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem4"), &elem4);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_NO_CHILDREN(elem1);
    EXPECT_NO_CHILDREN(elem2);
    EXPECT_NO_CHILDREN(elem3);

    todo_wine EXPECT_REF(elem2, 2);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem4, v, &node);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(node == (void*)elem4, "got %p\n", node);

    EXPECT_CHILDREN(elem1);
    hr = IXMLDOMElement_removeChild(elem1, (IXMLDOMNode*)elem4, NULL);
    EXPECT_HR(hr, S_OK);
    IXMLDOMElement_Release(elem4);

    EXPECT_NO_CHILDREN(elem1);

    V_VT(&v) = VT_NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem2, v, &node);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(node == (void*)elem2, "got %p\n", node);

    EXPECT_CHILDREN(elem1);
    todo_wine EXPECT_REF(elem2, 3);
    IXMLDOMNode_Release(node);

    /* again for already linked node */
    V_VT(&v) = VT_NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem2, v, &node);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(node == (void*)elem2, "got %p\n", node);

    EXPECT_CHILDREN(elem1);

    /* increments each time */
    todo_wine EXPECT_REF(elem2, 3);
    IXMLDOMNode_Release(node);

    /* try to add to another element */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMElement_insertBefore(elem3, (IXMLDOMNode*)elem2, v, &node);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(node == (void*)elem2, "got %p\n", node);

    EXPECT_CHILDREN(elem3);
    EXPECT_NO_CHILDREN(elem1);

    IXMLDOMNode_Release(node);

    /* cross document case - try to add as child to a node created with other doc */
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc2, _bstr_("elem4"), &elem4);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    todo_wine EXPECT_REF(elem4, 2);

    /* same name, another instance */
    hr = IXMLDOMDocument_createElement(doc2, _bstr_("elem4"), &elem5);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    todo_wine EXPECT_REF(elem5, 2);

    todo_wine EXPECT_REF(elem3, 2);
    V_VT(&v) = VT_NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem3, (IXMLDOMNode*)elem4, v, &node);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(node == (void*)elem4, "got %p\n", node);
    todo_wine EXPECT_REF(elem4, 3);
    todo_wine EXPECT_REF(elem3, 2);
    IXMLDOMNode_Release(node);

    V_VT(&v) = VT_NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem3, (IXMLDOMNode*)elem5, v, &node);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(node == (void*)elem5, "got %p\n", node);
    todo_wine EXPECT_REF(elem4, 2);
    todo_wine EXPECT_REF(elem5, 3);
    IXMLDOMNode_Release(node);

    IXMLDOMDocument_Release(doc2);

    IXMLDOMElement_Release(elem1);
    IXMLDOMElement_Release(elem2);
    IXMLDOMElement_Release(elem3);
    IXMLDOMElement_Release(elem4);
    IXMLDOMElement_Release(elem5);

    /* elements with same default namespace */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    elem1 = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("elem1"), _bstr_("http://winehq.org/default"), (IXMLDOMNode**)&elem1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(elem1 != NULL, "got %p\n", elem1);

    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    elem2 = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("elem2"), _bstr_("http://winehq.org/default"), (IXMLDOMNode**)&elem2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(elem2 != NULL, "got %p\n", elem2);

    /* check contents so far */
    p = NULL;
    hr = IXMLDOMElement_get_xml(elem1, &p);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(p, _bstr_("<elem1 xmlns=\"http://winehq.org/default\"/>")), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    p = NULL;
    hr = IXMLDOMElement_get_xml(elem2, &p);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(p, _bstr_("<elem2 xmlns=\"http://winehq.org/default\"/>")), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    V_VT(&v) = VT_NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem2, v, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* get_xml depends on context, for top node it omits child namespace attribute,
       but at child level it's still returned */
    p = NULL;
    hr = IXMLDOMElement_get_xml(elem1, &p);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    todo_wine ok(!lstrcmpW(p, _bstr_("<elem1 xmlns=\"http://winehq.org/default\"><elem2/></elem1>")),
        "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    p = NULL;
    hr = IXMLDOMElement_get_xml(elem2, &p);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(p, _bstr_("<elem2 xmlns=\"http://winehq.org/default\"/>")), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    IXMLDOMElement_Release(elem1);
    IXMLDOMElement_Release(elem2);

    /* child without default namespace added to node with default namespace */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    elem1 = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("elem1"), _bstr_("http://winehq.org/default"), (IXMLDOMNode**)&elem1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(elem1 != NULL, "got %p\n", elem1);

    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    elem2 = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("elem2"), NULL, (IXMLDOMNode**)&elem2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(elem2 != NULL, "got %p\n", elem2);

    EXPECT_REF(elem2, 1);
    V_VT(&v) = VT_NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem2, v, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(elem2, 1);

    p = NULL;
    hr = IXMLDOMElement_get_xml(elem2, &p);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(p, _bstr_("<elem2/>")), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    hr = IXMLDOMElement_removeChild(elem1, (IXMLDOMNode*)elem2, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    p = NULL;
    hr = IXMLDOMElement_get_xml(elem2, &p);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(p, _bstr_("<elem2/>")), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    IXMLDOMElement_Release(elem1);
    IXMLDOMElement_Release(elem2);
    IXMLDOMDocument_Release(doc);
}

static void test_appendChild(void)
{
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMElement *elem, *elem2;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &elem);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMDocument_createElement(doc2, _bstr_("elem2"), &elem2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(doc, 1);
    todo_wine EXPECT_REF(elem, 2);
    EXPECT_REF(doc2, 1);
    todo_wine EXPECT_REF(elem2, 2);
    EXPECT_NO_CHILDREN(doc);
    EXPECT_NO_CHILDREN(doc2);

    /* append from another document */
    hr = IXMLDOMDocument_appendChild(doc2, (IXMLDOMNode*)elem, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(doc, 1);
    todo_wine EXPECT_REF(elem, 2);
    EXPECT_REF(doc2, 1);
    todo_wine EXPECT_REF(elem2, 2);
    EXPECT_NO_CHILDREN(doc);
    EXPECT_CHILDREN(doc2);

    IXMLDOMElement_Release(elem);
    IXMLDOMElement_Release(elem2);
    IXMLDOMDocument_Release(doc);
    IXMLDOMDocument_Release(doc2);
}

static void test_get_doctype(void)
{
    IXMLDOMDocumentType *doctype;
    IXMLDOMDocument *doc;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_doctype(doc, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    doctype = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_doctype(doc, &doctype);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(doctype == NULL, "got %p\n", doctype);

    IXMLDOMDocument_Release(doc);
}

static void test_get_tagName(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem, *elem2;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("element"), &elem);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXMLDOMElement_get_tagName(elem, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    str = NULL;
    hr = IXMLDOMElement_get_tagName(elem, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(str, _bstr_("element")), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("s:element"), &elem2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = NULL;
    hr = IXMLDOMElement_get_tagName(elem2, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(str, _bstr_("s:element")), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMDocument_Release(elem);
    IXMLDOMDocument_Release(elem2);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

typedef struct _get_datatype_t {
    DOMNodeType type;
    const char *name;
    VARTYPE vt;
    HRESULT hr;
} get_datatype_t;

static const get_datatype_t get_datatype[] = {
    { NODE_ELEMENT,                "element",   VT_NULL, S_FALSE },
    { NODE_ATTRIBUTE,              "attr",      VT_NULL, S_FALSE },
    { NODE_TEXT,                   "text",      VT_NULL, S_FALSE },
    { NODE_CDATA_SECTION ,         "cdata",     VT_NULL, S_FALSE },
    { NODE_ENTITY_REFERENCE,       "entityref", VT_NULL, S_FALSE },
    { NODE_PROCESSING_INSTRUCTION, "pi",        VT_NULL, S_FALSE },
    { NODE_COMMENT,                "comment",   VT_NULL, S_FALSE },
    { NODE_DOCUMENT_FRAGMENT,      "docfrag",   VT_NULL, S_FALSE },
    { 0 }
};

static void test_get_dataType(void)
{
    IXMLDOMDocument *doc;
    const get_datatype_t *entry = get_datatype;

    doc = create_document(&IID_IXMLDOMDocument);

    while (entry->type)
    {
        IXMLDOMNode *node = NULL;
        VARIANT var, type;
        HRESULT hr;

        V_VT(&var) = VT_I4;
        V_I4(&var) = entry->type;
        hr = IXMLDOMDocument_createNode(doc, var, _bstr_(entry->name), NULL, &node);
        ok(hr == S_OK, "failed to create node, type %d\n", entry->type);

        hr = IXMLDOMNode_get_dataType(node, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

        VariantInit(&type);
        hr = IXMLDOMNode_get_dataType(node, &type);
        ok(hr == entry->hr, "got 0x%08x, expected 0x%08x. node type %d\n",
            hr, entry->hr, entry->type);
        ok(V_VT(&type) == entry->vt, "got %d, expected %d. node type %d\n",
            V_VT(&type), entry->vt, entry->type);
        VariantClear(&type);

        IXMLDOMNode_Release(node);

        entry++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

typedef struct _get_node_typestring_t {
    DOMNodeType type;
    const char *string;
} get_node_typestring_t;

static const get_node_typestring_t get_node_typestring[] = {
    { NODE_ELEMENT,                "element"               },
    { NODE_ATTRIBUTE,              "attribute"             },
    { NODE_TEXT,                   "text"                  },
    { NODE_CDATA_SECTION ,         "cdatasection"          },
    { NODE_ENTITY_REFERENCE,       "entityreference"       },
    { NODE_PROCESSING_INSTRUCTION, "processinginstruction" },
    { NODE_COMMENT,                "comment"               },
    { NODE_DOCUMENT_FRAGMENT,      "documentfragment"      },
    { 0 }
};

static void test_get_nodeTypeString(void)
{
    const get_node_typestring_t *entry = get_node_typestring;
    IXMLDOMDocument *doc;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_nodeTypeString(doc, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(str, _bstr_("document")), "got string %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    while (entry->type)
    {
        IXMLDOMNode *node = NULL;
        VARIANT var;

        V_VT(&var) = VT_I4;
        V_I4(&var) = entry->type;
        hr = IXMLDOMDocument_createNode(doc, var, _bstr_("node"), NULL, &node);
        ok(hr == S_OK, "failed to create node, type %d\n", entry->type);

        hr = IXMLDOMNode_get_nodeTypeString(node, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

        hr = IXMLDOMNode_get_nodeTypeString(node, &str);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(!lstrcmpW(str, _bstr_(entry->string)), "got string %s, expected %s. node type %d\n",
            wine_dbgstr_w(str), entry->string, entry->type);
        SysFreeString(str);
        IXMLDOMNode_Release(node);

        entry++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

typedef struct _get_attributes_t {
    DOMNodeType type;
    HRESULT hr;
} get_attributes_t;

static const get_attributes_t get_attributes[] = {
    { NODE_ATTRIBUTE,              S_FALSE },
    { NODE_TEXT,                   S_FALSE },
    { NODE_CDATA_SECTION ,         S_FALSE },
    { NODE_ENTITY_REFERENCE,       S_FALSE },
    { NODE_PROCESSING_INSTRUCTION, S_FALSE },
    { NODE_COMMENT,                S_FALSE },
    { NODE_DOCUMENT_FRAGMENT,      S_FALSE },
    { 0 }
};

static void test_get_attributes(void)
{
    const get_attributes_t *entry = get_attributes;
    IXMLDOMNamedNodeMap *map;
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *node2;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str;
    LONG length;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(complete4A), &b);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IXMLDOMDocument_get_attributes(doc, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    map = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_attributes(doc, &map);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok(map == NULL, "got %p\n", map);

    /* first child is <?xml ?> */
    hr = IXMLDOMDocument_get_firstChild(doc, &node);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IXMLDOMNode_get_attributes(node, &map);
    ok(hr == S_OK, "got %08x\n", hr);

    length = -1;
    hr = IXMLDOMNamedNodeMap_get_length(map, &length);
    EXPECT_HR(hr, S_OK);
    todo_wine ok(length == 1, "got %d\n", length);

    if (hr == S_OK && length == 1)
    {
        IXMLDOMAttribute *attr;
        DOMNodeType type;
        VARIANT v;

        node2 = NULL;
        hr = IXMLDOMNamedNodeMap_get_item(map, 0, &node2);
        EXPECT_HR(hr, S_OK);
        ok(node != NULL, "got %p\n", node2);

        hr = IXMLDOMNode_get_nodeName(node2, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW(str, _bstr_("version")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        length = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &length);
        EXPECT_HR(hr, S_OK);
        ok(length == 1, "got %d\n", length);

        type = -1;
        hr = IXMLDOMNode_get_nodeType(node2, &type);
        EXPECT_HR(hr, S_OK);
        ok(type == NODE_ATTRIBUTE, "got %d\n", type);

        hr = IXMLDOMNode_get_xml(node, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW(str, _bstr_("<?xml version=\"1.0\"?>")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_get_text(node, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW(str, _bstr_("version=\"1.0\"")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNamedNodeMap_removeNamedItem(map, _bstr_("version"), NULL);
        EXPECT_HR(hr, S_OK);

        length = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &length);
        EXPECT_HR(hr, S_OK);
        ok(length == 0, "got %d\n", length);

        hr = IXMLDOMNode_get_xml(node, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW(str, _bstr_("<?xml version=\"1.0\"?>")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_get_text(node, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW(str, _bstr_("")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMNamedNodeMap_Release(map);

        hr = IXMLDOMNode_get_attributes(node, &map);
        ok(hr == S_OK, "got %08x\n", hr);

        length = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &length);
        EXPECT_HR(hr, S_OK);
        ok(length == 0, "got %d\n", length);

        hr = IXMLDOMDocument_createAttribute(doc, _bstr_("encoding"), &attr);
        EXPECT_HR(hr, S_OK);

        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = _bstr_("UTF-8");
        hr = IXMLDOMAttribute_put_nodeValue(attr, v);
        EXPECT_HR(hr, S_OK);

        EXPECT_REF(attr, 2);
        hr = IXMLDOMNamedNodeMap_setNamedItem(map, (IXMLDOMNode*)attr, NULL);
        EXPECT_HR(hr, S_OK);
        EXPECT_REF(attr, 2);

        hr = IXMLDOMNode_get_attributes(node, &map);
        ok(hr == S_OK, "got %08x\n", hr);

        length = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &length);
        EXPECT_HR(hr, S_OK);
        ok(length == 1, "got %d\n", length);

        hr = IXMLDOMNode_get_xml(node, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW(str, _bstr_("<?xml version=\"1.0\"?>")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_get_text(node, &str);
        EXPECT_HR(hr, S_OK);
        ok(!lstrcmpW(str, _bstr_("encoding=\"UTF-8\"")), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMNamedNodeMap_Release(map);
        IXMLDOMNode_Release(node2);
    }

    IXMLDOMNode_Release(node);

    /* last child is element */
    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument_get_lastChild(doc, &node);
    ok(hr == S_OK, "got %08x\n", hr);
    EXPECT_REF(doc, 1);

    EXPECT_REF(node, 1);
    hr = IXMLDOMNode_get_attributes(node, &map);
    ok(hr == S_OK, "got %08x\n", hr);
    EXPECT_REF(node, 1);
    EXPECT_REF(doc, 1);

    EXPECT_REF(map, 1);
    hr = IXMLDOMNamedNodeMap_get_item(map, 0, &node2);
    ok(hr == S_OK, "got %08x\n", hr);
    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);
    EXPECT_REF(map, 1);
    EXPECT_REF(doc, 1);
    IXMLDOMNode_Release(node2);

    /* release node before map release, map still works */
    IXMLDOMNode_Release(node);

    length = 0;
    hr = IXMLDOMNamedNodeMap_get_length(map, &length);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(length == 1, "got %d\n", length);

    node2 = NULL;
    hr = IXMLDOMNamedNodeMap_get_item(map, 0, &node2);
    ok(hr == S_OK, "got %08x\n", hr);
    EXPECT_REF(node2, 1);
    IXMLDOMNode_Release(node2);

    IXMLDOMNamedNodeMap_Release(map);

    while (entry->type)
    {
        VARIANT var;

        node = NULL;

        V_VT(&var) = VT_I4;
        V_I4(&var) = entry->type;
        hr = IXMLDOMDocument_createNode(doc, var, _bstr_("node"), NULL, &node);
        ok(hr == S_OK, "failed to create node, type %d\n", entry->type);

        hr = IXMLDOMNode_get_attributes(node, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

        map = (void*)0xdeadbeef;
        hr = IXMLDOMNode_get_attributes(node, &map);
        ok(hr == entry->hr, "got 0x%08x, expected 0x%08x. node type %d\n",
            hr, entry->hr, entry->type);
        ok(map == NULL, "got %p\n", map);

        IXMLDOMNode_Release(node);

        entry++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_selection(void)
{
    IXMLDOMSelection *selection, *selection2;
    IEnumVARIANT *enum1, *enum2, *enum3;
    IXMLDOMNodeList *list;
    IXMLDOMDocument *doc;
    IDispatchEx *dispex;
    IXMLDOMNode *node;
    IDispatch *disp;
    VARIANT_BOOL b;
    HRESULT hr;
    DISPID did;
    VARIANT v;
    BSTR name;
    ULONG ret;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("root"), &list);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNodeList_QueryInterface(list, &IID_IXMLDOMSelection, (void**)&selection);
    EXPECT_HR(hr, S_OK);
    IXMLDOMSelection_Release(selection);

    /* collection disp id */
    hr = IXMLDOMSelection_QueryInterface(selection, &IID_IDispatchEx, (void**)&dispex);
    EXPECT_HR(hr, S_OK);
    did = 0;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("0"), 0, &did);
    EXPECT_HR(hr, S_OK);
    ok(did == DISPID_DOM_COLLECTION_BASE, "got %d\n", did);
    len = 0;
    hr = IXMLDOMSelection_get_length(selection, &len);
    EXPECT_HR(hr, S_OK);
    ok(len == 1, "got %d\n", len);
    hr = IDispatchEx_GetDispID(dispex, _bstr_("10"), 0, &did);
    EXPECT_HR(hr, S_OK);
    ok(did == DISPID_DOM_COLLECTION_BASE+10, "got %d\n", did);
    IDispatchEx_Release(dispex);

    /* IEnumVARIANT tests */
    enum1 = NULL;
    hr = IXMLDOMSelection_QueryInterface(selection, &IID_IEnumVARIANT, (void**)&enum1);
    EXPECT_HR(hr, S_OK);
    ok(enum1 != NULL, "got %p\n", enum1);
    EXPECT_REF(enum1, 2);

    enum3 = NULL;
    hr = IXMLDOMSelection_QueryInterface(selection, &IID_IEnumVARIANT, (void**)&enum3);
    EXPECT_HR(hr, S_OK);
    ok(enum3 != NULL, "got %p\n", enum3);
    ok(enum1 == enum3, "got %p and %p\n", enum1, enum3);
    EXPECT_REF(enum1, 3);
    IEnumVARIANT_Release(enum3);

    EXPECT_REF(selection, 1);
    EXPECT_REF(enum1, 2);

    enum2 = NULL;
    hr = IXMLDOMSelection_get__newEnum(selection, (IUnknown**)&enum2);
    EXPECT_HR(hr, S_OK);
    ok(enum2 != NULL, "got %p\n", enum2);

    EXPECT_REF(selection, 2);
    EXPECT_REF(enum1, 2);
    EXPECT_REF(enum2, 1);

    ok(enum1 != enum2, "got %p, %p\n", enum1, enum2);

    selection2 = NULL;
    hr = IEnumVARIANT_QueryInterface(enum1, &IID_IXMLDOMSelection, (void**)&selection2);
    EXPECT_HR(hr, S_OK);
    ok(selection2 == selection, "got %p and %p\n", selection, selection2);
    EXPECT_REF(selection, 3);
    EXPECT_REF(enum1, 2);

    IXMLDOMSelection_Release(selection2);

    hr = IEnumVARIANT_QueryInterface(enum1, &IID_IDispatch, (void**)&disp);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(selection, 3);
    IDispatch_Release(disp);

    hr = IEnumVARIANT_QueryInterface(enum1, &IID_IEnumVARIANT, (void**)&enum3);
    EXPECT_HR(hr, S_OK);
    ok(enum3 == enum1, "got %p and %p\n", enum3, enum1);
    EXPECT_REF(selection, 2);
    EXPECT_REF(enum1, 3);

    IEnumVARIANT_Release(enum1);
    IEnumVARIANT_Release(enum2);

    enum1 = NULL;
    hr = IXMLDOMSelection_get__newEnum(selection, (IUnknown**)&enum1);
    EXPECT_HR(hr, S_OK);
    ok(enum1 != NULL, "got %p\n", enum1);
    EXPECT_REF(enum1, 1);
    EXPECT_REF(selection, 2);

    enum2 = NULL;
    hr = IXMLDOMSelection_get__newEnum(selection, (IUnknown**)&enum2);
    EXPECT_HR(hr, S_OK);
    ok(enum2 != NULL, "got %p\n", enum2);
    EXPECT_REF(enum2, 1);
    EXPECT_REF(selection, 3);

    ok(enum1 != enum2, "got %p, %p\n", enum1, enum2);

    IEnumVARIANT_AddRef(enum1);
    EXPECT_REF(selection, 3);
    EXPECT_REF(enum1, 2);
    EXPECT_REF(enum2, 1);
    IEnumVARIANT_Release(enum1);

    IEnumVARIANT_Release(enum1);
    IEnumVARIANT_Release(enum2);

    EXPECT_REF(selection, 1);

    IXMLDOMNodeList_Release(list);

    hr = IXMLDOMDocument_get_childNodes(doc, &list);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNodeList_QueryInterface(list, &IID_IXMLDOMSelection, (void**)&selection);
    EXPECT_HR(hr, E_NOINTERFACE);

    IXMLDOMNodeList_Release(list);

    /* test if IEnumVARIANT touches selection context */
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(xpath_simple_list), &b);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("root/*"), &list);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMNodeList_QueryInterface(list, &IID_IXMLDOMSelection, (void**)&selection);
    EXPECT_HR(hr, S_OK);

    len = 0;
    hr = IXMLDOMSelection_get_length(selection, &len);
    EXPECT_HR(hr, S_OK);
    ok(len == 4, "got %d\n", len);

    enum1 = NULL;
    hr = IXMLDOMSelection_get__newEnum(selection, (IUnknown**)&enum1);
    EXPECT_HR(hr, S_OK);

    /* no-op if zero count */
    V_VT(&v) = VT_I2;
    hr = IEnumVARIANT_Next(enum1, 0, &v, NULL);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&v) == VT_I2, "got var type %d\n", V_VT(&v));

    /* positive count, null array pointer */
    hr = IEnumVARIANT_Next(enum1, 1, NULL, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    ret = 1;
    hr = IEnumVARIANT_Next(enum1, 1, NULL, &ret);
    EXPECT_HR(hr, E_INVALIDARG);
    ok(ret == 0, "got %d\n", ret);

    V_VT(&v) = VT_I2;
    hr = IEnumVARIANT_Next(enum1, 1, &v, NULL);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&v) == VT_DISPATCH, "got var type %d\n", V_VT(&v));

    hr = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IXMLDOMNode, (void**)&node);
    EXPECT_HR(hr, S_OK);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(name, _bstr_("a")), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);
    VariantClear(&v);

    /* list cursor is updated */
    hr = IXMLDOMSelection_nextNode(selection, &node);
    EXPECT_HR(hr, S_OK);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(name, _bstr_("c")), "got node name %s\n", wine_dbgstr_w(name));
    IXMLDOMNode_Release(node);

    V_VT(&v) = VT_I2;
    hr = IEnumVARIANT_Next(enum1, 1, &v, NULL);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&v) == VT_DISPATCH, "got var type %d\n", V_VT(&v));
    hr = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IXMLDOMNode, (void**)&node);
    EXPECT_HR(hr, S_OK);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(name, _bstr_("b")), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);
    VariantClear(&v);

    hr = IXMLDOMSelection_nextNode(selection, &node);
    EXPECT_HR(hr, S_OK);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(name, _bstr_("d")), "got node name %s\n", wine_dbgstr_w(name));
    IXMLDOMNode_Release(node);

    IXMLDOMSelection_Release(selection);
    IXMLDOMNodeList_Release(list);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_load(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMNodeList *list;
    VARIANT_BOOL b;
    HANDLE hfile;
    VARIANT src;
    HRESULT hr;
    BOOL ret;
    BSTR path, bstr1, bstr2;
    DWORD written;
    void* ptr;

    /* prepare a file */
    hfile = CreateFileA("test.xml", GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(hfile != INVALID_HANDLE_VALUE, "failed to create test file\n");
    if(hfile == INVALID_HANDLE_VALUE) return;

    ret = WriteFile(hfile, szNonUnicodeXML, sizeof(szNonUnicodeXML)-1, &written, NULL);
    ok(ret, "WriteFile failed\n");

    CloseHandle(hfile);

    doc = create_document(&IID_IXMLDOMDocument);

    path = _bstr_("test.xml");

    /* load from path: VT_BSTR */
    V_VT(&src) = VT_BSTR;
    V_BSTR(&src) = path;
    hr = IXMLDOMDocument_load(doc, src, &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    /* load from a path: VT_BSTR|VT_BYREF */
    V_VT(&src) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&src) = &path;
    hr = IXMLDOMDocument_load(doc, src, &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    /* load from a path: VT_BSTR|VT_BYREF, null ptr */
    V_VT(&src) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&src) = NULL;
    hr = IXMLDOMDocument_load(doc, src, &b);
    EXPECT_HR(hr, E_INVALIDARG);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    IXMLDOMDocument_Release(doc);

    DeleteFileA("test.xml");

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szExampleXML), &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("//*"), &list);
    EXPECT_HR(hr, S_OK);
    bstr1 = _bstr_(list_to_string(list));

    hr = IXMLDOMNodeList_reset(list);
    EXPECT_HR(hr, S_OK);

    IXMLDOMDocument_Release(doc);

    doc = create_document(&IID_IXMLDOMDocument);

    VariantInit(&src);
    V_ARRAY(&src) = SafeArrayCreateVector(VT_UI1, 0, lstrlenA(szExampleXML));
    V_VT(&src) = VT_ARRAY|VT_UI1;
    ok(V_ARRAY(&src) != NULL, "SafeArrayCreateVector() returned NULL\n");
    ptr = NULL;
    hr = SafeArrayAccessData(V_ARRAY(&src), &ptr);
    EXPECT_HR(hr, S_OK);
    ok(ptr != NULL, "SafeArrayAccessData() returned NULL\n");

    memcpy(ptr, szExampleXML, lstrlenA(szExampleXML));
    hr = SafeArrayUnlock(V_ARRAY(&src));
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_load(doc, src, &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("//*"), &list);
    EXPECT_HR(hr, S_OK);
    bstr2 = _bstr_(list_to_string(list));

    hr = IXMLDOMNodeList_reset(list);
    EXPECT_HR(hr, S_OK);

    ok(lstrcmpW(bstr1, bstr2) == 0, "strings not equal: %s : %s\n",
       wine_dbgstr_w(bstr1), wine_dbgstr_w(bstr2));

    IXMLDOMDocument_Release(doc);
    IXMLDOMNodeList_Release(list);
    VariantClear(&src);

    /* UTF-16 isn't accepted */
    doc = create_document(&IID_IXMLDOMDocument);

    V_ARRAY(&src) = SafeArrayCreateVector(VT_UI1, 0, lstrlenW(szComplete1) * sizeof(WCHAR));
    V_VT(&src) = VT_ARRAY|VT_UI1;
    ok(V_ARRAY(&src) != NULL, "SafeArrayCreateVector() returned NULL\n");
    ptr = NULL;
    hr = SafeArrayAccessData(V_ARRAY(&src), &ptr);
    EXPECT_HR(hr, S_OK);
    ok(ptr != NULL, "SafeArrayAccessData() returned NULL\n");

    memcpy(ptr, szComplete1, lstrlenW(szComplete1) * sizeof(WCHAR));
    hr = SafeArrayUnlock(V_ARRAY(&src));
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_load(doc, src, &b);
    todo_wine EXPECT_HR(hr, S_FALSE);
    todo_wine ok(b == VARIANT_FALSE, "got %d\n", b);

    VariantClear(&src);

    /* it doesn't like it as a VT_ARRAY|VT_UI2 either */
    V_ARRAY(&src) = SafeArrayCreateVector(VT_UI2, 0, lstrlenW(szComplete1));
    V_VT(&src) = VT_ARRAY|VT_UI2;
    ok(V_ARRAY(&src) != NULL, "SafeArrayCreateVector() returned NULL\n");
    ptr = NULL;
    hr = SafeArrayAccessData(V_ARRAY(&src), &ptr);
    EXPECT_HR(hr, S_OK);
    ok(ptr != NULL, "SafeArrayAccessData() returned NULL\n");

    memcpy(ptr, szComplete1, lstrlenW(szComplete1) * sizeof(WCHAR));
    hr = SafeArrayUnlock(V_ARRAY(&src));
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_load(doc, src, &b);
    todo_wine EXPECT_HR(hr, E_INVALIDARG);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    VariantClear(&src);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_nsnamespacemanager(void)
{
    static const char xmluriA[] = "http://www.w3.org/XML/1998/namespace";
    IMXNamespaceManager *nsmgr;
    IVBMXNamespaceManager *mgr2;
    IDispatch *disp;
    HRESULT hr;
    WCHAR buffW[250];
    INT len;

    hr = CoCreateInstance(&CLSID_MXNamespaceManager40, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMXNamespaceManager, (void**)&nsmgr);
    if (hr != S_OK)
    {
        win_skip("MXNamespaceManager is not available\n");
        return;
    }

    /* IMXNamespaceManager inherits from IUnknown */
    hr = IMXNamespaceManager_QueryInterface(nsmgr, &IID_IDispatch, (void**)&disp);
    EXPECT_HR(hr, S_OK);
    IDispatch_Release(disp);

    hr = IMXNamespaceManager_QueryInterface(nsmgr, &IID_IVBMXNamespaceManager, (void**)&mgr2);
    EXPECT_HR(hr, S_OK);
    IVBMXNamespaceManager_Release(mgr2);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, NULL);
    EXPECT_HR(hr, S_OK);

    /* prefix already added */
    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, _bstr_("ns0 uri"));
    EXPECT_HR(hr, S_FALSE);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns0"), NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    /* "xml" and "xmlns" are not allowed here */
    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("xml"), _bstr_("uri1"));
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("xmlns"), _bstr_("uri1"));
    EXPECT_HR(hr, E_INVALIDARG);
todo_wine {
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, -1, NULL, NULL);
    EXPECT_HR(hr, E_FAIL);
}
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, NULL, NULL);
    EXPECT_HR(hr, E_POINTER);

    len = -1;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, NULL, &len);
    EXPECT_HR(hr, S_OK);
    ok(len == 3, "got %d\n", len);

    len = -1;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, buffW, &len);
    EXPECT_HR(hr, E_XML_BUFFERTOOSMALL);
    ok(len == -1, "got %d\n", len);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);

    len = 10;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(len == 3, "got %d\n", len);
    ok(!lstrcmpW(buffW, _bstr_("xml")), "got prefix %s\n", wine_dbgstr_w(buffW));

    /* getURI */
    hr = IMXNamespaceManager_getURI(nsmgr, NULL, NULL, NULL, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    len = -1;
    hr = IMXNamespaceManager_getURI(nsmgr, NULL, NULL, NULL, &len);
    EXPECT_HR(hr, E_INVALIDARG);
    ok(len == -1, "got %d\n", len);

    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml"), NULL, NULL, NULL);
    EXPECT_HR(hr, E_POINTER);

    len = -1;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml"), NULL, NULL, &len);
    EXPECT_HR(hr, S_OK);
    /* length of "xml" uri is constant */
    ok(len == strlen(xmluriA), "got %d\n", len);

    len = 100;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml"), NULL, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(len == strlen(xmluriA), "got %d\n", len);
    ok(!lstrcmpW(buffW, _bstr_(xmluriA)), "got prefix %s\n", wine_dbgstr_w(buffW));

    len = strlen(xmluriA)-1;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml"), NULL, buffW, &len);
    EXPECT_HR(hr, E_XML_BUFFERTOOSMALL);
    ok(len == strlen(xmluriA)-1, "got %d\n", len);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);

    /* prefix xml1 not defined */
    len = -1;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml1"), NULL, NULL, &len);
    EXPECT_HR(hr, S_FALSE);
    ok(len == 0, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml1"), NULL, buffW, &len);
    EXPECT_HR(hr, S_FALSE);
    ok(buffW[0] == 0, "got %x\n", buffW[0]);
    ok(len == 0, "got %d\n", len);

    IMXNamespaceManager_Release(nsmgr);

    free_bstrs();
}

static void test_nsnamespacemanager_override(void)
{
    IMXNamespaceManager *nsmgr;
    WCHAR buffW[250];
    VARIANT_BOOL b;
    HRESULT hr;
    INT len;

    hr = CoCreateInstance(&CLSID_MXNamespaceManager40, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMXNamespaceManager, (void**)&nsmgr);
    if (hr != S_OK)
    {
        win_skip("MXNamespaceManager is not available\n");
        return;
    }

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(buffW, _bstr_("xml")), "got prefix %s\n", wine_dbgstr_w(buffW));

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 1, buffW, &len);
    EXPECT_HR(hr, E_FAIL);

    hr = IMXNamespaceManager_getAllowOverride(nsmgr, NULL);
    EXPECT_HR(hr, E_POINTER);

    b = VARIANT_FALSE;
    hr = IMXNamespaceManager_getAllowOverride(nsmgr, &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IMXNamespaceManager_putAllowOverride(nsmgr, VARIANT_FALSE);
    EXPECT_HR(hr, S_OK);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, _bstr_("ns0 uri"));
    EXPECT_HR(hr, S_OK);

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_(""), NULL, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(buffW, _bstr_("ns0 uri")), "got uri %s\n", wine_dbgstr_w(buffW));

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns0"), _bstr_("ns0 uri"));
    EXPECT_HR(hr, S_OK);

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(buffW, _bstr_("xml")), "got prefix %s\n", wine_dbgstr_w(buffW));

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 1, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(buffW, _bstr_("ns0")), "got prefix %s\n", wine_dbgstr_w(buffW));

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 2, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(buffW, _bstr_("")), "got prefix %s\n", wine_dbgstr_w(buffW));

    /* new prefix placed at index 1 always */
    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns1"), _bstr_("ns1 uri"));
    EXPECT_HR(hr, S_OK);

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 1, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(buffW, _bstr_("ns1")), "got prefix %s\n", wine_dbgstr_w(buffW));

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_(""), NULL);
    todo_wine EXPECT_HR(hr, E_FAIL);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, NULL);
    EXPECT_HR(hr, E_FAIL);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, _bstr_("ns0 uri"));
    EXPECT_HR(hr, E_FAIL);

    hr = IMXNamespaceManager_putAllowOverride(nsmgr, VARIANT_TRUE);
    EXPECT_HR(hr, S_OK);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, _bstr_("ns0 uri override"));
    EXPECT_HR(hr, S_FALSE);

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_(""), NULL, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(buffW, _bstr_("ns0 uri override")), "got uri %s\n", wine_dbgstr_w(buffW));

    len = sizeof(buffW)/sizeof(WCHAR);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 3, buffW, &len);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(buffW, _bstr_("")), "got prefix %s\n", wine_dbgstr_w(buffW));

    IMXNamespaceManager_Release(nsmgr);

    free_bstrs();
}

static void test_domobj_dispex(IUnknown *obj)
{
    DISPID dispid = DISPID_XMLDOM_NODELIST_RESET;
    IDispatchEx *dispex;
    IUnknown *unk;
    DWORD props;
    UINT ticnt;
    HRESULT hr;
    BSTR name;

    hr = IUnknown_QueryInterface(obj, &IID_IDispatchEx, (void**)&dispex);
    EXPECT_HR(hr, S_OK);
    if (FAILED(hr)) return;

    ticnt = 0;
    hr = IDispatchEx_GetTypeInfoCount(dispex, &ticnt);
    EXPECT_HR(hr, S_OK);
    ok(ticnt == 1, "ticnt=%u\n", ticnt);

    name = SysAllocString(szstar);
    hr = IDispatchEx_DeleteMemberByName(dispex, name, fdexNameCaseSensitive);
    EXPECT_HR(hr, E_NOTIMPL);
    SysFreeString(name);

    hr = IDispatchEx_DeleteMemberByDispID(dispex, dispid);
    EXPECT_HR(hr, E_NOTIMPL);

    props = 0;
    hr = IDispatchEx_GetMemberProperties(dispex, dispid, grfdexPropCanAll, &props);
    EXPECT_HR(hr, E_NOTIMPL);
    ok(props == 0, "expected 0 got %d\n", props);

    hr = IDispatchEx_GetMemberName(dispex, dispid, &name);
    EXPECT_HR(hr, E_NOTIMPL);
    if (SUCCEEDED(hr)) SysFreeString(name);

    hr = IDispatchEx_GetNextDispID(dispex, fdexEnumDefault, DISPID_XMLDOM_NODELIST_RESET, &dispid);
    EXPECT_HR(hr, E_NOTIMPL);

    hr = IDispatchEx_GetNameSpaceParent(dispex, &unk);
    EXPECT_HR(hr, E_NOTIMPL);
    if (hr == S_OK && unk) IUnknown_Release(unk);

    IDispatchEx_Release(dispex);
}

static const DOMNodeType dispex_types_test[] =
{
    NODE_ELEMENT,
    NODE_ATTRIBUTE,
    NODE_TEXT,
    NODE_CDATA_SECTION,
    NODE_ENTITY_REFERENCE,
    NODE_PROCESSING_INSTRUCTION,
    NODE_COMMENT,
    NODE_DOCUMENT_FRAGMENT,
    NODE_INVALID
};

static void test_dispex(void)
{
    const DOMNodeType *type = dispex_types_test;
    IXMLDOMImplementation *impl;
    IXMLDOMNodeList *node_list;
    IXMLDOMParseError *error;
    IXMLDOMNamedNodeMap *map;
    IXMLDOMDocument *doc;
    IXMLHTTPRequest *req;
    IXMLDOMElement *elem;
    IDispatchEx *dispex;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    IUnknown *unk;
    HRESULT hr;
    DISPID did;

    doc = create_document(&IID_IXMLDOMDocument);

    IXMLDOMDocument_QueryInterface(doc, &IID_IUnknown, (void**)&unk);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);

    for(; *type != NODE_INVALID; type++)
    {
        IXMLDOMNode *node;
        VARIANT v;

        V_VT(&v) = VT_I2;
        V_I2(&v) = *type;

        hr = IXMLDOMDocument_createNode(doc, v, _bstr_("name"), NULL, &node);
        ok(hr == S_OK, "failed to create node type %d\n", *type);

        IXMLDOMNode_QueryInterface(node, &IID_IUnknown, (void**)&unk);

        test_domobj_dispex(unk);
        IUnknown_Release(unk);
        IXMLDOMNode_Release(node);
    }

    /* IXMLDOMNodeList */
    hr = IXMLDOMDocument_getElementsByTagName(doc, _bstr_("*"), &node_list);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNodeList_QueryInterface(node_list, &IID_IUnknown, (void**)&unk);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);
    IXMLDOMNodeList_Release(node_list);

    /* IXMLDOMParseError */
    hr = IXMLDOMDocument_get_parseError(doc, &error);
    EXPECT_HR(hr, S_OK);
    IXMLDOMParseError_QueryInterface(error, &IID_IUnknown, (void**)&unk);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);
    IXMLDOMParseError_Release(error);

    /* IXMLDOMNamedNodeMap */
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(xpath_simple_list), &b);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("root/a"), &node_list);
    EXPECT_HR(hr, S_OK);
    hr = IXMLDOMNodeList_get_item(node_list, 0, &node);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNodeList_Release(node_list);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNode_Release(node);
    hr = IXMLDOMElement_get_attributes(elem, &map);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IUnknown, (void**)&unk);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);
    /* collection dispex test */
    hr = IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IDispatchEx, (void**)&dispex);
    EXPECT_HR(hr, S_OK);
    did = 0;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("0"), 0, &did);
    EXPECT_HR(hr, S_OK);
    ok(did == DISPID_DOM_COLLECTION_BASE, "got 0x%08x\n", did);
    IDispatchEx_Release(dispex);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("root/b"), &node_list);
    EXPECT_HR(hr, S_OK);
    hr = IXMLDOMNodeList_get_item(node_list, 0, &node);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNodeList_Release(node_list);
    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
    EXPECT_HR(hr, S_OK);
    IXMLDOMNode_Release(node);
    hr = IXMLDOMElement_get_attributes(elem, &map);
    EXPECT_HR(hr, S_OK);
    /* collection dispex test, empty collection */
    hr = IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IDispatchEx, (void**)&dispex);
    EXPECT_HR(hr, S_OK);
    did = 0;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("0"), 0, &did);
    EXPECT_HR(hr, S_OK);
    ok(did == DISPID_DOM_COLLECTION_BASE, "got 0x%08x\n", did);
    hr = IDispatchEx_GetDispID(dispex, _bstr_("1"), 0, &did);
    EXPECT_HR(hr, S_OK);
    ok(did == DISPID_DOM_COLLECTION_BASE+1, "got 0x%08x\n", did);
    IDispatchEx_Release(dispex);

    IXMLDOMNamedNodeMap_Release(map);
    IXMLDOMElement_Release(elem);

    /* IXMLDOMImplementation */
    hr = IXMLDOMDocument_get_implementation(doc, &impl);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMImplementation_QueryInterface(impl, &IID_IDispatchEx, (void**)&dispex);
    EXPECT_HR(hr, S_OK);
    IDispatchEx_Release(dispex);
    IXMLDOMImplementation_Release(impl);

    IXMLDOMDocument_Release(doc);

    /* IXMLHTTPRequest */
    hr = CoCreateInstance(&CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER,
        &IID_IXMLHttpRequest, (void**)&req);
    if (hr == S_OK)
    {
        hr = IXMLHTTPRequest_QueryInterface(req, &IID_IDispatchEx, (void**)&dispex);
        EXPECT_HR(hr, E_NOINTERFACE);
        IXMLHTTPRequest_Release(req);
    }

    free_bstrs();
}

static void test_parseerror(void)
{
    IXMLDOMParseError *error;
    IXMLDOMDocument *doc;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_parseError(doc, &error);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMParseError_get_line(error, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLDOMParseError_get_srcText(error, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    hr = IXMLDOMParseError_get_linepos(error, NULL);
    EXPECT_HR(hr, E_INVALIDARG);

    IXMLDOMParseError_Release(error);

    IXMLDOMDocument_Release(doc);
}

static void test_getAttributeNode(void)
{
    IXMLDOMAttribute *attr;
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    VARIANT_BOOL v;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szExampleXML), &v);
    EXPECT_HR(hr, S_OK);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    EXPECT_HR(hr, S_OK);

    str = SysAllocString(nonexistent_fileW);
    hr = IXMLDOMElement_getAttributeNode(elem, str, NULL);
    EXPECT_HR(hr, E_FAIL);

    attr = (IXMLDOMAttribute*)0xdeadbeef;
    hr = IXMLDOMElement_getAttributeNode(elem, str, &attr);
    EXPECT_HR(hr, E_FAIL);
    ok(attr == NULL, "got %p\n", attr);
    SysFreeString(str);

    str = SysAllocString(nonexistent_attrW);
    hr = IXMLDOMElement_getAttributeNode(elem, str, NULL);
    EXPECT_HR(hr, S_FALSE);

    attr = (IXMLDOMAttribute*)0xdeadbeef;
    hr = IXMLDOMElement_getAttributeNode(elem, str, &attr);
    EXPECT_HR(hr, S_FALSE);
    ok(attr == NULL, "got %p\n", attr);
    SysFreeString(str);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("foo:b"), &attr);
    EXPECT_HR(hr, S_OK);
    IXMLDOMAttribute_Release(attr);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("b"), &attr);
    EXPECT_HR(hr, S_FALSE);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("a"), &attr);
    EXPECT_HR(hr, S_OK);
    IXMLDOMAttribute_Release(attr);

    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

START_TEST(domdoc)
{
    IXMLDOMDocument *doc;
    HRESULT hr;

    hr = CoInitialize( NULL );
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK) return;

    test_XMLHTTP();

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc );
    if (hr != S_OK)
    {
        win_skip("IXMLDOMDocument is not available (0x%08x)\n", hr);
        return;
    }

    IXMLDOMDocument_Release(doc);

    test_domdoc();
    test_persiststreaminit();
    test_domnode();
    test_refs();
    test_create();
    test_getElementsByTagName();
    test_get_text();
    test_get_childNodes();
    test_get_firstChild();
    test_get_lastChild();
    test_removeChild();
    test_replaceChild();
    test_removeNamedItem();
    test_IXMLDOMDocument2();
    test_whitespace();
    test_XPath();
    test_XSLPattern();
    test_cloneNode();
    test_xmlTypes();
    test_nodeTypeTests();
    test_save();
    test_testTransforms();
    test_namespaces();
    test_FormattingXML();
    test_nodeTypedValue();
    test_TransformWithLoadingLocalFile();
    test_put_nodeValue();
    test_document_IObjectSafety();
    test_splitText();
    test_getQualifiedItem();
    test_removeQualifiedItem();
    test_get_ownerDocument();
    test_setAttributeNode();
    test_put_dataType();
    test_createNode();
    test_get_prefix();
    test_default_properties();
    test_selectSingleNode();
    test_events();
    test_createProcessingInstruction();
    test_put_nodeTypedValue();
    test_get_xml();
    test_insertBefore();
    test_appendChild();
    test_get_doctype();
    test_get_tagName();
    test_get_dataType();
    test_get_nodeTypeString();
    test_get_attributes();
    test_selection();
    test_load();
    test_dispex();
    test_parseerror();
    test_getAttributeNode();

    test_xsltemplate();

    test_nsnamespacemanager();
    test_nsnamespacemanager_override();

    CoUninitialize();
}
