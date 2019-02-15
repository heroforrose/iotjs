/* Copyright 2018-present Rokid Co., Ltd. and other contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jerryscript-ext/handle-scope.h"
#include "jerryscript.h"
#include "internal/node_api_internal.h"
#include "modules/iotjs_module_buffer.h"

napi_status napi_assign_bool(bool value, bool* result) {
  NAPI_ASSIGN(result, value);
  NAPI_RETURN(napi_ok);
}

napi_status napi_assign_nvalue(jerry_value_t jvalue, napi_value* nvalue) {
  NAPI_ASSIGN(nvalue, AS_NAPI_VALUE(jvalue));
  NAPI_RETURN(napi_ok);
}

napi_status napi_create_array(napi_env env, napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval, jerry_create_array(0));
  return napi_assign_nvalue(jval, result);
}

napi_status napi_create_array_with_length(napi_env env, size_t length,
                                          napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval, jerry_create_array(length));
  return napi_assign_nvalue(jval, result);
}

napi_status napi_create_buffer(napi_env env, size_t size, void** data,
                               napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval_buf, iotjs_bufferwrap_create_buffer(size));
  iotjs_bufferwrap_t* buf_wrap = iotjs_bufferwrap_from_jbuffer(jval_buf);

  NAPI_ASSIGN(data, buf_wrap->buffer);

  return napi_assign_nvalue(jval_buf, result);
}

napi_status napi_create_buffer_copy(napi_env env, size_t size, const void* data,
                                    void** result_data, napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval_buf, iotjs_bufferwrap_create_buffer(size));
  iotjs_bufferwrap_t* buf_wrap = iotjs_bufferwrap_from_jbuffer(jval_buf);

  iotjs_bufferwrap_copy(buf_wrap, (char*)data, size);

  NAPI_ASSIGN(result_data, buf_wrap->buffer);
  NAPI_ASSIGN(result, AS_NAPI_VALUE(jval_buf));

  NAPI_RETURN(napi_ok);
}

napi_status napi_create_external_buffer(napi_env env, size_t length, void* data,
                                        napi_finalize finalize_cb,
                                        void* finalize_hint,
                                        napi_value* result) {
  NAPI_TRY_ENV(env);
  char* nval = NULL;
  napi_value res;
  NAPI_INTERNAL_CALL(
      napi_create_buffer_copy(env, length, data, (void**)&nval, &res));

  NAPI_ASSIGN(result, res);
  NAPI_RETURN(napi_ok);
}

napi_status napi_create_object(napi_env env, napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval, jerry_create_object());
  return napi_assign_nvalue(jval, result);
}

static napi_status napi_create_error_helper(jerry_error_t jerry_error_type,
                                            napi_env env, napi_value code,
                                            napi_value msg,
                                            napi_value* result) {
  NAPI_TRY_ENV(env);

  jerry_value_t jval_code = AS_JERRY_VALUE(code);
  jerry_value_t jval_msg = AS_JERRY_VALUE(msg);

  NAPI_TRY_TYPE(string, jval_msg);

  jerry_size_t msg_size = jerry_get_utf8_string_size(jval_msg);
  jerry_char_t raw_msg[msg_size + 1];
  jerry_size_t written_size =
      jerry_string_to_utf8_char_buffer(jval_msg, raw_msg, msg_size);
  NAPI_WEAK_ASSERT(napi_invalid_arg, written_size == msg_size);
  raw_msg[msg_size] = '\0';

  jerry_value_t jval_error = jerry_create_error(jerry_error_type, raw_msg);
  jval_error = jerry_create_error_from_value(jval_error, true);
  /**
   * reference count of error flag cleared jerry_value_t is separated
   * from its error reference, so it has be added to scope after clearing
   * error flag.
   */
  jerryx_create_handle(jval_error);
  /** code has to be an JS string type, thus it can not be an number 0 */
  if (code != NULL) {
    NAPI_TRY_TYPE(string, jval_code);
    iotjs_jval_set_property_jval(jval_error, IOTJS_MAGIC_STRING_CODE,
                                 jval_code);
  }
  NAPI_ASSIGN(result, AS_NAPI_VALUE(jval_error));

  NAPI_RETURN(napi_ok);
}

napi_status napi_create_error(napi_env env, napi_value code, napi_value msg,
                              napi_value* result) {
  return napi_create_error_helper(JERRY_ERROR_COMMON, env, code, msg, result);
}

napi_status napi_create_range_error(napi_env env, napi_value code,
                                    napi_value msg, napi_value* result) {
  return napi_create_error_helper(JERRY_ERROR_RANGE, env, code, msg, result);
}

