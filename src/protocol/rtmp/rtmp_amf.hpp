/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */
#pragma once

#include <string>
#include <map>
#include <vector>
#include "util/util.hpp"
#include <defs/err.hpp>
#include <defs/tmss_def.hpp>
#include <rtmp_def.hpp>
#include <io/io_buffer.hpp>

namespace tmss {
// AMF0 marker
#define RTMP_AMF0_Number                     0x00
#define RTMP_AMF0_Boolean                     0x01
#define RTMP_AMF0_String                     0x02
#define RTMP_AMF0_Object                     0x03
#define RTMP_AMF0_MovieClip                 0x04    // reserved, not supported
#define RTMP_AMF0_Null                         0x05
#define RTMP_AMF0_Undefined                 0x06
#define RTMP_AMF0_Reference                 0x07
#define RTMP_AMF0_EcmaArray                 0x08
#define RTMP_AMF0_ObjectEnd                 0x09
#define RTMP_AMF0_StrictArray                 0x0A
#define RTMP_AMF0_Date                         0x0B
#define RTMP_AMF0_LongString                 0x0C
#define RTMP_AMF0_UnSupported                 0x0D
#define RTMP_AMF0_RecordSet                 0x0E    // reserved, not supported
#define RTMP_AMF0_XmlDocument                 0x0F
#define RTMP_AMF0_TypedObject                 0x10
// AVM+ object is the AMF3 object.
#define RTMP_AMF0_AVMplusObject             0x11
// origin array whos data takes the same form as LengthValueBytes
#define RTMP_AMF0_OriginStrictArray         0x20

// User defined
#define RTMP_AMF0_Invalid                     0x3F

class UnSortedHashtable;
class Amf0ObjectEOF;
class Amf0Object;
class Amf0EcmaArray;
class Amf0StrictArray;
class Amf0Any : public std::enable_shared_from_this<Amf0Any> {
 public:
    char marker;

 public:
    Amf0Any();
    virtual ~Amf0Any() = default;
// type identify, user should identify the type then convert from/to value.

 public:
    /**
    * whether current instance is an AMF0 string.
    * @return true if instance is an AMF0 string; otherwise, false.
    * @remark, if true, use to_string() to get its value.
    */
    virtual bool is_string();
    /**
    * whether current instance is an AMF0 boolean.
    * @return true if instance is an AMF0 boolean; otherwise, false.
    * @remark, if true, use to_boolean() to get its value.
    */
    virtual bool is_boolean();
    /**
    * whether current instance is an AMF0 number.
    * @return true if instance is an AMF0 number; otherwise, false.
    * @remark, if true, use to_number() to get its value.
    */
    virtual bool is_number();
    /**
    * whether current instance is an AMF0 null.
    * @return true if instance is an AMF0 null; otherwise, false.
    */
    virtual bool is_null();
    /**
    * whether current instance is an AMF0 undefined.
    * @return true if instance is an AMF0 undefined; otherwise, false.
    */
    virtual bool is_undefined();
    /**
    * whether current instance is an AMF0 object.
    * @return true if instance is an AMF0 object; otherwise, false.
    * @remark, if true, use to_object() to get its value.
    */
    virtual bool is_object();
    /**
    * whether current instance is an AMF0 object-EOF.
    * @return true if instance is an AMF0 object-EOF; otherwise, false.
    */
    virtual bool is_object_eof();
    /**
    * whether current instance is an AMF0 ecma-array.
    * @return true if instance is an AMF0 ecma-array; otherwise, false.
    * @remark, if true, use to_ecma_array() to get its value.
    */
    virtual bool is_ecma_array();
    /**
    * whether current instance is an AMF0 strict-array.
    * @return true if instance is an AMF0 strict-array; otherwise, false.
    * @remark, if true, use to_strict_array() to get its value.
    */
    virtual bool is_strict_array();
    /**
    * whether current instance is an AMF0 date.
    * @return true if instance is an AMF0 date; otherwise, false.
    * @remark, if true, use to_date() to get its value.
    */
    virtual bool is_date();
    /**
    * whether current instance is an AMF0 object, object-EOF, ecma-array or strict-array.
    */
    virtual bool is_complex_object();
// get value of instance

