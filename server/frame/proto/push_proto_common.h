// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: push_proto_common.proto

#ifndef PROTOBUF_push_5fproto_5fcommon_2eproto__INCLUDED
#define PROTOBUF_push_5fproto_5fcommon_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2005000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2005000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_enum_reflection.h>
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_push_5fproto_5fcommon_2eproto();
void protobuf_AssignDesc_push_5fproto_5fcommon_2eproto();
void protobuf_ShutdownFile_push_5fproto_5fcommon_2eproto();


enum ResultCode {
  RESULT_SUCCESS = 0,
  RESULT_FORBIDDEN = 1,
  RESULT_UNLOAD = 2,
  RESULT_FAIL = 10000
};
bool ResultCode_IsValid(int value);
const ResultCode ResultCode_MIN = RESULT_SUCCESS;
const ResultCode ResultCode_MAX = RESULT_FAIL;
const int ResultCode_ARRAYSIZE = ResultCode_MAX + 1;

const ::google::protobuf::EnumDescriptor* ResultCode_descriptor();
inline const ::std::string& ResultCode_Name(ResultCode value) {
  return ::google::protobuf::internal::NameOfEnum(
    ResultCode_descriptor(), value);
}
inline bool ResultCode_Parse(
    const ::std::string& name, ResultCode* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ResultCode>(
    ResultCode_descriptor(), name, value);
}
// ===================================================================


// ===================================================================


// ===================================================================


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {

template <>
inline const EnumDescriptor* GetEnumDescriptor< ::ResultCode>() {
  return ::ResultCode_descriptor();
}

}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_push_5fproto_5fcommon_2eproto__INCLUDED