napi_status napi_create_type_error(napi_env env, napi_value code,
                                   napi_value msg, napi_value* result) {
  return napi_create_error_helper(JERRY_ERROR_TYPE, env, code, msg, result);
}

static napi_status napi_number_convert_from_c_type_helper(napi_env env,
                                                          double value,
                                                          napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval, jerry_create_number(value));
  return napi_assign_nvalue(jval, result);
}

#define DEF_NAPI_NUMBER_CONVERT_FROM_C_TYPE(type, name)                        \
  napi_status napi_create_##name(napi_env env, type value,                     \
                                 napi_value* result) {                         \
    return napi_number_convert_from_c_type_helper(env, (double)value, result); \
  }

DEF_NAPI_NUMBER_CONVERT_FROM_C_TYPE(int32_t, int32);
DEF_NAPI_NUMBER_CONVERT_FROM_C_TYPE(uint32_t, uint32);
DEF_NAPI_NUMBER_CONVERT_FROM_C_TYPE(int64_t, int64);
DEF_NAPI_NUMBER_CONVERT_FROM_C_TYPE(double, double);
#undef DEF_NAPI_NUMBER_CONVERT_FROM_C_TYPE

#define DEF_NAPI_NUMBER_CONVERT_FROM_NVALUE(type, name)             \
  napi_status napi_get_value_##name(napi_env env, napi_value value, \
                                    type* result) {                 \
    NAPI_TRY_ENV(env);                                              \
    jerry_value_t jval = AS_JERRY_VALUE(value);                     \
    NAPI_TRY_TYPE(number, jval);                                    \
    double num_val = jerry_get_number_value(jval);                  \
    NAPI_ASSIGN(result, num_val);                                   \
    NAPI_RETURN(napi_ok);                                           \
  }

DEF_NAPI_NUMBER_CONVERT_FROM_NVALUE(double, double);
DEF_NAPI_NUMBER_CONVERT_FROM_NVALUE(int32_t, int32);
DEF_NAPI_NUMBER_CONVERT_FROM_NVALUE(int64_t, int64);
DEF_NAPI_NUMBER_CONVERT_FROM_NVALUE(uint32_t, uint32);
#undef DEF_NAPI_NUMBER_CONVERT_FROM_NVALUE

napi_status napi_create_string_utf8(napi_env env, const char* str,
                                    size_t length, napi_value* result) {
  NAPI_TRY_ENV(env);
  if (length == NAPI_AUTO_LENGTH) {
    length = strlen(str);
  }
  JERRYX_CREATE(jval,
                jerry_create_string_sz_from_utf8((jerry_char_t*)str, length));
  return napi_assign_nvalue(jval, result);
}

napi_status napi_get_array_length(napi_env env, napi_value value,
                                  uint32_t* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval = AS_JERRY_VALUE(value);
  NAPI_ASSIGN(result, jerry_get_array_length(jval));
  NAPI_RETURN(napi_ok);
}

napi_status napi_get_buffer_info(napi_env env, napi_value value, void** data,
                                 size_t* length) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval = AS_JERRY_VALUE(value);
  iotjs_bufferwrap_t* buf_wrap = iotjs_bufferwrap_from_jbuffer(jval);
  NAPI_ASSIGN(data, buf_wrap->buffer);
  NAPI_ASSIGN(length, iotjs_bufferwrap_length(buf_wrap));

  NAPI_RETURN(napi_ok);
}

napi_status napi_get_prototype(napi_env env, napi_value object,
                               napi_value* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval = AS_JERRY_VALUE(object);
  JERRYX_CREATE(jval_proto, jerry_get_prototype(jval));
  return napi_assign_nvalue(jval_proto, result);
}

napi_status napi_get_value_bool(napi_env env, napi_value value, bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval = AS_JERRY_VALUE(value);
  NAPI_TRY_TYPE(boolean, jval);
  return napi_assign_bool(jerry_get_boolean_value(jval), result);
}

napi_status napi_get_boolean(napi_env env, bool value, napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval, jerry_create_boolean(value));
  return napi_assign_nvalue(jval, result);
}

napi_status napi_get_value_string_utf8(napi_env env, napi_value value,
                                       char* buf, size_t bufsize,
                                       size_t* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval = AS_JERRY_VALUE(value);
  NAPI_TRY_TYPE(string, jval);

  size_t str_size = jerry_get_utf8_string_size(jval);
  if (buf == NULL) {
    /* null terminator is excluded */
    NAPI_ASSIGN(result, str_size);
    NAPI_RETURN(napi_ok);
  }

  jerry_size_t written_size =
      jerry_string_to_utf8_char_buffer(jval, (jerry_char_t*)buf, bufsize);
  NAPI_WEAK_ASSERT(napi_generic_failure,
                   str_size == 0 || (bufsize > 0 && written_size != 0),
                   "Insufficient buffer not supported yet.");
  /* expects one more byte to write null terminator  */
  if (bufsize > written_size) {
    buf[written_size] = '\0';
  }
  NAPI_ASSIGN(result, written_size);
  NAPI_RETURN(napi_ok);
}

