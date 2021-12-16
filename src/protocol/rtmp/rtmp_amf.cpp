/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include <rtmp_amf.hpp>
#include <utility>
#include <vector>
#include <sstream>
#include <defs/err.hpp>
#include <log/log.hpp>
#include <util/util.hpp>

namespace tmss {
void fill_level_spaces(std::stringstream& ss, int level) {
    for (int i = 0; i < level; i++) {
        ss << "    ";
    }
}
void amf0_do_print(std::shared_ptr<Amf0Any> any, std::stringstream& ss, int level) {
    if (any->is_boolean()) {
        ss << "Boolean " << (any->to_boolean()? "true":"false") << std::endl;
    } else if (any->is_number()) {
        ss << "Number " << std::fixed << any->to_number() << std::endl;
    } else if (any->is_string()) {
        ss << "String " << any->to_str() << std::endl;
    } else if (any->is_date()) {
        ss << "Date " << std::hex << any->to_date()
            << "/" << std::hex << any->to_date_time_zone() << std::endl;
    } else if (any->is_null()) {
        ss << "Null" << std::endl;
    } else if (any->is_ecma_array()) {
        std::shared_ptr<Amf0EcmaArray> obj = any->to_ecma_array();
        ss << "EcmaArray " << "(" << obj->count() << " items)" << std::endl;
        for (int i = 0; i < obj->count(); i++) {
            fill_level_spaces(ss, level + 1);
            ss << "Elem '" << obj->key_at(i) << "' ";
            if (obj->value_at(i)->is_complex_object()) {
                amf0_do_print(obj->value_at(i), ss, level + 1);
            } else {
                amf0_do_print(obj->value_at(i), ss, 0);
            }
        }
    } else if (any->is_strict_array()) {
        std::shared_ptr<Amf0StrictArray> obj = any->to_strict_array();
        ss << "StrictArray " << "(" << obj->count() << " items)" << std::endl;
        for (int i = 0; i < obj->count(); i++) {
            fill_level_spaces(ss, level + 1);
            ss << "Elem ";
            if (obj->at(i)->is_complex_object()) {
                amf0_do_print(obj->at(i), ss, level + 1);
            } else {
                amf0_do_print(obj->at(i), ss, 0);
            }
        }
    } else if (any->is_object()) {
        std::shared_ptr<Amf0Object> obj = any->to_object();
        ss << "Object " << "(" << obj->count() << " items)" << std::endl;
        for (int i = 0; i < obj->count(); i++) {
            fill_level_spaces(ss, level + 1);
            ss << "Property '" << obj->key_at(i) << "' ";
            if (obj->value_at(i)->is_complex_object()) {
                amf0_do_print(obj->value_at(i), ss, level + 1);
            } else {
                amf0_do_print(obj->value_at(i), ss, 0);
            }
        }
    } else {
        ss << "Unknown" << std::endl;
    }
}

Amf0Any::Amf0Any() {
    marker = RTMP_AMF0_Invalid;
}

bool Amf0Any::is_string() {
    return marker == RTMP_AMF0_String;
}

bool Amf0Any::is_boolean() {
    return marker == RTMP_AMF0_Boolean;
}

bool Amf0Any::is_number() {
    return marker == RTMP_AMF0_Number;
}

bool Amf0Any::is_null() {
    return marker == RTMP_AMF0_Null;
}

bool Amf0Any::is_undefined() {
    return marker == RTMP_AMF0_Undefined;
}

bool Amf0Any::is_object() {
    return marker == RTMP_AMF0_Object;
}

bool Amf0Any::is_ecma_array() {
    return marker == RTMP_AMF0_EcmaArray;
}

bool Amf0Any::is_strict_array() {
    return marker == RTMP_AMF0_StrictArray;
}

bool Amf0Any::is_date() {
    return marker == RTMP_AMF0_Date;
}

bool Amf0Any::is_complex_object() {
    return is_object() || is_object_eof() || is_ecma_array() || is_strict_array();
}

std::string Amf0Any::to_str() {
    std::shared_ptr<Amf0String> p = std::dynamic_pointer_cast<Amf0String>(shared_from_this());
    assert(p != NULL);
    return p->value;
}

const char* Amf0Any::to_str_raw() {
    std::shared_ptr<Amf0String> p = std::dynamic_pointer_cast<Amf0String>(shared_from_this());
    assert(p != NULL);
    return p->value.data();
}

bool Amf0Any::to_boolean() {
    std::shared_ptr<Amf0Boolean> p = std::dynamic_pointer_cast<Amf0Boolean>(shared_from_this());
    assert(p != NULL);
    return p->value;
}

double Amf0Any::to_number() {
    std::shared_ptr<Amf0Number> p = std::dynamic_pointer_cast<Amf0Number>(shared_from_this());
    assert(p != NULL);
    return p->value;
}

int64_t Amf0Any::to_date() {
    std::shared_ptr<Amf0Date> p = std::dynamic_pointer_cast<Amf0Date>(shared_from_this());
    assert(p != NULL);
    return p->date();
}

int16_t Amf0Any::to_date_time_zone() {
    std::shared_ptr<Amf0Date> p = std::dynamic_pointer_cast<Amf0Date>(shared_from_this());
    assert(p != NULL);
    return p->time_zone();
}

std::shared_ptr<Amf0Object> Amf0Any::to_object() {
    std::shared_ptr<Amf0Object> p = std::dynamic_pointer_cast<Amf0Object>(shared_from_this());
    assert(p != NULL);
    return p;
}

std::shared_ptr<Amf0EcmaArray> Amf0Any::to_ecma_array() {
    std::shared_ptr<Amf0EcmaArray> p = std::dynamic_pointer_cast<Amf0EcmaArray>(shared_from_this());
    assert(p != NULL);
    return p;
}

std::shared_ptr<Amf0StrictArray> Amf0Any::to_strict_array() {
    std::shared_ptr<Amf0StrictArray> p = std::dynamic_pointer_cast<Amf0StrictArray>(shared_from_this());
    assert(p != NULL);
    return p;
}

void Amf0Any::set_number(double value) {
    std::shared_ptr<Amf0Number> p = std::dynamic_pointer_cast<Amf0Number>(shared_from_this());
    assert(p != NULL);
    p->value = value;
}

bool Amf0Any::is_object_eof() {
    return marker == RTMP_AMF0_ObjectEnd;
}

char* Amf0Any::human_print(char** pdata, int* psize) {
    std::stringstream ss;

    ss.precision(1);

    amf0_do_print(shared_from_this(), ss, 0);

    std::string str = ss.str();
    if (str.empty()) {
        return NULL;
    }

    char* data = new char[str.length() + 1];
    memcpy(data, str.data(), str.length());
    data[str.length()] = 0;

    if (pdata) {
        *pdata = data;
    }
    if (psize) {
        *psize = str.length();
    }

    return data;
}

std::shared_ptr<Amf0Any> Amf0Any::str(const char* value) {
    return std::make_shared<Amf0String>(value);
}

std::shared_ptr<Amf0Any> Amf0Any::boolean(bool value) {
    return std::make_shared<Amf0Boolean>(value);
}

std::shared_ptr<Amf0Any> Amf0Any::number(double value) {
    return std::make_shared<Amf0Number>(value);
}

std::shared_ptr<Amf0Any> Amf0Any::null() {
    return std::make_shared<Amf0Null>();
}

std::shared_ptr<Amf0Any> Amf0Any::undefined() {
    return std::make_shared<Amf0Undefined>();
}

std::shared_ptr<Amf0Object> Amf0Any::object() {
    return std::make_shared<Amf0Object>();
}

std::shared_ptr<Amf0Any> Amf0Any::object_eof() {
    return std::make_shared<Amf0ObjectEOF>();
}

std::shared_ptr<Amf0EcmaArray> Amf0Any::ecma_array() {
    return std::make_shared<Amf0EcmaArray>();
}

std::shared_ptr<Amf0StrictArray> Amf0Any::strict_array() {
    return std::make_shared<Amf0StrictArray>();
}

std::shared_ptr<Amf0Any> Amf0Any::date(int64_t value) {
    return std::make_shared<Amf0Date>(value);
}

int Amf0Any::discovery(Buffer* stream, std::shared_ptr<Amf0Any>& ppvalue) {
    int ret = error_success;

    // detect the object-eof specially
    if (amf0_is_object_eof(stream)) {
        ppvalue = std::make_shared<Amf0ObjectEOF>();
        return ret;
    }

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read any marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    tmss_info("amf0 any marker success");

    // backward the 1byte marker.
    stream->seek_read(-1);

    switch (marker) {
        case RTMP_AMF0_String: {
            ppvalue = Amf0Any::str();
            return ret;
        }
        case RTMP_AMF0_Boolean: {
            ppvalue = Amf0Any::boolean();
            return ret;
        }
        case RTMP_AMF0_Number: {
            ppvalue = Amf0Any::number();
            return ret;
        }
        case RTMP_AMF0_Null: {
            ppvalue = Amf0Any::null();
            return ret;
        }
        case RTMP_AMF0_Undefined: {
            ppvalue = Amf0Any::undefined();
            return ret;
        }
        case RTMP_AMF0_Object: {
            ppvalue = Amf0Any::object();
            return ret;
        }
        case RTMP_AMF0_EcmaArray: {
            ppvalue = Amf0Any::ecma_array();
            return ret;
        }
        case RTMP_AMF0_StrictArray: {
            ppvalue = Amf0Any::strict_array();
            return ret;
        }
        case RTMP_AMF0_Date: {
            ppvalue = Amf0Any::date();
            return ret;
        }
        case RTMP_AMF0_Invalid:
        default: {
            ret = error_rtmp_amf0_invalid;
            tmss_error("invalid amf0 message type. marker=%#x, ret={}", marker, ret);
            return ret;
        }
    }
}

Amf0Object::Amf0Object() {
    properties = new UnSortedHashtable();
    eof = new Amf0ObjectEOF();
    marker = RTMP_AMF0_Object;
}

Amf0Object::~Amf0Object() {
    freep(properties);
    freep(eof);
}

int Amf0Object::total_size() {
    int size = 1;

    for (int i = 0; i < properties->count(); i++) {
        std::string name = key_at(i);
        std::shared_ptr<Amf0Any> value = value_at(i);
        size += Amf0Size::utf8(name);
        size += Amf0Size::any(value);
    }

    size += Amf0Size::object_eof();

    return size;
}

int Amf0Object::read(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read object marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_Object) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check object marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_Object, ret);
        return ret;
    }
    tmss_info("amf0 read object marker success");

    // value
    while (!stream->read_empty()) {
        // detect whether is eof.
        if (amf0_is_object_eof(stream)) {
            Amf0ObjectEOF pbj_eof;
            if ((ret = pbj_eof.read(stream)) != error_success) {
                tmss_error("amf0 object read eof failed. ret={}", ret);
                return ret;
            }
            tmss_info("amf0 read object EOF.");
            break;
        }

        // property-name: utf8 string
        std::string property_name;
        if ((ret = amf0_read_utf8(stream, property_name)) != error_success) {
            tmss_error("amf0 object read property name failed. ret={}", ret);
            return ret;
        }
        // property-value: any
        std::shared_ptr<Amf0Any> property_value;
        if ((ret = amf0_read_any(stream, property_value)) != error_success) {
            tmss_error("amf0 object read property_value failed. "
                "name={}, ret={}", property_name.c_str(), ret);
            //  freep(property_value);
            return ret;
        }

        // add property
        this->set(property_name, property_value);
    }

    return ret;
}

