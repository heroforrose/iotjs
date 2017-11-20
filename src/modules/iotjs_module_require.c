/* Copyright 2018-present Samsung Electronics Co., Ltd. and other contributors
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

#include "iotjs_def.h"

/**
 * 1) Looking for a compiled module
 * 2) Lokking for an external JS file or module
 * 3) Cache!
 *
 *
 */

static const char*
resolve_file_path(const char* id, const char** parent) {
  var dirs = moduledirs;
  if (parent) {
    if (!parent.dirs) {
      parent.dirs = [];
    }
    dirs = parent.dirs.concat(dirs);
  }
  return dirs;
}

static const char*
resolve_file_path(const char* id, const char** directories) {

}


JS_FUNCTION(Require) {
  DJS_CHECK_ARGS(1, string);
  iotjs_string_t id = JS_GET_ARG(0, string);

  jerry_value_t jmodule = iotjs_module_get (id);

  if (!jerry_value_is_undefined(jmodule)) {
    iotjs_string_destroy(&id);
    return id;
  }

  for (unsigned i = 0; i < iotjs_modules_count; i++) {
    iotjs_jval_set_property_jval(builtin_modules, iotjs_modules[i].name,
                                 jerry_create_boolean(true));
  }


  jerry_value_t jglobal = jerry_get_global_object();
  jerry_value_t jprocess = iotjs_jval_get_property(jglobal, "process");
  jerry_value_t jbuiltins = iotjs_jval_get_property(jprocess, "builtin_modules");


  char path[IOTJS_MAX_PATH_SIZE];
  size_t size_path = sizeof(path);
  int err = uv_cwd(path, &size_path);
  if (err) {
    return JS_CREATE_ERROR(COMMON, "cwd error");
  }


  jerry_release_value(jglobal);
  jerry_release_value(jprocess);
  jerry_release_value(jbuiltins);
}

iotjs_jval_t InitRequire() {
  return jerry_create_external_function(Require);
}