napi_status napi_get_global(napi_env env, napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval, jerry_get_global_object());
  return napi_assign_nvalue(jval, result);
}

napi_status napi_get_null(napi_env env, napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval, jerry_create_null());
  return napi_assign_nvalue(jval, result);
}

napi_status napi_get_undefined(napi_env env, napi_value* result) {
  NAPI_TRY_ENV(env);
  JERRYX_CREATE(jval, jerry_create_undefined());
  return napi_assign_nvalue(jval, result);
}

#define DEF_NAPI_COERCE_TO(type, alias)                             \
  napi_status napi_coerce_to_##type(napi_env env, napi_value value, \
                                    napi_value* result) {           \
    NAPI_TRY_ENV(env);                                              \
    jerry_value_t jval = AS_JERRY_VALUE(value);                     \
    JERRYX_CREATE(jval_result, jerry_value_to_##alias(jval));       \
    return napi_assign_nvalue(jval_result, result);                 \
  }

DEF_NAPI_COERCE_TO(bool, boolean);
DEF_NAPI_COERCE_TO(number, number);
DEF_NAPI_COERCE_TO(object, object);
DEF_NAPI_COERCE_TO(string, string);

napi_status napi_typeof(napi_env env, napi_value value,
                        napi_valuetype* result) {
  // TODO: napi_extarnal is unsupported

  NAPI_TRY_ENV(env);
  jerry_value_t jval = AS_JERRY_VALUE(value);
  jerry_type_t type = jerry_value_get_type(jval);

  switch (type) {
    case JERRY_TYPE_UNDEFINED: {
      NAPI_ASSIGN(result, napi_undefined);
      break;
    }
    case JERRY_TYPE_NULL: {
      NAPI_ASSIGN(result, napi_null);
      break;
    }
    case JERRY_TYPE_BOOLEAN: {
      NAPI_ASSIGN(result, napi_boolean);
      break;
    }
    case JERRY_TYPE_NUMBER: {
      NAPI_ASSIGN(result, napi_number);
      break;
    }
    case JERRY_TYPE_STRING: {
      NAPI_ASSIGN(result, napi_string);
      break;
    }
    case JERRY_TYPE_OBJECT: {
      NAPI_ASSIGN(result, napi_object);
      break;
    }
    case JERRY_TYPE_FUNCTION: {
      NAPI_ASSIGN(result, napi_function);
      break;
    }
    default:
      NAPI_RETURN(napi_invalid_arg, NULL);
  }

  NAPI_RETURN(napi_ok);
}

#define DEF_NAPI_VALUE_IS(type)                                              \
  napi_status napi_is_##type(napi_env env, napi_value value, bool* result) { \
    NAPI_TRY_ENV(env);                                                       \
    return napi_assign_bool(jerry_value_is_##type(AS_JERRY_VALUE(value)),    \
                            result);                                         \
  }

DEF_NAPI_VALUE_IS(array);
DEF_NAPI_VALUE_IS(arraybuffer);
DEF_NAPI_VALUE_IS(error);
DEF_NAPI_VALUE_IS(typedarray);

napi_status napi_is_buffer(napi_env env, napi_value value, bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_global = jerry_get_global_object();
  jerry_value_t jval_buffer =
      iotjs_jval_get_property(jval_global, IOTJS_MAGIC_STRING_BUFFER);

  napi_status status =
      napi_instanceof(env, value, AS_NAPI_VALUE(jval_buffer), result);

  jerry_release_value(jval_buffer);
  jerry_release_value(jval_global);

  return status;
}

napi_status napi_instanceof(napi_env env, napi_value object,
                            napi_value constructor, bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  jerry_value_t jval_cons = AS_JERRY_VALUE(constructor);

  return napi_assign_bool(jerry_value_instanceof(jval_object, jval_cons),
                          result);
}

napi_status napi_strict_equals(napi_env env, napi_value lhs, napi_value rhs,
                               bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_lhs = AS_JERRY_VALUE(lhs);
  jerry_value_t jval_rhs = AS_JERRY_VALUE(rhs);

  return napi_assign_bool(jerry_value_strict_equal(jval_lhs, jval_rhs), result);
}