int Amf0Object::write(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write object marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_Object);
    tmss_info("amf0 write object marker success");

    // value
    for (int i = 0; i < properties->count(); i++) {
        std::string name = this->key_at(i);
        std::shared_ptr<Amf0Any> any = this->value_at(i);

        if ((ret = amf0_write_utf8(stream, name)) != error_success) {
            tmss_error("write object property name failed. ret={}", ret);
            return ret;
        }

        if ((ret = amf0_write_any(stream, any)) != error_success) {
            tmss_error("write object property value failed. ret={}", ret);
            return ret;
        }

        tmss_info("write amf0 property success. name={}", name.c_str());
    }

    if ((ret = eof->write(stream)) != error_success) {
        tmss_error("write object eof failed. ret={}", ret);
        return ret;
    }

    tmss_info("write amf0 object success.");

    return ret;
}

std::shared_ptr<Amf0Any> Amf0Object::copy() {
    std::shared_ptr<Amf0Object> copy = std::make_shared<Amf0Object>();
    copy->properties->copy(properties);
    return copy;
}

void Amf0Object::clear() {
    properties->clear();
}

int Amf0Object::count() {
    return properties->count();
}

std::string Amf0Object::key_at(int index) {
    return properties->key_at(index);
}

const char* Amf0Object::key_raw_at(int index) {
    return properties->key_raw_at(index);
}