 public:
    /**
    * get a string copy of instance.
    * @remark assert is_string(), user must ensure the type then convert.
    */
    virtual std::string to_str();
    /**
    * get the raw str of instance,
    * user can directly set the content of str.
    * @remark assert is_string(), user must ensure the type then convert.
    */
    virtual const char* to_str_raw();
    /**
    * convert instance to amf0 boolean,
    * @remark assert is_boolean(), user must ensure the type then convert.
    */
    virtual bool to_boolean();
    /**
    * convert instance to amf0 number,
    * @remark assert is_number(), user must ensure the type then convert.
    */
    virtual double to_number();
    /**
    * convert instance to date,
    * @remark assert is_date(), user must ensure the type then convert.
    */
    virtual int64_t to_date();
    virtual int16_t to_date_time_zone();
    /**
    * convert instance to amf0 object,
    * @remark assert is_object(), user must ensure the type then convert.
    */
    virtual std::shared_ptr<Amf0Object> to_object();
    /**
    * convert instance to ecma array,
    * @remark assert is_ecma_array(), user must ensure the type then convert.
    */
    virtual std::shared_ptr<Amf0EcmaArray> to_ecma_array();
    /**
    * convert instance to strict array,
    * @remark assert is_strict_array(), user must ensure the type then convert.
    */
    virtual std::shared_ptr<Amf0StrictArray> to_strict_array();
// set value of instance

 public:
    /**
    * set the number of any when is_number() indicates true.
    * user must ensure the type is a number, or assert failed.
    */
    virtual void set_number(double value);
// serialize/deseriaize instance.

 public:
    /**
    * get the size of amf0 any, including the marker size.
    * the size is the bytes which instance serialized to.
    */
    virtual int total_size() = 0;
    /**
    * read AMF0 instance from stream.
    */
    virtual int read(Buffer* stream) = 0;
    /**
    * write AMF0 instance to stream.
    */
    virtual int write(Buffer* stream) = 0;
    /**
    * copy current AMF0 instance.
    */
    virtual std::shared_ptr<Amf0Any> copy() = 0;
    /**
    * human readable print 
    * @param pdata, output the heap data, NULL to ignore.
    * @return return the *pdata for print. NULL to ignore.
    * @remark user must free the data returned or output by pdata.
    */
    virtual char* human_print(char** pdata, int* psize);
// create AMF0 instance.

 public:
    /**
    * create an AMF0 string instance, set string content by value.
    */
    static std::shared_ptr<Amf0Any> str(const char* value = nullptr);
    /**
    * create an AMF0 boolean instance, set boolean content by value.
    */
    static std::shared_ptr<Amf0Any> boolean(bool value = false);
    /**
    * create an AMF0 number instance, set number content by value.
    */
    static std::shared_ptr<Amf0Any> number(double value = 0.0);
    /**
    * create an AMF0 date instance
    */
    static std::shared_ptr<Amf0Any> date(int64_t value = 0);
    /**
    * create an AMF0 null instance
    */
    static std::shared_ptr<Amf0Any> null();
    /**
    * create an AMF0 undefined instance
    */
    static std::shared_ptr<Amf0Any> undefined();
    /**
    * create an AMF0 empty object instance
    */
    static std::shared_ptr<Amf0Object> object();
    /**
    * create an AMF0 object-EOF instance
    */
    static std::shared_ptr<Amf0Any> object_eof();
    /**
    * create an AMF0 empty ecma-array instance
    */
    static std::shared_ptr<Amf0EcmaArray> ecma_array();
    /**
    * create an AMF0 empty strict-array instance
    */
    static std::shared_ptr<Amf0StrictArray> strict_array();
// discovery instance from stream

