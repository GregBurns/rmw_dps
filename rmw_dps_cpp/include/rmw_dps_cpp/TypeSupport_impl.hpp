// Copyright 2018 Intel Corporation All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RMW_DPS_CPP__TYPESUPPORT_IMPL_HPP_
#define RMW_DPS_CPP__TYPESUPPORT_IMPL_HPP_

#include <stdexcept>
#include <vector>

#include "rmw_dps_cpp/macros.hpp"

#include "rosidl_generator_c/primitives_array_functions.h"

namespace rmw_dps_cpp
{

template<typename T>
struct GenericCArray;

// multiple definitions of ambiguous primitive types
SPECIALIZE_GENERIC_C_ARRAY(bool, bool)
SPECIALIZE_GENERIC_C_ARRAY(byte, uint8_t)
SPECIALIZE_GENERIC_C_ARRAY(char, char)
SPECIALIZE_GENERIC_C_ARRAY(float32, float)
SPECIALIZE_GENERIC_C_ARRAY(float64, double)
SPECIALIZE_GENERIC_C_ARRAY(int8, int8_t)
SPECIALIZE_GENERIC_C_ARRAY(int16, int16_t)
SPECIALIZE_GENERIC_C_ARRAY(uint16, uint16_t)
SPECIALIZE_GENERIC_C_ARRAY(int32, int32_t)
SPECIALIZE_GENERIC_C_ARRAY(uint32, uint32_t)
SPECIALIZE_GENERIC_C_ARRAY(int64, int64_t)
SPECIALIZE_GENERIC_C_ARRAY(uint64, uint64_t)

typedef struct rosidl_generator_c__void__Array
{
  void * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} rosidl_generator_c__void__Array;

inline
bool
rosidl_generator_c__void__Array__init(
  rosidl_generator_c__void__Array * array, size_t size, size_t member_size)
{
  if (!array) {
    return false;
  }
  void * data = nullptr;
  if (size) {
    data = static_cast<void *>(calloc(size, member_size));
    if (!data) {
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

inline
void
rosidl_generator_c__void__Array__fini(rosidl_generator_c__void__Array * array)
{
  if (!array) {
    return;
  }
  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    free(array->data);
    array->data = nullptr;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

template<typename MembersType>
TypeSupport<MembersType>::TypeSupport(const MembersType * members)
{
  assert(members);
  this->members_ = members;
}

// C++ specialization
template<typename T>
void serialize_field(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  cbor::TxStream & ser)
{
  if (!member->is_array_) {
    ser << *static_cast<T *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    ser.serializeArray(static_cast<T *>(field), member->array_size_);
  } else {
    std::vector<T> & data = *reinterpret_cast<std::vector<T> *>(field);
    ser << data;
  }
}

// C specialization
template<typename T>
void serialize_field(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  cbor::TxStream & ser)
{
  if (!member->is_array_) {
    ser << *static_cast<T *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    ser.serializeArray(static_cast<T *>(field), member->array_size_);
  } else {
    auto & data = *reinterpret_cast<typename GenericCArray<T>::type *>(field);
    ser.serializeArray(reinterpret_cast<T *>(data.data), data.size);
  }
}

template<>
inline
void serialize_field<std::string>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  cbor::TxStream & ser)
{
  using CStringHelper = StringHelper<rosidl_typesupport_introspection_c__MessageMembers>;
  if (!member->is_array_) {
    auto && str = CStringHelper::convert_to_std_string(field);
    // Control maximum length.
    if (member->string_upper_bound_ && str.length() > member->string_upper_bound_ + 1) {
      throw std::runtime_error("string overcomes the maximum length");
    }
    ser << str;
  } else {
    // First, cast field to rosidl_generator_c
    // Then convert to a std::string using StringHelper and serialize the std::string
    std::vector<std::string> cpp_string_vector;
    if (member->array_size_ && !member->is_upper_bound_) {
      auto string_field = static_cast<rosidl_generator_c__String *>(field);
      for (size_t i = 0; i < member->array_size_; ++i) {
        cpp_string_vector.push_back(
          CStringHelper::convert_to_std_string(string_field[i]));
      }
    } else {
      auto & string_array_field = *reinterpret_cast<rosidl_generator_c__String__Array *>(field);
      for (size_t i = 0; i < string_array_field.size; ++i) {
        cpp_string_vector.push_back(
          CStringHelper::convert_to_std_string(string_array_field.data[i]));
      }
    }
    ser << cpp_string_vector;
  }
}

inline
size_t get_submessage_array_serialize(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  cbor::TxStream & ser,
  void * & field,
  void * & subros_message)
{
  if (member->array_size_ && !member->is_upper_bound_) {
      subros_message = field;
      return member->array_size_;
  } else {
      subros_message = field;
      size_t array_size = member->size_function(field);
      if (member->is_upper_bound_ && array_size > member->array_size_) {
          throw std::runtime_error("array overcomes the maximum length");
      }
      // Serialize length
      ser << (uint32_t)array_size;
      return array_size;
  }
}

inline
size_t get_submessage_array_serialize(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  cbor::TxStream & ser,
  void * & field,
  void * & subros_message)
{
  if (member->array_size_ && !member->is_upper_bound_) {
      subros_message = &field;
      return member->array_size_;
  } else {
      subros_message = field;
      size_t array_size = member->size_function(field);
      if (member->is_upper_bound_ && array_size > member->array_size_) {
          throw std::runtime_error("array overcomes the maximum length");
      }
      // Serialize length
      ser << (uint32_t)array_size;
      return array_size;
  }
}

template<typename MembersType>
bool TypeSupport<MembersType>::serializeROSmessage(
  cbor::TxStream & ser, const MembersType * members, const void * ros_message)
{
  assert(ros_message);
  assert(members);

  ser.serializeArray(members->member_count_);

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto member = members->members_ + i;
    void * field = const_cast<char *>(static_cast<const char *>(ros_message)) + member->offset_;
    switch (member->type_id_) {
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
        if (!member->is_array_) {
          // don't cast to bool here because if the bool is
          // uninitialized the random value can't be deserialized
          ser << (*static_cast<uint8_t *>(field) ? true : false);
        } else {
          serialize_field<bool>(member, field, ser);
        }
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        serialize_field<uint8_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
        serialize_field<char>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
        serialize_field<float>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
        serialize_field<double>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        serialize_field<int16_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        serialize_field<uint16_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        serialize_field<int32_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        serialize_field<uint32_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        serialize_field<int64_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        serialize_field<uint64_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
        serialize_field<std::string>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
        {
          auto sub_members = static_cast<const MembersType *>(member->members_->data);
          if (!member->is_array_) {
            serializeROSmessage(ser, sub_members, field);
          } else {
            void * subros_message = nullptr;
            size_t array_size = 0;

            array_size = get_submessage_array_serialize(
              member, ser, field, subros_message);

            for (size_t index = 0; index < array_size; ++index) {
              serializeROSmessage(ser, sub_members, member->get_function(subros_message, index));
            }
          }
        }
        break;
      default:
        throw std::runtime_error("unknown type");
    }
  }

  return true;
}

template<typename T>
void deserialize_field(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  cbor::RxStream & deser,
  bool)
{
  if (!member->is_array_) {
    deser >> *static_cast<T *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    deser.deserializeArray(static_cast<T *>(field), member->array_size_);
  } else {
    auto & vector = *reinterpret_cast<std::vector<T> *>(field);
    new(&vector) std::vector<T>;
    deser >> vector;
  }
}

template<typename T>
void deserialize_field(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  cbor::RxStream & deser,
  bool)
{
  if (!member->is_array_) {
    deser >> *static_cast<T *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    deser.deserializeArray(static_cast<T *>(field), member->array_size_);
  } else {
    auto & data = *reinterpret_cast<typename GenericCArray<T>::type *>(field);
    size_t dsize = 0;
    deser.deserializeArraySize(&dsize);
    if (!GenericCArray<T>::init(&data, dsize)) {
      throw std::runtime_error("unable to initialize GenericCArray");
    }
    deser.deserializeArray(reinterpret_cast<T *>(data.data), dsize);
  }
}

template<>
inline
void deserialize_field<std::string>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  cbor::RxStream & deser,
  bool call_new)
{
  using CStringHelper = StringHelper<rosidl_typesupport_introspection_c__MessageMembers>;
  if (!member->is_array_) {
    CStringHelper::assign(deser, field, call_new);
  } else {
    std::vector<std::string> cpp_string_vector;
    deser >> cpp_string_vector;

    if (member->array_size_ && !member->is_upper_bound_) {
      auto deser_field = static_cast<rosidl_generator_c__String *>(field);
      for (size_t i = 0; i < member->array_size_; ++i) {
        if (!rosidl_generator_c__String__assign(&deser_field[i],
          cpp_string_vector[i].c_str()))
        {
          throw std::runtime_error("unable to assign rosidl_generator_c__String");
        }
      }
    } else {
      auto & string_array_field = *reinterpret_cast<rosidl_generator_c__String__Array *>(field);
      if (!rosidl_generator_c__String__Array__init(&string_array_field, cpp_string_vector.size())) {
        throw std::runtime_error("unable to initialize rosidl_generator_c__String array");
      }
      for (size_t i = 0; i < cpp_string_vector.size(); ++i) {
        if (!rosidl_generator_c__String__assign(&string_array_field.data[i],
          cpp_string_vector[i].c_str()))
        {
          throw std::runtime_error("unable to assign rosidl_generator_c__String");
        }
      }
    }
  }
}

inline
size_t get_submessage_array_deserialize(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  cbor::RxStream & deser,
  void * & field,
  void * & subros_message,
  bool & call_new)
{
  if (member->array_size_ && !member->is_upper_bound_) {
    subros_message = field;
    return member->array_size_;
  } else {
    // Deserialize length
    uint32_t array_size = 0;
    deser >> array_size;
    auto vector = reinterpret_cast<std::vector<unsigned char> *>(field);
    new(vector) std::vector<unsigned char>;
    member->resize_function(field, array_size);
    subros_message = field;
    call_new = true;
    return array_size;
  }
}

inline
size_t get_submessage_array_deserialize(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  cbor::RxStream & deser,
  void * & field,
  void * & subros_message,
  bool & call_new)
{
  if (member->array_size_ && !member->is_upper_bound_) {
    subros_message = &field;
    return member->array_size_;
  } else {
    // Deserialize length
    uint32_t array_size = 0;
    deser >> array_size;
    member->resize_function(field, array_size);
    subros_message = field;
    call_new = true;
    return array_size;
  }
}

template<typename MembersType>
bool TypeSupport<MembersType>::deserializeROSmessage(
  cbor::RxStream & deser, const MembersType * members, void * ros_message, bool call_new)
{
  assert(members);
  assert(ros_message);

  size_t member_count = 0;
  deser.deserializeArray(&member_count);
  if (member_count != members->member_count_) {
    throw std::runtime_error("failed to deserialize value");
  }

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto * member = members->members_ + i;
    void * field = static_cast<char *>(ros_message) + member->offset_;
    switch (member->type_id_) {
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
        deserialize_field<bool>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        deserialize_field<uint8_t>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
        deserialize_field<char>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
        deserialize_field<float>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
        deserialize_field<double>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        deserialize_field<int16_t>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        deserialize_field<uint16_t>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        deserialize_field<int32_t>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        deserialize_field<uint32_t>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        deserialize_field<int64_t>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        deserialize_field<uint64_t>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
        deserialize_field<std::string>(member, field, deser, call_new);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
        {
          auto sub_members = (const MembersType *)member->members_->data;
          if (!member->is_array_) {
            deserializeROSmessage(deser, sub_members, field, call_new);
          } else {
            void * subros_message = nullptr;
            size_t array_size = 0;
            bool recall_new = call_new;

            array_size = get_submessage_array_deserialize(
              member, deser, field, subros_message, recall_new);

            for (size_t index = 0; index < array_size; ++index) {
              deserializeROSmessage(
                deser, sub_members, member->get_function(subros_message, index), recall_new);
            }
          }
        }
        break;
      default:
        throw std::runtime_error("unknown type");
    }
  }

  return true;
}

template<typename MembersType>
bool TypeSupport<MembersType>::serializeROSmessage(
  const void * ros_message, cbor::TxStream & ser)
{
  assert(ros_message);

  if (members_->member_count_ != 0) {
    TypeSupport::serializeROSmessage(ser, members_, ros_message);
  } else {
    ser << (uint8_t)0;
  }
  if (ser.status() == DPS_ERR_OVERFLOW) {
      ser = cbor::TxStream(ser.size_needed());
      if (members_->member_count_ != 0) {
          TypeSupport::serializeROSmessage(ser, members_, ros_message);
      } else {
          ser << (uint8_t)0;
      }
  }
  return ser.status() == DPS_OK;
}

template<typename MembersType>
bool TypeSupport<MembersType>::deserializeROSmessage(
  cbor::RxStream & deser, void * ros_message)
{
  assert(ros_message);

  if (members_->member_count_ != 0) {
    TypeSupport::deserializeROSmessage(deser, members_, ros_message, false);
  } else {
    uint8_t dump = 0;
    deser >> dump;
    (void)dump;
  }

  return true;
}

}  // namespace rmw_dps_cpp

#endif  // RMW_DPS_CPP__TYPESUPPORT_IMPL_HPP_