std::shared_ptr<Amf0Any> Amf0Object::value_at(int index) {
    return properties->value_at(index);
}

void Amf0Object::set(std::string key, std::shared_ptr<Amf0Any> value) {
    properties->set(key, value);
}

std::shared_ptr<Amf0Any> Amf0Object::get_property(std::string name) {
    return properties->get_property(name);
}

std::shared_ptr<Amf0Any> Amf0Object::ensure_property_string(std::string name) {
    return properties->ensure_property_string(name);
}

std::shared_ptr<Amf0Any> Amf0Object::ensure_property_number(std::string name) {
    return properties->ensure_property_number(name);
}

void Amf0Object::remove(std::string name) {
    properties->remove(name);
}

Amf0EcmaArray::Amf0EcmaArray() {
    _count = 0;
    properties = new UnSortedHashtable();
    eof = new Amf0ObjectEOF();
    marker = RTMP_AMF0_EcmaArray;
}

Amf0EcmaArray::~Amf0EcmaArray() {
    freep(properties);
    freep(eof);
}

int Amf0EcmaArray::total_size() {
    int size = 1 + 4;
    for (int i = 0; i < properties->count(); i++) {
        std::string name = key_at(i);
        std::shared_ptr<Amf0Any> value = value_at(i);
        size += Amf0Size::utf8(name);
        size += Amf0Size::any(value);
    }
    size += Amf0Size::object_eof();

    return size;
}