 public:
    /**
    * discovery AMF0 instance from stream
    * @param ppvalue, output the discoveried AMF0 instance.
    *       NULL if error.
    * @remark, instance is created without read from stream, user must
    *       use (*ppvalue)->read(stream) to get the instance.
    */
    static int discovery(Buffer* stream, std::shared_ptr<Amf0Any>& ppvalue);
};

class Amf0Object : public Amf0Any {
 protected:
    UnSortedHashtable* properties;
    Amf0ObjectEOF* eof;

 public:
    friend class Amf0Any;
    /**
    * make amf0 object to private,
    * use should never declare it, use Amf0Any::object() to create it.
    */
    Amf0Object();

 public:
    virtual ~Amf0Object();
// serialize/deserialize to/from stream.

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();
// properties iteration

 public:
    /**
    * clear all propergies.
    */
    virtual void clear();
    /**
    * get the count of properties(key:value).
    */
    virtual int count();
    /**
    * get the property(key:value) key at index.
    * @remark: max index is count().
    */
    virtual std::string key_at(int index);
    /**
    * get the property(key:value) key raw bytes at index.
    * user can directly set the key bytes.
    * @remark: max index is count().
    */
    virtual const char* key_raw_at(int index);
    /**
    * get the property(key:value) value at index.
    * @remark: max index is count().
    */
    virtual std::shared_ptr<Amf0Any> value_at(int index);
// property set/get.

 public:
    /**
    * set the property(key:value) of object,
    * @param key, string property name.
    * @param value, an AMF0 instance property value.
    * @remark user should never free the value, this instance will manage it.
    */
    virtual void set(std::string key, std::shared_ptr<Amf0Any> value);
    /**
    * get the property(key:value) of object,
    * @param name, the property name/key
    * @return the property AMF0 value, NULL if not found.
    * @remark user should never free the returned value, copy it if needed.
    */
    virtual std::shared_ptr<Amf0Any> get_property(std::string name);
    /**
    * get the string property, ensure the property is_string().
    * @return the property AMF0 value, NULL if not found, or not a string.
    * @remark user should never free the returned value, copy it if needed.
    */
    virtual std::shared_ptr<Amf0Any> ensure_property_string(std::string name);
    /**
    * get the number property, ensure the property is_number().
    * @return the property AMF0 value, NULL if not found, or not a number.
    * @remark user should never free the returned value, copy it if needed.
    */
    virtual std::shared_ptr<Amf0Any> ensure_property_number(std::string name);
    /**
     * remove the property specified by name.
     */
    virtual void remove(std::string name);
};

class Amf0EcmaArray : public Amf0Object {
 private:
    //  UnSortedHashtable* properties;
    //  Amf0ObjectEOF* eof;
    int32_t _count;

 public:
    friend class Amf0Any;
    /**
    * make amf0 object to private,
    * use should never declare it, use Amf0Any::ecma_array() to create it.
    */
    Amf0EcmaArray();

 public:
    virtual ~Amf0EcmaArray();
    // serialize/deserialize to/from stream.

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();    //  */
    // properties iteration

/*
 public:
    virtual void clear();
    virtual int count();
    virtual std::string key_at(int index);
    virtual const char* key_raw_at(int index);
    virtual std::shared_ptr<Amf0Any> value_at(int index);
    // property set/get.

 public:
    virtual void set(std::string key, std::shared_ptr<Amf0Any> value);
    virtual std::shared_ptr<Amf0Any> get_property(std::string name);
    virtual std::shared_ptr<Amf0Any> ensure_property_string(std::string name);
    virtual std::shared_ptr<Amf0Any> ensure_property_number(std::string name);  //  */
};

/**
* 2.12 Strict Array Type
* array-count = U32 
* strict-array-type = array-count *(value-type)
*/
class Amf0StrictArray : public Amf0Any {
 private:
    std::vector<std::shared_ptr<Amf0Any>> properties;
    int32_t _count;

