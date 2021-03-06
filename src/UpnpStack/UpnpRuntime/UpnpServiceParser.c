/*
* Copyright (C) 2013-2015
*
* @author jxfengzi@gmail.com
* @date   2013-7-9
*
* @file   SDD.c
*
* @remark
*      set tabstop=4
*      set shiftwidth=4
*      set expandtab
*/

#include "UpnpServiceParser.h"
#include "TinyXml.h"
#include "HttpClient.h"
#include "tiny_log.h"
#include "tiny_str_equal.h"

#define TAG                             "SDD"


#define SERVICE_ID_LEN                   128

/* <root> */
#define SDD_ROOT_SCPD                   "scpd"

/* <specVersion> */
#define SDD_SEPCVERSION                 "specVersion"
#define SDD_SEPCVERSION_MAJOR           "major"
#define SDD_SEPCVERSION_MINOR           "minor"

/* <actionList> */
#define SDD_ACTIONLIST                                          "actionList"
#define SDD_ACTION                                              "action"
#define SDD_ACTION_NAME                                         "name"
#define SDD_ARGUMENTLIST                                        "argumentList"
#define SDD_ARGUMENT                                            "argument"
#define SDD_ARGUMENT_NAME                                       "name"
#define SDD_ARGUMENT_DIRECTION                                  "direction"
#define SDD_ARGUMENT_RELATED_STATE                              "relatedStateVariable"

/* <serviceStateTable> */
#define SDD_STATELIST                                           "serviceStateTable"
#define SDD_STATE                                               "stateVariable"
#define SDD_STATE_SENDEVENTS                                    "sendEvents"
#define SDD_STATE_NAME                                          "name"
#define SDD_STATE_DATATYPE                                      "dataType"
#if 0
#define SDD_ALLOWEDVALUELIST                                    "allowedValueList"
#define SDD_ALLOWEDVALUE                                        "allowedValue"
#define SDD_ALLOWEDVALUERANGE                                   "allowedValueRange"
#endif

/* <direction> */
#define DIRECTION_IN                                            "in"
#define DIRECTION_OUT                                           "out"


static TinyRet SDD_ParseXml(UpnpService *thiz, TinyXml *xml);
static TinyRet SDD_LoadSpecVersion(UpnpService *thiz, TinyXmlNode *root);
static TinyRet SDD_LoadActionList(UpnpService *thiz, TinyXmlNode *root);
static TinyRet SDD_LoadServiceStateTable(UpnpService *thiz, TinyXmlNode *root);

static uint32_t UpnpServiceParser_ActionToXml(UpnpAction *action, char *xml, uint32_t len);
static uint32_t UpnpServiceParser_StateToXml(UpnpStateVariable *state, char *xml, uint32_t len);

TinyRet UpnpServiceParser_Parse(const char *url, UpnpService *service, uint32_t timeout)
{
    LOG_TIME_BEGIN(TAG, UpnpService_Parse);
    TinyRet ret = TINY_RET_OK;
    HttpClient *client = NULL;
    HttpMessage *request = NULL;
    HttpMessage *response = NULL;

    RETURN_VAL_IF_FAIL(service, TINY_RET_E_ARG_NULL);
    RETURN_VAL_IF_FAIL(url, TINY_RET_E_ARG_NULL);

    LOG_D(TAG, "UpnpService_Parse: %s", url);

    do
    {
        client = HttpClient_New();
        if (client == NULL)
        {
            ret = TINY_RET_E_NEW;
            break;
        }

        request = HttpMessage_New();
        if (request == NULL)
        {
            ret = TINY_RET_E_NEW;
            break;
        }

        response = HttpMessage_New();
        if (response == NULL)
        {
            ret = TINY_RET_E_NEW;
            break;
        }

        do
        {
            TinyXml * xml = NULL;

            ret = HttpMessage_SetRequest(request, "GET", url);
            if (RET_FAILED(ret))
            {
                break;
            }

            ret = HttpClient_Execute(client, request, response, timeout);
            if (RET_FAILED(ret))
            {
                break;
            }

            if (HttpMessage_GetStatusCode(response) != HTTP_STATUS_OK)
            {
                LOG_D(TAG, "HttpMessage_GetStatusCode failed: %d %s",
                    HttpMessage_GetStatusCode(response),
                    HttpMessage_GetStatus(response));
                break;
            }

            xml = TinyXml_New();
            if (xml == NULL)
            {
                LOG_D(TAG, "Out of memory");
                ret = TINY_RET_E_NEW;
                break;
            }

            do
            {
                ret = TinyXml_Parse(xml, HttpMessage_GetContentObject(response), HttpMessage_GetContentSize(response));
                if (RET_FAILED(ret))
                {
                    LOG_D(TAG, "TinyXml_Parse failed: %s", tiny_ret_to_str(ret));
                    break;
                }

                ret = SDD_ParseXml(service, xml);
                if (RET_FAILED(ret))
                {
                    LOG_D(TAG, "SDD_ParseXml failed: %s", tiny_ret_to_str(ret));
                    break;
                }
            } while (0);

            TinyXml_Delete(xml);
        } while (0);
    } while (0);

    if (client != NULL)
    {
        HttpClient_Delete(client);
    }

    if (response != NULL)
    {
        HttpMessage_Delete(response);
    }

    if (request != NULL)
    {
        HttpMessage_Delete(request);
    }

    LOG_TIME_END(TAG, UpnpService_Parse);
    return ret;
}