int Amf0EcmaArray::read(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read ecma_array marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_EcmaArray) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check ecma_array marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_EcmaArray, ret);
        return ret;
    }
    tmss_info("amf0 read ecma_array marker success");

    // count
    if (!stream->read_require(4)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read ecma_array count failed. ret={}", ret);
        return ret;
    }

    int32_t count = stream->read_4bytes();
    tmss_info("amf0 read ecma_array count success. count={}", count);

    // value
    this->_count = count;

    while (!stream->read_empty() && this->count() < count) {
        // detect whether is eof.
        if (amf0_is_object_eof(stream)) {
            Amf0ObjectEOF pbj_eof;
            if ((ret = pbj_eof.read(stream)) != error_success) {
                tmss_error("amf0 ecma_array read eof failed. ret={}", ret);
                return ret;
            }
            tmss_info("amf0 read ecma_array EOF.");
            break;
        }

        // property-name: utf8 string
        std::string property_name;
        if ((ret =amf0_read_utf8(stream, property_name)) != error_success) {
            tmss_error("amf0 ecma_array read property name failed. ret={}", ret);
            return ret;
        }
        // property-value: any
        std::shared_ptr<Amf0Any> property_value;
        if ((ret = amf0_read_any(stream, property_value)) != error_success) {
            tmss_error("amf0 ecma_array read property_value failed. "
                "name={}, ret={}", property_name.c_str(), ret);
            return ret;
        }

        tmss_info("name={}", property_name.c_str());
        // add property
        this->set(property_name, property_value);
    }

    return ret;
}
int Amf0EcmaArray::write(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write ecma_array marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_EcmaArray);
    tmss_info("amf0 write ecma_array marker success");

    // count
    if (!stream->read_require(4)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write ecma_array count failed. ret={}", ret);
        return ret;
    }

    stream->write_4bytes(this->_count);
    tmss_info("amf0 write ecma_array count success. count={}", _count);

    // value
    for (int i = 0; i < properties->count(); i++) {
        std::string name = this->key_at(i);
        std::shared_ptr<Amf0Any> any = this->value_at(i);

        if ((ret = amf0_write_utf8(stream, name)) != error_success) {
            tmss_error("write ecma_array property name failed. ret={}", ret);
            return ret;
        }

        if ((ret = amf0_write_any(stream, any)) != error_success) {
            tmss_error("write ecma_array property value failed. ret={}", ret);
            return ret;
        }

        tmss_info("write amf0 property success. name={}", name.c_str());
    }

    if ((ret = eof->write(stream)) != error_success) {
        tmss_error("write ecma_array eof failed. ret={}", ret);
        return ret;
    }

    tmss_info("write ecma_array object success.");

    return ret;
}

std::shared_ptr<Amf0Any> Amf0EcmaArray::copy() {
    std::shared_ptr<Amf0EcmaArray> copy = std::make_shared<Amf0EcmaArray>();
    copy->properties->copy(properties);
    copy->_count = _count;
    return copy;
}

/*void Amf0EcmaArray::clear() {
    properties->clear();
}

int Amf0EcmaArray::count() {
    return properties->count();
}

std::string Amf0EcmaArray::key_at(int index) {
    return properties->key_at(index);
}

const char* Amf0EcmaArray::key_raw_at(int index) {
    return properties->key_raw_at(index);
}

std::shared_ptr<Amf0Any> Amf0EcmaArray::value_at(int index) {
    return properties->value_at(index);
}

void Amf0EcmaArray::set(std::string key, std::shared_ptr<Amf0Any> value) {
    properties->set(key, value);
}

std::shared_ptr<Amf0Any> Amf0EcmaArray::get_property(std::string name) {
    return properties->get_property(name);
}

std::shared_ptr<Amf0Any> Amf0EcmaArray::ensure_property_string(std::string name) {
    return properties->ensure_property_string(name);
}

std::shared_ptr<Amf0Any> Amf0EcmaArray::ensure_property_number(std::string name) {
    return properties->ensure_property_number(name);
}
//  */
Amf0StrictArray::Amf0StrictArray() {
    marker = RTMP_AMF0_StrictArray;
    _count = 0;
}

Amf0StrictArray::~Amf0StrictArray() {
    std::vector<std::shared_ptr<Amf0Any>>::iterator it;
    for (it = properties.begin(); it != properties.end(); ++it) {
        std::shared_ptr<Amf0Any> any = *it;
        //  freep(any);
    }
    properties.clear();
}

int Amf0StrictArray::total_size() {
    int size = 1 + 4;

    for (int i = 0; i < static_cast<int>(properties.size()); i++) {
        std::shared_ptr<Amf0Any> any = properties[i];
        size += any->total_size();
    }

    return size;
}