 public:
    friend class Amf0Any;
    /**
    * make amf0 object to private,
    * use should never declare it, use Amf0Any::strict_array() to create it.
    */
    Amf0StrictArray();

 public:
    virtual ~Amf0StrictArray();
    // serialize/deserialize to/from stream.

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();
    // properties iteration

 public:
    /**
    * clear all elements.
    */
    virtual void clear();
    /**
    * get the count of elements
    */
    virtual int count();
    /**
    * get the elements key at index.
    * @remark: max index is count().
    */
    virtual std::shared_ptr<Amf0Any> at(int index);
    // property set/get.

 public:
    /**
    * append new element to array
    * @param any, an AMF0 instance property value.
    * @remark user should never free the any, this instance will manage it.
    */
    virtual void append(std::shared_ptr<Amf0Any> any);
};

/**
* the class to get amf0 object size
*/
class Amf0Size {
 public:
    static int utf8(std::string value);
    static int str(std::string value);
    static int number();
    static int date();
    static int null();
    static int undefined();
    static int boolean();
    static int object(std::shared_ptr<Amf0Object> obj);
    static int object_eof();
    static int ecma_array(std::shared_ptr<Amf0EcmaArray> arr);
    static int strict_array(std::shared_ptr<Amf0StrictArray> arr);
    static int any(std::shared_ptr<Amf0Any> o);
};

/**
* read amf0 string from stream.
* 2.4 String Type
* string-type = string-marker UTF-8
* @return default value is empty string.
* @remark: use Amf0Any::str() to create it.
*/
class Amf0String : public Amf0Any {
 public:
    std::string value;

 public:
    friend class Amf0Any;
    /**
    * make amf0 string to private,
    * use should never declare it, use Amf0Any::str() to create it.
    */
    explicit Amf0String(const char* _value);

 public:
    virtual ~Amf0String();

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();
};

/**
* read amf0 boolean from stream.
* 2.4 String Type
* boolean-type = boolean-marker U8
*         0 is false, <> 0 is true
* @return default value is false.
*/
class Amf0Boolean : public Amf0Any {
 public:
    bool value;

 public:
    friend class Amf0Any;
    /**
    * make amf0 boolean to private,
    * use should never declare it, use Amf0Any::boolean() to create it.
    */
    explicit Amf0Boolean(bool _value);

 public:
    virtual ~Amf0Boolean();

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();
};

/**
* read amf0 number from stream.
* 2.2 Number Type
* number-type = number-marker DOUBLE
* @return default value is 0.
*/
class Amf0Number : public Amf0Any {
 public:
    double value;

 public:
    friend class Amf0Any;
    /**
    * make amf0 number to private,
    * use should never declare it, use Amf0Any::number() to create it.
    */
    explicit Amf0Number(double _value);

 public:
    virtual ~Amf0Number();

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();
};

/**
* 2.13 Date Type
* time-zone = S16 ; reserved, not supported should be set to 0x0000
* date-type = date-marker DOUBLE time-zone
* @see: https://github.com/ossrs/srs/issues/185
*/
class Amf0Date : public Amf0Any {
 private:
    int64_t _date_value;
    int16_t _time_zone;

 public:
    friend class Amf0Any;
    /**
    * make amf0 date to private,
    * use should never declare it, use Amf0Any::date() to create it.
    */
    explicit Amf0Date(int64_t value);

 public:
    virtual ~Amf0Date();
    // serialize/deserialize to/from stream.

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();

 public:
    /**
    * get the date value.
    */
    virtual int64_t date();
    /**
    * get the time_zone.
    */
    virtual int16_t time_zone();
};

/**
* read amf0 null from stream.
* 2.7 null Type
* null-type = null-marker
*/
class Amf0Null : public Amf0Any {
 public:
    friend class Amf0Any;
    /**
    * make amf0 null to private,
    * use should never declare it, use Amf0Any::null() to create it.
    */
    Amf0Null();

