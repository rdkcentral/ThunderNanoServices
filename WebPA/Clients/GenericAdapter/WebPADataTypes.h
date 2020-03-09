/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include <stdint.h>
#include <string>
#include <core/core.h>

enum {
    Ok = 0,
    NOk = -1,
    NotHandled = -2
};

typedef enum _NotifyType
{
    PARAM_NOTIFY = 0,
    UPSTREAM_MSG,
    TRANS_STATUS,
    CONNECTED_CLIENT_NOTIFY,
    PARAM_VALUE_CHANGE_NOTIFY
} NotifyType;

typedef enum _EventId {
    EVENT_ADD,
    EVENT_REMOVE,
    EVENT_VALUECHANGED,
    EVENT_MAX,
} EventId;

typedef enum _faultCodes
{
    NoFault = 0,
    Error,
    MethodNotSupported = 9000,
    RequestDenied,
    InternalError,
    InvalidArguments,
    ResourcesExceeded,
    InvalidParameterName,
    InvalidParameterType,
    InvalidParameterValue,
    AttemptToSetaNonWritableParameter = 9008,
} FaultCode;

class Variant {
public:
    enum ParamType : uint8_t {
        TypeString = 0,
        TypeInteger,
        TypeUnsignedInteger,
        TypeBoolean,
        TypeDateTime,
        TypeBase64,
        TypeLong,
        TypeUnsignedLong,
        TypeFloat,
        TypeDouble,
        TypeByte,
        TypeNone = 0xFF
    };

    union Value;

public:
    Variant& operator=(const Variant& RHS)
    {
        _type = RHS._type;
        Value(RHS._value);
        return (*this);
    }

    Variant(const Variant& copy) : _type(copy._type)
    {
        Value(copy._value);
    }

    Variant() : _type(TypeNone)
    {
    }
    Variant(const std::string& value) : _type(TypeString)
    {
        new (&_value.typeString) std::string();
        _value.typeString = value;
    }
    Variant(const int& value) : _type(TypeInteger)
    {
        _value.typeInteger = value;
    }
    Variant(const unsigned int& value) : _type(TypeUnsignedInteger)
    {
        _value.typeUnsignedInteger = value;
    }
    Variant(const bool& value) : _type(TypeBoolean)
    {
        _value.typeBoolean = value;
    }
    Variant(const uint64_t& value) : _type(TypeDateTime)
    {
        _value.typeDateTime = value;
    }
    Variant(const std::vector<unsigned char>& value) : _type(TypeBase64)
    {
        new (&_value.typeBase64) std::vector<unsigned char>();
        _value.typeBase64 = value;
    }
    Variant(const long& value) : _type(TypeLong)
    {
        _value.typeLong = value;
    }
    Variant(const unsigned long& value) : _type(TypeUnsignedLong)
    {
        _value.typeUnsignedLong = value;
    }
    Variant(const float& value) : _type(TypeFloat)
    {
        _value.typeFloat = value;
    }
    Variant(const double& value) : _type(TypeDouble)
    {
        _value.typeDouble = value;
    }
    Variant(const unsigned char& value) : _type(TypeByte)
    {
        _value.typeByte = value;
    }

    virtual ~Variant()
    {
        switch(_type)
        {
        case TypeString:
            _value.typeString.~basic_string();
            break;
        case TypeBase64:
            _value.typeBase64.~vector();
            break;
        default:
            break;
        }
    }

public:
    inline ParamType Type() const
    {
        return (_type);
    }
    inline void Type(const ParamType type)
    {
        _type = type;
    }
    inline const std::string& String() const
    {
        ASSERT(_type == TypeString);
        return(_value.typeString);
    }
    inline int Integer() const
    {
        ASSERT(_type == TypeInteger);
        return(_value.typeInteger);
    }
    inline unsigned int UnsignedInteger() const
    {
        ASSERT(_type == TypeUnsignedInteger);
        return(_value.typeUnsignedInteger);
    }
    inline bool Boolean() const
    {
        ASSERT(_type == TypeBoolean);
        return(_value.typeBoolean);
    }
    inline uint64_t DateTime() const
    {
        ASSERT(_type == TypeDateTime);
        return(_value.typeDateTime);
    }
    inline std::vector<unsigned char> Base64() const
    {
        ASSERT(_type == TypeBase64);
        return(_value.typeBase64);
    }
    inline uint64_t Long() const
    {
        ASSERT(_type == TypeLong);
        return(_value.typeLong);
    }
    inline unsigned long UnsignedLong() const
    {
        ASSERT(_type == TypeUnsignedLong);
        return(_value.typeUnsignedLong);
    }
    inline float Float() const
    {
        ASSERT(_type == TypeFloat);
        return(_value.typeFloat);
    }
    inline double Double() const
    {
        ASSERT(_type == TypeDouble);
        return(_value.typeDouble);
    }
    inline unsigned char Byte() const
    {
        ASSERT(_type == TypeByte);
        return(_value.typeByte);
    }

private:
    void Value(const Value& value)
    {
        switch (_type)
        {
        case TypeString: {
            new (&_value.typeString) std::string();
            _value.typeString = value.typeString;
            break;
        }
        case TypeInteger: {
            _value.typeInteger = value.typeInteger;
            break;
        }
        case TypeUnsignedInteger: {
            _value.typeUnsignedInteger = value.typeUnsignedInteger;
            break;
        }
        case TypeBoolean: {
            _value.typeBoolean = value.typeBoolean;
            break;
        }
        case TypeDateTime: {
            _value.typeDateTime = value.typeDateTime;
            break;
        }
        case TypeBase64: {
            new (&_value.typeBase64) std::vector<unsigned char>();
            _value.typeBase64 = value.typeBase64;
            break;
        }
        case TypeLong: {
            _value.typeLong = value.typeLong;
            break;
        }
        case TypeUnsignedLong: {
            _value.typeUnsignedLong = value.typeUnsignedLong;
            break;
        }
        case TypeFloat: {
            _value.typeFloat = value.typeFloat;
            break;
        }
        case TypeDouble: {
            _value.typeDouble = value.typeDouble;
            break;
        }
        case TypeByte: {
            _value.typeByte = value.typeByte;
            break;
        }
        default:
            break;
        }
    }

private:
    ParamType _type;
    union Value {
    public:
        Value(const Value&) = delete;
        Value& operator=(const Value&) = delete;