static TinyRet SDD_ParseXml(UpnpService *thiz, TinyXml *xml)
{
    LOG_TIME_BEGIN(TAG, SDD_ParseXml);
    TinyRet ret = TINY_RET_OK;

    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);
    RETURN_VAL_IF_FAIL(xml, TINY_RET_E_ARG_NULL);

    do
    {
        TinyXmlNode *root = TinyXml_GetRoot(xml);
        if (root == NULL)
        {
            ret = TINY_RET_E_XML_INVALID;
            break;
        }

        if (!str_equal(TinyXmlNode_GetName(root), SDD_ROOT_SCPD, true))
        {
            ret = TINY_RET_E_XML_INVALID;
            break;
        }

        ret = SDD_LoadSpecVersion(thiz, root);
        if (RET_FAILED(ret))
        {
            break;
        }
        
        ret = SDD_LoadServiceStateTable(thiz, root);
        if (RET_FAILED(ret))
        {
            break;
        }

        ret = SDD_LoadActionList(thiz, root);
        if (RET_FAILED(ret))
        {
            break;
        }
    } while (0);

    LOG_TIME_END(TAG, SDD_ParseXml);

    return ret;
}

static TinyRet SDD_LoadSpecVersion(UpnpService *thiz, TinyXmlNode *root)
{
    TinyRet ret = TINY_RET_OK;

    do
    {
        TinyXmlNode *version = NULL;
        const char *major = NULL;
        const char *minor = NULL;
        int majorVersion = 0;
        int minorVersion = 0;

        version = TinyXmlNode_GetChildByName(root, SDD_SEPCVERSION);
        if (version == NULL)
        {
            ret = TINY_RET_E_XML_INVALID;
            break;
        }

        major = TinyXmlNode_GetChildContent(version, SDD_SEPCVERSION_MAJOR);
        minor = TinyXmlNode_GetChildContent(version, SDD_SEPCVERSION_MINOR);
        if (major == NULL || minor == NULL)
        {
            ret = TINY_RET_E_XML_INVALID;
            break;
        }

        majorVersion = atoi(major);
        minorVersion = atoi(minor);

        if (majorVersion != 1 || minorVersion != 0)
        {
            LOG_W(TAG, "SpecVersion: %d.%d", majorVersion, minorVersion);
        }
    } while (0);

    return ret;
}

static TinyRet SDD_LoadServiceStateTable(UpnpService *thiz, TinyXmlNode *root)
{
    TinyRet ret = TINY_RET_OK;

    do
    {
        uint32_t count = 0;
        uint32_t i = 0;

        /* <serviceStateTable> */
        TinyXmlNode *actionList = TinyXmlNode_GetChildByName(root, SDD_STATELIST);
        if (actionList == NULL)
        {
            ret = TINY_RET_E_XML_INVALID;
            break;
        }

        count = TinyXmlNode_GetChildren(actionList);
        if (count == 0)
        {
            ret = TINY_RET_E_XML_INVALID;
            break;
        }

        for (i = 0; i < count; i++)
        {
            UpnpStateVariable * stateVariable = NULL;
            TinyXmlNode *node_state = NULL;
            TinyXmlAttr *attr_sendEvents = NULL;
            const char *name = NULL;
            const char *dataType = NULL;

            node_state = TinyXmlNode_GetChildAt(actionList, i);
            if (!str_equal(TinyXmlNode_GetName(node_state), SDD_STATE, true))
            {
                continue;
            }

            attr_sendEvents = TinyXmlNode_GetAttr(node_state, SDD_STATE_SENDEVENTS);
            if (attr_sendEvents == NULL)
            {
                continue;
            }

            name = TinyXmlNode_GetChildContent(node_state, SDD_STATE_NAME);
            if (name == NULL)
            {
                continue;
            }

            dataType = TinyXmlNode_GetChildContent(node_state, SDD_STATE_DATATYPE);
            if (dataType == NULL)
            {
                continue;
            }

            stateVariable = UpnpStateVariable_New();
            if (stateVariable == NULL)
            {
                LOG_E(TAG, "OUT OF MEMORY");
                ret = TINY_RET_E_NEW;
                break;
            }

            UpnpStateVariable_Initialize(stateVariable, name, dataType, NULL, attr_sendEvents->value);
            UpnpService_AddStateVariable(thiz, stateVariable);
        }
    } while (0);

    return ret;
}

