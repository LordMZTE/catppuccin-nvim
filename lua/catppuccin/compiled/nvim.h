#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define luaL_reg luaL_Reg
typedef struct lua_State lua_State;
#define LUA_API extern
#define LUA_GLOBALSINDEX (-10002)
#define LUA_MULTRET (-1)
#define lua_pushliteral(L, s)                                                  \
  lua_pushlstring(L, "" s, (sizeof(s) / sizeof(char)) - 1)
LUA_API void(lua_pushlstring)(lua_State *L, const char *s, size_t l);
LUA_API void  (lua_pushstring) (lua_State *L, const char *s);
LUA_API void(lua_getfield)(lua_State *L, int idx, const char *k);
LUA_API void(lua_gettable)(lua_State *L, int idx);
LUA_API int(lua_gettop)(lua_State *L);
LUA_API void(lua_remove)(lua_State *L, int idx);
LUA_API void(lua_call)(lua_State *L, int nargs, int nresults);
LUA_API void(lua_concat)(lua_State *L, int n);
LUA_API void(lua_insert)(lua_State *L, int idx);
LUA_API void(lua_pushvalue)(lua_State *L, int idx);
LUA_API void luaL_where(lua_State *L, int level);
LUA_API int lua_error(lua_State *L);
typedef int(*lua_CFunction)(lua_State*L);
typedef struct luaL_Reg {
  const char *name;
  lua_CFunction func;
} luaL_Reg;
LUA_API void luaL_openlib(lua_State *L, const char *libname, const luaL_Reg *l, int nup);

#define ARRAY_DICT_INIT KV_INITIAL_VALUE
#define STRING_INIT                                                            \
  { .data = NULL, .size = 0 }
#define OBJECT_INIT                                                            \
  { .type = kObjectTypeNil }
#define ERROR_INIT                                                             \
  { .type = kErrorTypeNone, .msg = NULL }
#define ERROR_SET(e) ((e)->type != kErrorTypeNone)

#define kvec_t(type)                                                           \
  struct {                                                                     \
    size_t size;                                                               \
    size_t capacity;                                                           \
    type *items;                                                               \
  }

// Basic types
typedef enum {
  kErrorTypeNone = -1,
  kErrorTypeException,
  kErrorTypeValidation,
} ErrorType;

typedef struct {
  ErrorType type;
  char *msg;
} Error;

typedef bool Boolean;
typedef int64_t Integer;
typedef double Float;

typedef struct {
  char *data;
  size_t size;
} String;

typedef struct object Object;
typedef kvec_t(Object) Array;

typedef struct key_value_pair KeyValuePair;
typedef kvec_t(KeyValuePair) Dictionary;

typedef enum {
  kObjectTypeNil = 0,
  kObjectTypeBoolean,
  kObjectTypeInteger,
  kObjectTypeFloat,
  kObjectTypeString,
  kObjectTypeArray,
  kObjectTypeDictionary,
  kObjectTypeLuaRef,
  // EXT types, cannot be split or reordered, see #EXT_OBJECT_TYPE_SHIFT
  kObjectTypeBuffer,
  kObjectTypeWindow,
  kObjectTypeTabpage,
} ObjectType;

typedef int LuaRef;
struct object {
  ObjectType type;
  union {
    Boolean boolean;
    Integer integer;
    Float floating;
    String string;
    Array array;
    Dictionary dictionary;
    LuaRef luaref;
  } data;
};

struct key_value_pair {
  String key;
  Object value;
};

# define Dict(name) KeyDict_##name
typedef struct {
  Object scope;
  Object win;
  Object buf;
} KeyDict_option;
typedef struct {
  Object bg;
  Object fg;
  Object sp;
  Object bold;
  Object link;
  Object blend;
  Object cterm;
  Object italic;
  Object special;
  Object ctermbg;
  Object ctermfg;
  Object default_;
  Object reverse;
  Object fallback;
  Object standout;
  Object nocombine;
  Object undercurl;
  Object underline;
  Object background;
  Object foreground;
  Object global_link;
  Object underdashed;
  Object underdotted;
  Object underdouble;
  Object strikethrough;
} KeyDict_highlight;

#define api_free_boolean(value)
#define api_free_integer(value)
#define api_free_float(value)
#define api_free_buffer(value)
#define api_free_string(value)
#define api_free_keydict_highlight(value)
#define api_clear_error(value)
extern void api_set_error(Error *err, ErrorType errType, const char *format, ...);
extern Object *KeyDict_highlight_get_field(void *retval, const char *str, size_t len);
extern void nvim_set_hl(Integer ns_id, String name, Dict(highlight) *val, Error *err);

#define ms(x)                                                                  \
  (String) { x, sizeof(x) / sizeof(char) - 1 }
#define s(x)                                                                   \
  (Object) {                                                                   \
    .type = kObjectTypeString, .data.string = (String) {                       \
      x, sizeof(x) / sizeof(char) - 1                                          \
    }                                                                          \
  }
#define True                                                                   \
  (Object) { .type = kObjectTypeBoolean, .data.boolean = true }
#define False                                                                  \
  (Object) { .type = kObjectTypeBoolean, .data.boolean = false }

extern void nvim_set_option_value(String name, Object value,
                                  Dict(option) * opts, Error *err);
#define o(k, v)                                                                \
  do {                                                                         \
    KeyDict_option o = {};                                                     \
    Error e = ERROR_INIT;                                                      \
    nvim_set_option_value(                                                     \
        (String){.data = k, .size = sizeof(k) / sizeof(char) - 1}, v, &o, &e); \
  } while (0)

extern void nvim_set_var(String name, Object value, Error *err);
#define g(k, v)                                                                \
  do {                                                                         \
    Error e = ERROR_INIT;                                                      \
    nvim_set_var((String){.data = k, .size = sizeof(k) / sizeof(char) - 1}, v, \
                 &e);                                                          \
  } while (0)

#define h(name, ...) \
	do { \
		KeyDict_highlight dict = { __VA_ARGS__ }; \
		nvim_set_hl(0, ms(name), &dict, &err); \
		api_free_keydict_highlight(&dict); \
	} while (0)

#define c(name) \
	do { \
		nvim_set_hl(0, ms(name), &o, &err); \
	} while (0)

#define Error_check \
exit_0: \
	if (ERROR_SET(&err)) { \
		luaL_where(lstate, 1); \
		lua_pushstring(lstate, err.msg); \
		api_clear_error(&err); \
		lua_concat(lstate, 2); \
		return lua_error(lstate); \
	} \
	return 1;