int Amf0StrictArray::read(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read strict_array marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_StrictArray) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check strict_array marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_StrictArray, ret);
        return ret;
    }
    tmss_info("amf0 read strict_array marker success");

    // count
    if (!stream->read_require(4)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read strict_array count failed. ret={}", ret);
        return ret;
    }

    int32_t count = stream->read_4bytes();
    tmss_info("amf0 read strict_array count success. count={}", count);

    // value
    this->_count = count;

    for (int i = 0; i < count && !stream->read_empty(); i++) {
        // property-value: any
        std::shared_ptr<Amf0Any> elem;
        if ((ret = amf0_read_any(stream, elem)) != error_success) {
            tmss_error("amf0 strict_array read value failed. ret={}", ret);
            return ret;
        }

        // add property
        properties.push_back(elem);
    }

    return ret;
}
int Amf0StrictArray::write(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write strict_array marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_StrictArray);
    tmss_info("amf0 write strict_array marker success");

    // count
    if (!stream->read_require(4)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write strict_array count failed. ret={}", ret);
        return ret;
    }

    stream->write_4bytes(this->_count);
    tmss_info("amf0 write strict_array count success. count={}", _count);

    // value
    for (int i = 0; i < static_cast<int>(properties.size()); i++) {
        std::shared_ptr<Amf0Any> any = properties[i];

        if ((ret = amf0_write_any(stream, any)) != error_success) {
            tmss_error("write strict_array property value failed. ret={}", ret);
            return ret;
        }

        tmss_info("write amf0 property success.");
    }

    tmss_info("write strict_array object success.");

    return ret;
}

std::shared_ptr<Amf0Any> Amf0StrictArray::copy() {
    std::shared_ptr<Amf0StrictArray> copy = std::make_shared<Amf0StrictArray>();

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        std::shared_ptr<Amf0Any> any = *it;
        copy->append(any->copy());
    }

    copy->_count = _count;
    return copy;
}

void Amf0StrictArray::clear() {
    properties.clear();
}

int Amf0StrictArray::count() {
    return properties.size();
}

std::shared_ptr<Amf0Any> Amf0StrictArray::at(int index) {
    assert(index < static_cast<int>(properties.size()));
    return properties.at(index);
}

void Amf0StrictArray::append(std::shared_ptr<Amf0Any> any) {
    properties.push_back(any);
    _count = static_cast<int32_t>(properties.size());
}

int Amf0Size::utf8(std::string value) {
    return 2 + value.length();
}

int Amf0Size::str(std::string value) {
    return 1 + Amf0Size::utf8(value);
}

int Amf0Size::number() {
    return 1 + 8;
}

int Amf0Size::date() {
    return 1 + 8 + 2;
}

int Amf0Size::null() {
    return 1;
}

int Amf0Size::undefined() {
    return 1;
}

int Amf0Size::boolean() {
    return 1 + 1;
}

int Amf0Size::object(std::shared_ptr<Amf0Object> obj) {
    if (!obj) {
        return 0;
    }

    return obj->total_size();
}

int Amf0Size::object_eof() {
    return 2 + 1;
}

int Amf0Size::ecma_array(std::shared_ptr<Amf0EcmaArray> arr) {
    if (!arr) {
        return 0;
    }

    return arr->total_size();
}

int Amf0Size::strict_array(std::shared_ptr<Amf0StrictArray> arr) {
    if (!arr) {
        return 0;
    }

    return arr->total_size();
}

int Amf0Size::any(std::shared_ptr<Amf0Any> o) {
    if (!o) {
        return 0;
    }

    return o->total_size();
}

Amf0String::Amf0String(const char* _value) {
    marker = RTMP_AMF0_String;
    if (_value) {
        value = _value;
    }
}

Amf0String::~Amf0String() {
}

int Amf0String::total_size() {
    return Amf0Size::str(value);
}

int Amf0String::read(Buffer* stream) {
    return amf0_read_string(stream, value);
}

int Amf0String::write(Buffer* stream) {
    return amf0_write_string(stream, value);
}

std::shared_ptr<Amf0Any> Amf0String::copy() {
    std::shared_ptr<Amf0String> copy = std::make_shared<Amf0String>(value.c_str());
    return copy;
}

Amf0Boolean::Amf0Boolean(bool _value) {
    marker = RTMP_AMF0_Boolean;
    value = _value;
}

Amf0Boolean::~Amf0Boolean() {
}

int Amf0Boolean::total_size() {
    return Amf0Size::boolean();
}

int Amf0Boolean::read(Buffer* stream) {
    return amf0_read_boolean(stream, value);
}

int Amf0Boolean::write(Buffer* stream) {
    return amf0_write_boolean(stream, value);
}

std::shared_ptr<Amf0Any> Amf0Boolean::copy() {
    std::shared_ptr<Amf0Boolean> copy = std::make_shared<Amf0Boolean>(value);
    return copy;
}

Amf0Number::Amf0Number(double _value) {
    marker = RTMP_AMF0_Number;
    value = _value;
}

Amf0Number::~Amf0Number() {
}

int Amf0Number::total_size() {
    return Amf0Size::number();
}

int Amf0Number::read(Buffer* stream) {
    return amf0_read_number(stream, value);
}

int Amf0Number::write(Buffer* stream) {
    return amf0_write_number(stream, value);
}

std::shared_ptr<Amf0Any> Amf0Number::copy() {
    std::shared_ptr<Amf0Number> copy = std::make_shared<Amf0Number>(value);
    return copy;
}

Amf0Date::Amf0Date(int64_t value) {
    marker = RTMP_AMF0_Date;
    _date_value = value;
    _time_zone = 0;
}