static TinyRet SDD_LoadActionList(UpnpService *thiz, TinyXmlNode *root)
{
    TinyRet ret = TINY_RET_OK;

    do
    {
        uint32_t count = 0;
        uint32_t i = 0;

        TinyXmlNode *actionList = TinyXmlNode_GetChildByName(root, SDD_ACTIONLIST);
        if (actionList == NULL)
        {
            ret = TINY_RET_E_XML_INVALID;
            break;
        }

        count = TinyXmlNode_GetChildren(actionList);
        if (count == 0)
        {
            ret = TINY_RET_E_XML_INVALID;
            break;
        }

        for (i = 0; i < count; i++)
        {
            TinyXmlNode *node_action = NULL;
            TinyXmlNode *node_arg_list = NULL;
            UpnpAction *action = NULL;
            const char *action_name = NULL;
            uint32_t action_arg_count = 0;
            uint32_t j = 0;

            node_action = TinyXmlNode_GetChildAt(actionList, i);
            if (!str_equal(TinyXmlNode_GetName(node_action), SDD_ACTION, true))
            {
                continue;
            }

            action_name = TinyXmlNode_GetChildContent(node_action, SDD_ACTION_NAME);
            if (action_name == NULL)
            {
                continue;
            }

            node_arg_list = TinyXmlNode_GetChildByName(node_action, SDD_ARGUMENTLIST);
            if (node_arg_list == NULL)
            {
                continue;
            }

            if (TinyXmlNode_GetChildren(node_arg_list) == 0)
            {
                continue;
            }

            action = UpnpAction_New();
            if (action == NULL)
            {
                ret = TINY_RET_E_OUT_OF_MEMORY;
                break;
            }

            UpnpAction_SetName(action, action_name);

            action_arg_count = TinyXmlNode_GetChildren(node_arg_list);
            for (j = 0; j < action_arg_count; j++)
            {
                UpnpArgument *argument;
                UpnpArgumentDirection direction = ARG_UNKNOWN;
                TinyXmlNode *node_arg = NULL;
                const char * arg_name = NULL;
                const char * arg_direction = NULL;
                const char * arg_releatedState = NULL;

                node_arg = TinyXmlNode_GetChildAt(node_arg_list, j);
                if (!str_equal(TinyXmlNode_GetName(node_arg), SDD_ARGUMENT, true))
                {
                    continue;
                }

                arg_name = TinyXmlNode_GetChildContent(node_arg, SDD_ARGUMENT_NAME);
                arg_direction = TinyXmlNode_GetChildContent(node_arg, SDD_ARGUMENT_DIRECTION);
                arg_releatedState = TinyXmlNode_GetChildContent(node_arg, SDD_ARGUMENT_RELATED_STATE);
                if (arg_name == NULL || arg_direction == NULL || arg_releatedState == NULL)
                {
                    LOG_D(TAG, "<%s> or <%s> or <%s> is NULL",
                        SDD_ARGUMENT_NAME,
                        SDD_ARGUMENT_DIRECTION,
                        SDD_ARGUMENT_RELATED_STATE);
                    continue;
                }

                direction = UpnpArgumentDirection_Retrieve(arg_direction);
                if (direction == ARG_UNKNOWN)
                {
                    LOG_D(TAG, "<%s> invalid : %s", SDD_ARGUMENT_DIRECTION, arg_direction);
                    continue;
                }

                argument = UpnpArgument_New(arg_name, direction, arg_releatedState);
                if (argument == NULL)
                {
                    LOG_D(TAG, "OUT OF MEMORY");
                    continue;
                }

                UpnpAction_AddArgument(action, argument);
            }

            ret = UpnpService_AddAction(thiz, action);
            if (RET_FAILED(ret))
            {
                LOG_D(TAG, "UpnpActionList_AddAction failed: <%s>", action_name);
            }
        }
    } while (0);

    return ret;
}