    public:
        Value()
        {
        }
        Value(const std::string type)
            : typeString(type)
        {
        }
        Value(const int type)
            : typeInteger(type)
        {
        }
        Value(const unsigned int type)
            : typeUnsignedInteger(type)
        {
        }
        Value(const bool type)
            : typeBoolean(type)
        {
        }
        Value(const uint64_t type)
            : typeDateTime(type)
        {
        }
        Value(const std::vector<unsigned char> type)
            : typeBase64(type)
        {
        }
        Value(const long type)
            : typeLong(type)
        {
        }
        Value(const unsigned long type)
            : typeUnsignedLong(type)
        {
        }
        Value(const float type)
            : typeFloat(type)
        {
        }
        Value(const double type)
            : typeDouble(type)
        {
        }
        Value(const unsigned char type)
            : typeByte(type)
        {
        }

       ~Value()
        {
        }

    public:
        std::string   typeString;
        int           typeInteger;
        unsigned int  typeUnsignedInteger;
        bool          typeBoolean;
        uint64_t      typeDateTime;
        std::vector<unsigned char> typeBase64;
        long          typeLong;
        unsigned long typeUnsignedLong;
        float         typeFloat;
        double        typeDouble;
        unsigned char typeByte;
    }_value;
};

class Data {
public:
    Data() : _name(), _value() {
    }
    Data(const std::string& name) : _name (name), _value() {
    }
    Data(const std::string& name, const Variant& value) : _name(name), _value(value) {
    }
    inline bool operator<(const Data& rhs) const
    {
       return (_name < rhs._name);
    }

    std::string Name() const {
        return _name;
    }
    Variant Value() const {
        return _value;
    }
    void Value(const Variant& value) {
        _value = value;
    }
private:
    std::string _name;
    Variant _value;
};

typedef Data EventData;
typedef Data ParamNotify;
typedef Data AttributeData;
typedef std::string TransData;

typedef struct _NodeData
{
    std::string nodeMacId;
    std::string status;
} NodeData;

typedef struct
{
    NotifyType type;
    union
    {
        ParamNotify* notify;
        TransData* status;
        NodeData* node;
    } data;
} NotifyData;

typedef enum
{
    WEBPA_SUCCESS = 0,                    /**< Success. */
    WEBPA_FAILURE,                        /**< General Failure */
    WEBPA_ERR_TIMEOUT,
    WEBPA_ERR_NOT_EXIST,
    WEBPA_ERR_INVALID_PARAMETER_NAME,
    WEBPA_ERR_INVALID_PARAMETER_TYPE,
    WEBPA_ERR_INVALID_PARAMETER_VALUE,
    WEBPA_ERR_NOT_WRITABLE,
    WEBPA_ERR_SETATTRIBUTE_REJECTED,
    WEBPA_ERR_NAMESPACE_OVERLAP,
    WEBPA_ERR_UNKNOWN_COMPONENT,
    WEBPA_ERR_NAMESPACE_MISMATCH,
    WEBPA_ERR_UNSUPPORTED_NAMESPACE,
    WEBPA_ERR_DP_COMPONENT_VERSION_MISMATCH,
    WEBPA_ERR_INVALID_PARAM,
    WEBPA_ERR_UNSUPPORTED_DATATYPE,
    WEBPA_ERR_RESOURCES,
    WEBPA_ERR_WIFI_BUSY,
    WEBPA_ERR_INVALID_ATTRIBUTES,
    WEBPA_ERR_WILDCARD_NOT_SUPPORTED,
    WEBPA_ERR_SET_OF_CMC_OR_CID_NOT_SUPPORTED,
    WEBPA_ERR_VALUE_IS_EMPTY,
    WEBPA_ERR_VALUE_IS_NULL,
    WEBPA_ERR_DATATYPE_IS_NULL,
    WEBPA_ERR_CMC_TEST_FAILED,
    WEBPA_ERR_NEW_CID_IS_MISSING,
    WEBPA_ERR_CID_TEST_FAILED,
    WEBPA_ERR_SETTING_CMC_OR_CID,
    WEBPA_ERR_INVALID_INPUT_PARAMETER,
    WEBPA_ERR_ATTRIBUTES_IS_NULL,
    WEBPA_ERR_NOTIFY_IS_NULL,
    WEBPA_ERR_INVALID_WIFI_INDEX,
    WEBPA_ERR_INVALID_RADIO_INDEX,
    WEBPA_ERR_ATOMIC_GET_SET_FAILED,
    WEBPA_ERR_METHOD_NOT_SUPPORTED
} WebPAStatus;
