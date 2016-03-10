/*
 * Copyright (C) 2013-2015
 *
 * @author jxfengzi@gmail.com
 * @date   2013-11-19
 *
 * @file   Object.c
 *
 * @remark
 *
 */

#include "Object.h"

#define TAG     "Object"

void Object_Construct(Object *thiz)
{
    RETURN_IF_FAIL(thiz);

    memset(thiz, 0, sizeof(Object));
}

void Object_Dispose(Object *thiz)
{
    RETURN_IF_FAIL(thiz);

    do
    {
        if (thiz->type.clazzType == CLAZZ_UNDEFINED)
        {
            break;
        }

        if (thiz->type.clazzType == CLAZZ_STRING)
        {
            if (thiz->value.stringValue != NULL)
            {
                free(thiz->value.stringValue);
            }
        }

        memset(thiz, 0, sizeof(Object));
    } while (0);
}

void Object_Copy(Object *dst, Object *src)
{
    RETURN_IF_FAIL(dst);
    RETURN_IF_FAIL(src);

    if (dst != src)
    {
        Object_Dispose(dst);

        memcpy(dst, src, sizeof(Object));
        if (src->type.clazzType == CLAZZ_STRING)
        {
            if (src->value.stringValue != NULL)
            {
                src->value.stringValue = tiny_strdup(src->value.stringValue);
            }
        }

        ObjectType_Copy(&dst->type, &src->type);
    }
}

void Object_setByte(Object *thiz, int8_t value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_BYTE;
    thiz->value.byteValue = value;
}

void Object_setWord(Object *thiz, int16_t value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_WORD;
    thiz->value.wordValue = value;
}

void Object_setInteger(Object *thiz, int32_t value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_INTEGER;
    thiz->value.integerValue = value;
}

void Object_setLong(Object *thiz, int64_t value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_LONG;
    thiz->value.longValue = value;

}

void Object_setFloat(Object *thiz, float value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_FLOAT;
    thiz->value.floatValue = value;
}

void Object_setDouble(Object *thiz, double value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_DOUBLE;
    thiz->value.doubleValue = value;
}

void Object_setBool(Object *thiz, bool value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_BOOLEAN;
    thiz->value.boolValue = value;
}

void Object_setChar(Object *thiz, char value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_CHAR;
    thiz->value.charValue = value;
}

void Object_setString(Object *thiz, const char * value)
{
    RETURN_IF_FAIL(thiz);

    Object_Dispose(thiz);
    thiz->type.clazzType = CLAZZ_STRING;
    if (value == NULL)
    {
        thiz->value.stringValue = NULL;
    }
    else
    {
        thiz->value.stringValue = tiny_strdup(value);
    }
}