Amf0Date::~Amf0Date() {
}

int Amf0Date::total_size() {
    return Amf0Size::date();
}

int Amf0Date::read(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read date marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_Date) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check date marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_Date, ret);
        return ret;
    }
    tmss_info("amf0 read date marker success");

    // date value
    // An ActionScript Date is serialized as the number of milliseconds
    // elapsed since the epoch of midnight on 1st Jan 1970 in the UTC
    // time zone.
    if (!stream->read_require(8)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read date failed. ret={}", ret);
        return ret;
    }

    _date_value = stream->read_8bytes();
    tmss_info("amf0 read date success. date={}" , _date_value);

    // time zone
    // While the design of this type reserves room for time zone offset
    // information, it should not be filled in, nor used, as it is unconventional
    // to change time zones when serializing dates on a network. It is suggested
    // that the time zone be queried independently as needed.
    if (!stream->read_require(2)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read time zone failed. ret={}", ret);
        return ret;
    }

    _time_zone = stream->read_2bytes();
    tmss_info("amf0 read time zone success. zone={}", _time_zone);

    return ret;
}
int Amf0Date::write(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write date marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_Date);
    tmss_info("amf0 write date marker success");

    // date value
    if (!stream->read_require(8)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write date failed. ret={}", ret);
        return ret;
    }

    stream->write_8bytes(_date_value);
    tmss_info("amf0 write date success. date={}", _date_value);

    // time zone
    if (!stream->read_require(2)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write time zone failed. ret={}", ret);
        return ret;
    }

    stream->write_2bytes(_time_zone);
    tmss_info("amf0 write time zone success. date={}", _time_zone);

    tmss_info("write date object success.");

    return ret;
}

std::shared_ptr<Amf0Any> Amf0Date::copy() {
    std::shared_ptr<Amf0Date> copy = std::make_shared<Amf0Date>(0);

    copy->_date_value = _date_value;
    copy->_time_zone = _time_zone;

    return copy;
}

int64_t Amf0Date::date() {
    return _date_value;
}

int16_t Amf0Date::time_zone() {
    return _time_zone;
}

Amf0Null::Amf0Null() {
    marker = RTMP_AMF0_Null;
}

Amf0Null::~Amf0Null() {
}

int Amf0Null::total_size() {
    return Amf0Size::null();
}

int Amf0Null::read(Buffer* stream) {
    return amf0_read_null(stream);
}

int Amf0Null::write(Buffer* stream) {
    return amf0_write_null(stream);
}

std::shared_ptr<Amf0Any> Amf0Null::copy() {
    std::shared_ptr<Amf0Null> copy = std::make_shared<Amf0Null>();
    return copy;
}

Amf0Undefined::Amf0Undefined() {
    marker = RTMP_AMF0_Undefined;
}

Amf0Undefined::~Amf0Undefined() {
}

int Amf0Undefined::total_size() {
    return Amf0Size::undefined();
}

int Amf0Undefined::read(Buffer* stream) {
    return amf0_read_undefined(stream);
}

int Amf0Undefined::write(Buffer* stream) {
    return amf0_write_undefined(stream);
}

std::shared_ptr<Amf0Any> Amf0Undefined::copy() {
    std::shared_ptr<Amf0Undefined> copy = std::make_shared<Amf0Undefined>();
    return copy;
}

UnSortedHashtable::UnSortedHashtable() {
}

UnSortedHashtable::~UnSortedHashtable() {
    clear();
}

int UnSortedHashtable::count() {
    return static_cast<int>(properties.size());
}

void UnSortedHashtable::clear() {
    std::vector<Amf0ObjectPropertyType>::iterator it;
    for (it = properties.begin(); it != properties.end(); ++it) {
        Amf0ObjectPropertyType& elem = *it;
        std::shared_ptr<Amf0Any> any = elem.second;
        //  freep(any);
    }
    properties.clear();
}

std::string UnSortedHashtable::key_at(int index) {
    assert(index < count());
    Amf0ObjectPropertyType& elem = properties[index];
    return elem.first;
}

const char* UnSortedHashtable::key_raw_at(int index) {
    assert(index < count());
    Amf0ObjectPropertyType& elem = properties[index];
    return elem.first.data();
}

std::shared_ptr<Amf0Any> UnSortedHashtable::value_at(int index) {
    assert(index < count());
    Amf0ObjectPropertyType& elem = properties[index];
    return elem.second;
}

void UnSortedHashtable::set(std::string key, std::shared_ptr<Amf0Any> value) {
    std::vector<Amf0ObjectPropertyType>::iterator it;

    for (it = properties.begin(); it != properties.end(); ++it) {
        Amf0ObjectPropertyType& elem = *it;
        std::string name = elem.first;
        std::shared_ptr<Amf0Any> any = elem.second;

        if (key == name) {
            //  freep(any);
            properties.erase(it);
            break;
        }
    }

    if (value) {
        properties.push_back(std::make_pair(key, value));
    }
}

