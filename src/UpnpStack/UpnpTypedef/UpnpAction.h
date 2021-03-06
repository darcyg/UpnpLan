/*
* Copyright (C) 2013-2015
*
* @author jxfengzi@gmail.com
* @date   2013-11-19
*
* @file   UpnpAction.h
*
* @remark
*
*/

#ifndef __UPNP_ACTION_H__
#define __UPNP_ACTION_H__

#include "tiny_base.h"
#include "upnp_api.h"
#include "UpnpArgument.h"

TINY_BEGIN_DECLS


struct _UpnpAction;
typedef struct _UpnpAction UpnpAction;

UPNP_API UpnpAction * UpnpAction_New(void);
UPNP_API void UpnpAction_Delete(UpnpAction *thiz);

UPNP_API void UpnpAction_SetParentService(UpnpAction *thiz, void *service);
UPNP_API void * UpnpAction_GetParentService(UpnpAction *thiz);

UPNP_API TinyRet UpnpAction_SetName(UpnpAction *thiz, const char *name);
UPNP_API const char * UpnpAction_GetName(UpnpAction *thiz);

UPNP_API TinyRet UpnpAction_AddArgument(UpnpAction *thiz, UpnpArgument *argument);
UPNP_API uint32_t UpnpAction_GetArgumentCount(UpnpAction *thiz);
UPNP_API UpnpArgument * UpnpAction_GetArgumentAt(UpnpAction *thiz, uint32_t index);
UPNP_API UpnpArgument * UpnpAction_GetArgument(UpnpAction *thiz, const char *argumentName);
UPNP_API const char * UpnpAction_GetArgumentRelatedStateVariable(UpnpAction *thiz, const char *argumentName);


TINY_END_DECLS

#endif /* __UPNP_ACTION_H__ */