static uint32_t UpnpServiceParser_ActionToXml(UpnpAction *action, char *xml, uint32_t len)
{
    RETURN_VAL_IF_FAIL(action, 0);
    RETURN_VAL_IF_FAIL(xml, 0);

    do
    {
        uint32_t i = 0;
        uint32_t count = 0;

        strcat(xml, "<action>");

        {
            char name[1024];
            memset(name, 0, 1024);
            tiny_snprintf(name, 1024, "<name>%s</name>", UpnpAction_GetName(action));
            strcat(xml, name);
        }

        strcat(xml, "<argumentList>");

        count = UpnpAction_GetArgumentCount(action);
        for (i = 0; i < count; ++i)
        {
            UpnpArgument * arg = UpnpAction_GetArgumentAt(action, i);
            char name[1024];
            char direction[1024];
            char relatedStateVariable[1024];

            memset(name, 0, 1024);
            memset(direction, 0, 1024);
            memset(relatedStateVariable, 0, 1024);

            tiny_snprintf(name, 1024, "<name>%s</name>", UpnpArgument_GetName(arg));
            tiny_snprintf(direction, 1024, "<direction>%s</direction>", UpnpArgumentDirection_ToString(UpnpArgument_GetDirection(arg)));
            tiny_snprintf(relatedStateVariable, 1024, "<relatedStateVariable>%s</relatedStateVariable>",
                UpnpArgument_GetRelatedStateVariable(arg));

            strcat(xml, "<argument>");
            strcat(xml, name);
            strcat(xml, direction);
            strcat(xml, relatedStateVariable);
            strcat(xml, "</argument>");
        }

        strcat(xml, "</argumentList>");
        strcat(xml, "</action>");
    } while (0);

    return strlen(xml);
}

static uint32_t UpnpServiceParser_StateToXml(UpnpStateVariable *state, char *xml, uint32_t len)
{
    RETURN_VAL_IF_FAIL(state, 0);
    RETURN_VAL_IF_FAIL(xml, 0);

    do
    {
        char buf[1024 + 1];

        memset(buf, 0, 1024 + 1);
        tiny_snprintf(buf, 1024, "<stateVariable sendEvents=\"%s\">", DataType_BooleanToString(state->sendEvents));
        strcat(xml, buf);

        memset(buf, 0, 1024 + 1);
        tiny_snprintf(buf, 1024, "<name>%s</name>", state->definition.name);
        strcat(xml, buf);

        memset(buf, 0, 1024 + 1);
        tiny_snprintf(buf, 1024, "<dataType>%s</dataType>", state->definition.dataType.name);
        strcat(xml, buf);

        strcat(xml, "</stateVariable>");
    } while (0);

    return strlen(xml);
}

uint32_t UpnpServiceParser_ToXml(UpnpService *service, char *xml, uint32_t len)
{
    RETURN_VAL_IF_FAIL(service, 0);
    RETURN_VAL_IF_FAIL(xml, 0);

    do
    {
        uint32_t i = 0;
        uint32_t count = 0;

        strcat(xml, 
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">"
            "<specVersion>"
            "<major>1</major>"
            "<minor>0</minor>"
            "</specVersion>");

        /* <actionList> */
        strcat(xml, "<actionList>");
        count = UpnpService_GetActionCount(service);
        for (i = 0; i < count; ++i)
        {
            UpnpAction *action = UpnpService_GetActionAt(service, i);
            char buf[1024 * 4];
            memset(buf, 0, 1024 * 4);
            UpnpServiceParser_ActionToXml(action, buf, 1024 * 4);
            strcat(xml, buf);
        }
        strcat(xml, "</actionList>");

        /* <stateVariableList> */
        strcat(xml, "<serviceStateTable>");
        count = UpnpService_GetStateVariableCount(service);
        for (i = 0; i < count; ++i)
        {
            UpnpStateVariable * state = UpnpService_GetStateVariableAt(service, i);
            char buf[1024 * 4];
            memset(buf, 0, 1024 * 4);
            UpnpServiceParser_StateToXml(state, buf, 1024 * 4);
            strcat(xml, buf);
        }
        strcat(xml, "</serviceStateTable>");

        strcat(xml, "</scpd>");
    } while (0);

    return strlen(xml);
}