std::shared_ptr<Amf0Any> UnSortedHashtable::get_property(std::string name) {
    std::vector<Amf0ObjectPropertyType>::iterator it;

    for (it = properties.begin(); it != properties.end(); ++it) {
        Amf0ObjectPropertyType& elem = *it;
        std::string key = elem.first;
        std::shared_ptr<Amf0Any> any = elem.second;
        if (key == name) {
            return any;
        }
    }

    return NULL;
}

std::shared_ptr<Amf0Any> UnSortedHashtable::ensure_property_string(std::string name) {
    std::shared_ptr<Amf0Any> prop = get_property(name);

    if (!prop) {
        return NULL;
    }

    if (!prop->is_string()) {
        return NULL;
    }

    return prop;
}

std::shared_ptr<Amf0Any> UnSortedHashtable::ensure_property_number(std::string name) {
    std::shared_ptr<Amf0Any> prop = get_property(name);

    if (!prop) {
        return NULL;
    }

    if (!prop->is_number()) {
        return NULL;
    }

    return prop;
}

void UnSortedHashtable::remove(std::string name) {
    std::vector<Amf0ObjectPropertyType>::iterator it;

    for (it = properties.begin(); it != properties.end();) {
        std::string key = it->first;
        std::shared_ptr<Amf0Any> any = it->second;

        if (key == name) {
            //  freep(any);

            it = properties.erase(it);
        } else {
            ++it;
        }
    }
}

void UnSortedHashtable::copy(UnSortedHashtable* src) {
    std::vector<Amf0ObjectPropertyType>::iterator it;
    for (it = src->properties.begin(); it != src->properties.end(); ++it) {
        Amf0ObjectPropertyType& elem = *it;
        std::string key = elem.first;
        std::shared_ptr<Amf0Any> any = elem.second;
        set(key, any->copy());
    }
}

Amf0ObjectEOF::Amf0ObjectEOF() {
    marker = RTMP_AMF0_ObjectEnd;
}

Amf0ObjectEOF::~Amf0ObjectEOF() {
}

int Amf0ObjectEOF::total_size() {
    return Amf0Size::object_eof();
}

int Amf0ObjectEOF::read(Buffer* stream) {
    int ret = error_success;

    // value
    if (!stream->read_require(2)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read object eof value failed. ret={}", ret);
        return ret;
    }
    int16_t temp = stream->read_2bytes();
    if (temp != 0x00) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read object eof value check failed. "
            "must be 0x00, actual is %#x, ret={}", temp, ret);
        return ret;
    }

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read object eof marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_ObjectEnd) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check object eof marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_ObjectEnd, ret);
        return ret;
    }
    tmss_info("amf0 read object eof marker success");

    tmss_info("amf0 read object eof success");

    return ret;
}
int Amf0ObjectEOF::write(Buffer* stream) {
    int ret = error_success;

    // value
    if (!stream->read_require(2)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write object eof value failed. ret={}", ret);
        return ret;
    }
    stream->write_2bytes(0x00);
    tmss_info("amf0 write object eof value success");

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write object eof marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_ObjectEnd);

    tmss_info("amf0 read object eof success");

    return ret;
}

std::shared_ptr<Amf0Any> Amf0ObjectEOF::copy() {
    return std::make_shared<Amf0ObjectEOF>();
}

int amf0_read_any(Buffer* stream, std::shared_ptr<Amf0Any>& ppvalue) {
    int ret = error_success;

    if ((ret = Amf0Any::discovery(stream, ppvalue)) != error_success) {
        tmss_error("amf0 discovery any elem failed. ret={}", ret);
        return ret;
    }

    assert(ppvalue);

    if ((ret = ppvalue->read(stream)) != error_success) {
        tmss_error("amf0 parse elem failed. ret={}", ret);
        return ret;
    }

    return ret;
}

int amf0_read_string(Buffer* stream, std::string& value) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read string marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_String) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check string marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_String, ret);
        return ret;
    }
    tmss_info("amf0 read string marker success");

    return amf0_read_utf8(stream, value);
}

int amf0_write_string(Buffer* stream, std::string value) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write string marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_String);
    tmss_info("amf0 write string marker success");

    return amf0_write_utf8(stream, value);
}

int amf0_read_boolean(Buffer* stream, bool& value) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read bool marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_Boolean) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check bool marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_Boolean, ret);
        return ret;
    }
    tmss_info("amf0 read bool marker success");

    // value
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read bool value failed. ret={}", ret);
        return ret;
    }

    value = (stream->read_1byte() != 0);

    tmss_info("amf0 read bool value success. value={}", value);

    return ret;
}
int amf0_write_boolean(Buffer* stream, bool value) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write bool marker failed. ret={}", ret);
        return ret;
    }
    stream->write_1byte(RTMP_AMF0_Boolean);
    tmss_info("amf0 write bool marker success");

    // value
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write bool value failed. ret={}", ret);
        return ret;
    }

    if (value) {
        stream->write_1byte(0x01);
    } else {
        stream->write_1byte(0x00);
    }

    tmss_info("amf0 write bool value success. value={}", value);

    return ret;
}