 public:
    virtual ~Amf0Null();

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();
};

/**
* read amf0 undefined from stream.
* 2.8 undefined Type
* undefined-type = undefined-marker
*/
class Amf0Undefined : public Amf0Any {
 private:
    friend class Amf0Any;

 public:
    /**
    * make amf0 undefined to private,
    * use should never declare it, use Amf0Any::undefined() to create it.
    */
    Amf0Undefined();
    virtual ~Amf0Undefined();

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();
};

class UnSortedHashtable {
 private:
    typedef std::pair<std::string, std::shared_ptr<Amf0Any>> Amf0ObjectPropertyType;
    std::vector<Amf0ObjectPropertyType> properties;

 public:
    UnSortedHashtable();
    virtual ~UnSortedHashtable();

 public:
    virtual int count();
    virtual void clear();
    virtual std::string key_at(int index);
    virtual const char* key_raw_at(int index);
    virtual std::shared_ptr<Amf0Any> value_at(int index);
    /**
    * set the value of hashtable.
    * @param value, the value to set. NULL to delete the property.
    */
    virtual void set(std::string key, std::shared_ptr<Amf0Any> value);

 public:
    virtual std::shared_ptr<Amf0Any> get_property(std::string name);
    virtual std::shared_ptr<Amf0Any> ensure_property_string(std::string name);
    virtual std::shared_ptr<Amf0Any> ensure_property_number(std::string name);
    virtual void remove(std::string name);

 public:
    virtual void copy(UnSortedHashtable* src);
};

class Amf0ObjectEOF : public Amf0Any {
 public:
    Amf0ObjectEOF();
    virtual ~Amf0ObjectEOF();

 public:
    virtual int total_size();
    virtual int read(Buffer* stream);
    virtual int write(Buffer* stream);
    virtual std::shared_ptr<Amf0Any> copy();
};

/**
* read anything from stream.
* @param ppvalue, the output amf0 any elem.
*         NULL if error; otherwise, never NULL and user must free it.
*/
extern int amf0_read_any(Buffer* stream, std::shared_ptr<Amf0Any>& ppvalue);

/**
* read amf0 string from stream.
* 2.4 String Type
* string-type = string-marker UTF-8
*/
extern int amf0_read_string(Buffer* stream, std::string& value);
extern int amf0_write_string(Buffer* stream, std::string value);

/**
* read amf0 boolean from stream.
* 2.4 String Type
* boolean-type = boolean-marker U8
*         0 is false, <> 0 is true
*/
extern int amf0_read_boolean(Buffer* stream, bool& value);
extern int amf0_write_boolean(Buffer* stream, bool value);

/**
* read amf0 number from stream.
* 2.2 Number Type
* number-type = number-marker DOUBLE
*/
extern int amf0_read_number(Buffer* stream, double& value);
extern int amf0_write_number(Buffer* stream, double value);

/**
* read amf0 null from stream.
* 2.7 null Type
* null-type = null-marker
*/
extern int amf0_read_null(Buffer* stream);
extern int amf0_write_null(Buffer* stream);

/**
* read amf0 undefined from stream.
* 2.8 undefined Type
* undefined-type = undefined-marker
*/
extern int amf0_read_undefined(Buffer* stream);
extern int amf0_write_undefined(Buffer* stream);

/**
* read amf0 utf8 string from stream.
* 1.3.1 Strings and UTF-8
* UTF-8 = U16 *(UTF8-char)
* UTF8-char = UTF8-1 | UTF8-2 | UTF8-3 | UTF8-4
* UTF8-1 = %x00-7F
* @remark only support UTF8-1 char.
*/
extern int amf0_read_utf8(Buffer* stream, std::string& value);
extern int amf0_write_utf8(Buffer* stream, std::string value);

extern bool amf0_is_object_eof(Buffer* stream);
extern int amf0_write_object_eof(Buffer* stream, std::shared_ptr<Amf0ObjectEOF> value);

extern int amf0_write_any(Buffer* stream, std::shared_ptr<Amf0Any> value);

}  // namespace tmss