int amf0_read_number(Buffer* stream, double& value) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read number marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_Number) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check number marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_Number, ret);
        return ret;
    }
    tmss_info("amf0 read number marker success");

    // value
    if (!stream->read_require(8)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read number value failed. ret={}", ret);
        return ret;
    }

    int64_t temp = stream->read_8bytes();
    memcpy(&value, &temp, 8);

    tmss_info("amf0 read number value success. value=%.2f", value);

    return ret;
}
int amf0_write_number(Buffer* stream, double value) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write number marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_Number);
    tmss_info("amf0 write number marker success");

    // value
    if (!stream->read_require(8)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write number value failed. ret={}", ret);
        return ret;
    }

    int64_t temp = 0x00;
    memcpy(&temp, &value, 8);
    stream->write_8bytes(temp);

    tmss_info("amf0 write number value success. value=%.2f", value);

    return ret;
}

int amf0_read_null(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read null marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_Null) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check null marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_Null, ret);
        return ret;
    }
    tmss_info("amf0 read null success");

    return ret;
}
int amf0_write_null(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write null marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_Null);
    tmss_info("amf0 write null marker success");

    return ret;
}

int amf0_read_undefined(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read undefined marker failed. ret={}", ret);
        return ret;
    }

    char marker = stream->read_1byte();
    if (marker != RTMP_AMF0_Undefined) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 check undefined marker failed. "
            "marker=%#x, required=%#x, ret={}", marker, RTMP_AMF0_Undefined, ret);
        return ret;
    }
    tmss_info("amf0 read undefined success");

    return ret;
}
int amf0_write_undefined(Buffer* stream) {
    int ret = error_success;

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write undefined marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_Undefined);
    tmss_info("amf0 write undefined marker success");

    return ret;
}

int amf0_read_utf8(Buffer* stream, std::string& value) {
    int ret = error_success;

    // len
    if (!stream->read_require(2)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read string length failed. ret={}", ret);
        return ret;
    }
    int16_t len = stream->read_2bytes();
    tmss_info("amf0 read string length success. len={}", len);

    // empty string
    if (len <= 0) {
        tmss_info("amf0 read empty string. ret={}", ret);
        return ret;
    }

    // data
    if (!stream->read_require(len)) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read string data failed. ret={}", ret);
        return ret;
    }
    std::string str = stream->read_string(len);

    // support utf8-1 only
    // 1.3.1 Strings and UTF-8
    // UTF8-1 = %x00-7F
    // to do, support other utf-8 strings
    /* for (int i = 0; i < len; i++) {
        char ch = *(str.data() + i);
        if ((ch & 0x80) != 0) {
            ret = error_rtmp_amf0_decode;
            tmss_error("ignored. only support utf8-1, 0x00-0x7F, actual is %#x. ret={}", (int)ch, ret);
            ret = error_success;
        }
    }*/

    value = str;
    tmss_info("amf0 read string data success. str={}", str.c_str());

    return ret;
}
int amf0_write_utf8(Buffer* stream, std::string value) {
    int ret = error_success;

    // len
    if (!stream->read_require(2)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write string length failed. ret={}", ret);
        return ret;
    }
    stream->write_2bytes(value.length());
    tmss_info("amf0 write string length success. len={}", static_cast<int>(value.length()));

    // empty string
    if (value.length() <= 0) {
        tmss_info("amf0 write empty string. ret={}", ret);
        return ret;
    }

    // data
    if (!stream->read_require(value.length())) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write string data failed. ret={}", ret);
        return ret;
    }
    stream->write_bytes(value.c_str(), value.length());
    tmss_info("amf0 write string data success. str={}", value.c_str());

    return ret;
}

bool amf0_is_object_eof(Buffer* stream)  {
    // detect the object-eof specially
    if (stream->read_require(3)) {
        int32_t flag = stream->read_3bytes();
        stream->seek_read(-3);

        return 0x09 == flag;
    }

    return false;
}

int amf0_write_object_eof(Buffer* stream, std::shared_ptr<Amf0ObjectEOF> value) {
    int ret = error_success;

    assert(value != NULL);

    // value
    if (!stream->read_require(2)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write object eof value failed. ret={}", ret);
        return ret;
    }
    stream->write_2bytes(0x00);
    tmss_info("amf0 write object eof value success");

    // marker
    if (!stream->read_require(1)) {
        ret = error_rtmp_amf0_encode;
        tmss_error("amf0 write object eof marker failed. ret={}", ret);
        return ret;
    }

    stream->write_1byte(RTMP_AMF0_ObjectEnd);

    tmss_info("amf0 read object eof success");

    return ret;
}

int amf0_write_any(Buffer* stream, std::shared_ptr<Amf0Any> value) {
    assert(value != NULL);
    return value->write(stream);
}

}  // namespace tmss

