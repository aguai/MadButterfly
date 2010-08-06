dnl
dnl Developers should provide SET, WRAP, UNWRAP, and THROW C macros.
dnl
changequote(`[', `]')dnl
include([foreach.m4])dnl
divert([-1])dnl

define([UNQUOTE], [$*])

define([QUOTE], [[[$*]]])

define([VARFRAME], [dnl
pushdef([_FRAME_VARS], [])dnl
])

define([UNVARFRAME], [dnl
EXPAND(_FRAME_VARS)dnl
popdef([_FRAME_VARS])dnl
])

define([fdefine], [dnl
pushdef([$1], [$2])dnl
define([_FRAME_VARS], QUOTE(_FRAME_VARS[popdef([$1])]))
])

define([COUNT],[ifelse([$*],[],0,[$#])])

define([IMPORT],[define([$1],[$2$1(]$[]@[)])])

define([EXPAND], [$1])

define([PROJ_PREFIX], [xnjsmb_])

define([START_ACCESSOR], [dnl
divert([-1])dnl
VARFRAME
  fdefine([INT], [
static Handle<Value>
]PROJ_PREFIX[]STRUCT_NAME[_get_$][1(Local<String> property, const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *data;

    data = (STRUCT_TYPE *)UNWRAP(self);
    return Integer::New(data->$][1);
}

static void
]PROJ_PREFIX[]STRUCT_NAME[_set_$][1(Local<String> property,
		      Local<Value> value,
		      const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *data;

    data = (STRUCT_TYPE *)UNWRAP(self);
    data->$][1 = value->Int32Value();
}
])
  fdefine([NUMBER], [
static Handle<Value>
]PROJ_PREFIX[]STRUCT_NAME[_get_$][1(Local<String> property, const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *data;

    data = (STRUCT_TYPE *)UNWRAP(self);
    return Number::New(data->$][1);
}

static void
]PROJ_PREFIX[]STRUCT_NAME[_set_$][1(Local<String> property,
		      Local<Value> value,
		      const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *data;

    data = (STRUCT_TYPE *)UNWRAP(self);
    data->$][1 = value->NumberValue();
}
])
  fdefine([OBJ], [
static Handle<Value>
]PROJ_PREFIX[]STRUCT_NAME[_get_$][1(Local<String> property, const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *data;

    data = (STRUCT_TYPE *)UNWRAP(self);
    return ]PROJ_PREFIX[$][2_new(($][3 *)data->$][1);
}

static void
]PROJ_PREFIX[]STRUCT_NAME[_set_$][1(Local<String> property,
		      Local<Value> value,
		      const AccessorInfo &info) {
    Handle<Object> self = info.This();
    Handle<Object> obj;
    $][3 *v;
    STRUCT_TYPE *data;

    data = (STRUCT_TYPE *)UNWRAP(self);
    obj = value->ToObject();
    v = ($][3 *)UNWRAP(obj);
    data->$][1 = v;
}
])
  fdefine([STR], [
static Handle<Value>
]PROJ_PREFIX[]STRUCT_NAME[_get_$][1(Local<String> property, const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *data;

    data = (STRUCT_TYPE *)UNWRAP(self);
    return String::New(data->$][1);
}

static void
]PROJ_PREFIX[]STRUCT_NAME[_set_$][1(Local<String> property,
		      Local<Value> value,
		      const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *data;

    data = (STRUCT_TYPE *)UNWRAP(self);
    String::Utf8Value utf8(value->ToString());
    free(data->$][1);
    data->$][1 = strdup(*utf8);
}
])
dnl
dnl ACCESSOR(name, getter, setter)
dnl
  fdefine([ACCESSOR], [
static Handle<Value>
]PROJ_PREFIX[]STRUCT_NAME[_get_$][1(Local<String> property, const AccessorInfo &info) {
    Handle<Object> self = info.This();
    Handle<Value> _ret;
    STRUCT_TYPE *data;
    const char *err = NULL;

    data = (STRUCT_TYPE *)UNWRAP(self);
    _ret = $][2(self, data, &err);
    if(err)
	THROW(err);
    return _ret;
}

static void
]PROJ_PREFIX[]STRUCT_NAME[_set_$][1(Local<String> property,
		      Local<Value> value,
		      const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *data;
    const char *err = NULL;

    data = (STRUCT_TYPE *)UNWRAP(self);
    $][3(self, data, value, &err);
    if(err)
	THROW_noret(err);
}
])
divert([])dnl
])

define([STOP_ACCESSOR], [dnl
divert([-1])dnl
UNVARFRAME[]
divert([])dnl
])

define([SET_ACCESSSOR], [dnl
VARFRAME[]dnl
fdefine([INT], [$][1])dnl
fdefine([NUMBER], [$][1])dnl
fdefine([OBJ], [$][1])dnl
fdefine([STR], [$][1])dnl
fdefine([ACCESSOR], [$][1])dnl
    inst_temp->SetAccessor(String::New("$1"),
			   PROJ_PREFIX[]STRUCT_NAME[]_get_[]$1,
			   PROJ_PREFIX[]STRUCT_NAME[]_set_[]$1);
UNVARFRAME[]dnl
])

define([START_METHOD_ARG_VAR], [dnl
VARFRAME[]dnl
fdefine([INT], [dnl
    int arg_$][1;
])dnl
fdefine([NUMBER], [dnl
    double arg_$][1;
])dnl
fdefine([OBJ], [dnl
    $][3 *arg_$][1;
])dnl
fdefine([STR], [dnl
    char *arg_$][1;
])dnl
fdefine([FUNC], [dnl
    Handle<Function> arg_$][1;
])dnl
fdefine([SELF], [])dnl
fdefine([ERR], [])dnl
])

define([START_METHOD_ARG_TYPE_CHK], [dnl
VARFRAME[]dnl
fdefine([INT], [ ||
       !args[[i++]]->IsInt32()])dnl
fdefine([NUMBER], [ ||
       !args[[i++]]->IsNumber()])dnl
fdefine([OBJ], [ ||
       !args[[i++]]->IsObject()])dnl
fdefine([STR], [ ||
       !args[[i++]]->IsString()])dnl
fdefine([FUNC], [ ||
       !args[[i++]]->IsFunction()])dnl
fdefine([SELF], [])dnl
fdefine([ERR], [])dnl
])

define([START_TYPE_CHK], [dnl
VARFRAME[]dnl
fdefine([INT], [$1->IsInt32()])dnl
fdefine([NUMBER], [$1->IsNumber()])dnl
fdefine([OBJ], [$1->IsObject()])dnl
fdefine([STR], [$1->IsString()])dnl
fdefine([FUNC], [$1->IsFunction()])dnl
])

define([START_METHOD_ARG_ASSIGN], [dnl
VARFRAME[]dnl
fdefine([INT], [dnl
    arg_$][1 = args[[i++]]->Int32Value();
])dnl
fdefine([NUMBER], [dnl
    arg_$][1 = args[[i++]]->NumberValue();
])dnl
fdefine([OBJ], [dnl
    arg_$][1 = ($][3 *)UNWRAP(args[[i++]]->ToObject());
])dnl
fdefine([STR], [dnl
    arg_$][1 = strdup(*String::Utf8Value(args[[i++]]->ToString()));
])dnl
fdefine([FUNC], [dnl
    arg_$][1 = args[[i++]].As<Function>();
])dnl
fdefine([SELF], [])dnl
fdefine([ERR], [])dnl
])

define([START_VALUE_ASSIGN], [dnl
VARFRAME[]dnl
fdefine([INT], [dnl
    $1 = $2->Int32Value();
])dnl
fdefine([NUMBER], [dnl
    $1 = $2->NumberValue();
])dnl
fdefine([OBJ], [dnl
    $1 = ($][2 *)UNWRAP($2->ToObject());
])dnl
fdefine([STR], [dnl
    $1 = strdup(*String::Utf8Value($2->ToString()));
])dnl
fdefine([FUNC], [dnl
    $1 = $2.As<Function>();
])dnl
])

define([START_METHOD_ARG_PASS], [dnl
VARFRAME[]dnl
fdefine([INT], [arg_$][1])dnl
fdefine([NUMBER], [arg_$][1])dnl
fdefine([OBJ], [arg_$][1])dnl
fdefine([STR], [arg_$][1])dnl
fdefine([FUNC], [arg_$][1])dnl
fdefine([SELF], [self])dnl
fdefine([ERR], [&_err])dnl
fdefine([VAL], [&_err])dnl
])

define([START_METHOD_RET_VAL], [dnl
VARFRAME[]dnl
fdefine([INT], [dnl
    int _ret;
])dnl
fdefine([NUMBER], [dnl
    double _ret;
])dnl
fdefine([OBJ], [dnl
    $][2 *_ret;
])dnl
fdefine([STR], [dnl
    char *_ret;
])dnl
fdefine([FUNC], [dnl
    Handle<Function> _ret;
])dnl
fdefine([VAL], [dnl
    Handle<Value> _ret;
])dnl
])

define([START_VAR], [dnl
VARFRAME[]dnl
fdefine([INT], [dnl
    int $1;
])dnl
fdefine([NUMBER], [dnl
    double $1;
])dnl
fdefine([OBJ], [dnl
    $][2 *$1;
])dnl
fdefine([STR], [dnl
    char *$1;
])dnl
fdefine([FUNC], [dnl
    Handle<Function> $1;
])dnl
])

define([START_METHOD_RET_ASSIGN], [dnl
VARFRAME[]dnl
fdefine([INT], [_ret = (int)])dnl
fdefine([NUMBER], [_ret = (double)])dnl
fdefine([OBJ], [_ret = ($][2 *)])dnl
fdefine([STR], [_ret = (char *)])dnl
fdefine([FUNC], [_ret = ])dnl
fdefine([VAL], [_ret = ])dnl
])

define([START_METHOD_RET], [dnl
VARFRAME[]dnl
fdefine([INT], [
    _ret_val = Integer::New(_ret);
])dnl
fdefine([NUMBER], [
    _ret_val = Number::New(_ret);
])dnl
fdefine([OBJ], [
    _ret_val = PROJ_PREFIX[]$][1[]_new(_ret);
])dnl
fdefine([STR], [
    _ret_val = String::New(_ret);
])dnl
fdefine([FUNC], [
    _rt_val = _ret;
])dnl
fdefine([VAL], [
    _rt_val = _ret;
])dnl
])

define([STOP_METHOD_ARG], [dnl
UNVARFRAME[]dnl
])

define([START_METHOD], [dnl
dnl
dnl METHOD(name, func, arguments, cnt, ret_type, options)
dnl
define([METHOD], [
dnl
ifelse($][6, [], [], [dnl
foreach([ITER], ]$][6[, [EXPAND([define]ITER)])dnl
])dnl
dnl
static Handle<Value>
PROJ_PREFIX[]STRUCT_NAME[]_$][1(const Arguments &args) {
    HandleScope scope;
    int i;
    int argc = args.Length();
    Handle<Object> self = args.This();
    STRUCT_TYPE *_self = (STRUCT_TYPE *)UNWRAP(self);
    const char *_err = NULL;
foreach([ITER], $][3, [START_METHOD_ARG_VAR[]ITER[]STOP_METHOD_ARG])dnl
START_METHOD_RET_VAL[]$][5[]STOP_METHOD_ARG
    Handle<Value> _ret_val;

    if(argc != $][4)
        THROW("Invalid number of arguments (!=$][4)");
    i = 0;
    if(0[]dnl
foreach([ITER], $][3, [START_METHOD_ARG_TYPE_CHK[]ITER[]STOP_METHOD_ARG]))
        THROW("Invalid argument type");

    i = 0;
foreach([ITER], $][3, [START_METHOD_ARG_ASSIGN[]ITER[]STOP_METHOD_ARG])dnl

    START_METHOD_RET_ASSIGN[]$][5[]STOP_METHOD_ARG[]$][2(_self[]foreach([ITER], $][3, [START_METHOD_ARG_PASS[], ITER[]STOP_METHOD_ARG]));
    if(_err)
        THROW(_err);
START_METHOD_RET[]$][5[]STOP_METHOD_ARG[]dnl
ifelse($][5, [], [
    return Null();
], [dnl
dnl
dnl Modify returned object
dnl
ifdef([MOD], [
    MOD[](self, _ret_val);
])dnl
    scope.Close(_ret_val);
    return _ret_val;
])dnl
}
ifelse($][6, [], [], [dnl
foreach([ITER], ]$][6[, [EXPAND([undefine]ITER)])dnl
])dnl
])dnl
])

define([STOP_METHOD], [undefine([METHOD])])

define([SET_METHOD], [dnl
define([METHOD], [dnl
    SET(proto_temp, "$][1",
        FunctionTemplate::New(PROJ_PREFIX[]STRUCT_NAME[]_$][1));
])dnl
$1[]dnl
undefine([METHOD])dnl
])

define([DEF_GET_INDEX], [
static Handle<Value>
PROJ_PREFIX[]STRUCT_NAME[]_get_index(uint32_t index, const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *obj = (STRUCT_TYPE *)UNWRAP(self);
    const char *_err = NULL;
START_METHOD_RET_VAL[]$2[]STOP_METHOD_ARG[]dnl
    Handle<Value> _ret_val;

    _ret = $1(obj, self, index, &_err);
    if(_err)
        THROW(_err);
START_METHOD_RET[]$2[]STOP_METHOD_ARG[]dnl
    return _ret_val;
}
])

define([DEF_SET_INDEX], [
static Handle<Value>
PROJ_PREFIX[]STRUCT_NAME[]_set_index(uint32_t index, Local<Value> value,
	const AccessorInfo &info) {
    Handle<Object> self = info.This();
    STRUCT_TYPE *obj = (STRUCT_TYPE *)UNWRAP(self);
    const char *_err = NULL;
START_VAR([in_value])[]$2[]STOP_METHOD_ARG[]dnl
START_METHOD_RET_VAL[]$2[]STOP_METHOD_ARG[]dnl
    Handle<Value> _ret_val;

    if(START_TYPE_CHK(value)[]![]$2[]STOP_METHOD_ARG)
        THROW("Invalid value type");

START_VALUE_ASSIGN(in_value, value)[]$2[]STOP_METHOD_ARG[]dnl
    _ret = $1(obj, self, index, in_value, &_err);
    if(_err) THROW(_err);
START_METHOD_RET[]$2[]STOP_METHOD_ARG[]dnl
    return _ret_val;
}
])

define([INSTALL_INDEX_FUNCTIONS],[dnl
define([FIRST], [$][1])dnl
ifdef([GET_INDEX], [ifdef([SET_INDEX], [dnl
    inst_temp->SetIndexedPropertyHandler(PROJ_PREFIX[]STRUCT_NAME[]_get_index,
					 PROJ_PREFIX[]STRUCT_NAME[]_set_index);
], [dnl
    inst_temp->SetIndexedPropertyHandler(PROJ_PREFIX[]STRUCT_NAME[]_get_index);
])])dnl
undefine([FIRST])dnl
])

define([CTOR_INTERNAL], [dnl
    int argc = args.Length();
    Handle<Object> self = args.This();
    $4 *obj;
foreach([ITER], $2, [START_METHOD_ARG_VAR[]ITER[]STOP_METHOD_ARG])dnl
    int i;

    if(argc != $3)
        THROW("Invalid number of arguments (!=$][4)");
    i = 0;
    if(0]dnl
[foreach([ITER], $2, [START_METHOD_ARG_TYPE_CHK[]ITER[]STOP_METHOD_ARG]))
        THROW("Invalid argument type");

    i = 0;
foreach([ITER], $2, [START_METHOD_ARG_ASSIGN[]ITER[]STOP_METHOD_ARG])dnl

define([SEP], [])dnl
    obj = ($4 *)$1(foreach([ITER], $2, [START_METHOD_ARG_PASS[]SEP[]ITER[]STOP_METHOD_ARG[]define([SEP], [, ])]));[]undefine([SEP])

    WRAP(self, obj);
])

dnl
dnl STRUCT(struct_name, struct_type, member_vars, methods, options)
dnl
define([STRUCT], [dnl
define([STRUCT_NAME], [$1])dnl
define([STRUCT_TYPE], [$2])dnl
dnl
VARFRAME[]dnl
ifelse([$5], [], [], [dnl
foreach([ITER], $5, [dnl
EXPAND([fdefine]ITER)[]dnl
])dnl
])dnl
dnl
[
/* **************************************************
 * STRUCT: $1
 * Generated by gen_v8_binding.m4
 */
static Handle<Value>
]PROJ_PREFIX[$1(const Arguments &args) {
]ifdef([CTOR], [EXPAND([CTOR_INTERNAL](EXPAND([UNQUOTE]CTOR), [$2]))])dnl
    return Null();
[}

static Persistent<FunctionTemplate> ]PROJ_PREFIX[$1][_temp;

static Handle<Value>
]PROJ_PREFIX[$1][_new($2 *data) {
    Handle<Object> obj;
    Handle<Function> func;

    func = ]PROJ_PREFIX[$1][_temp->GetFunction();
    obj = func->NewInstance();
    WRAP(obj, data);

    return obj;
}
]dnl
foreach([ITER], ($3), [START_ACCESSOR ITER STOP_ACCESSOR])dnl
foreach([ITER], ($4), [START_METHOD ITER STOP_METHOD])dnl
ifdef([GET_INDEX], [EXPAND([DEF_GET_INDEX]GET_INDEX)])dnl
ifdef([SET_INDEX], [EXPAND([DEF_SET_INDEX]SET_INDEX)])dnl
[
static void
]PROJ_PREFIX[$1][_init(void) {
    Handle<FunctionTemplate> func_temp;
    Handle<ObjectTemplate> inst_temp;
    Handle<ObjectTemplate> proto_temp;

    func_temp = FunctionTemplate::New(]PROJ_PREFIX[$1);
    func_temp->SetClassName(String::New("]STRUCT_NAME["));
    inst_temp = func_temp->InstanceTemplate();
    inst_temp->SetInternalFieldCount(1);
    ]
foreach([ITER], ($3), [SET_ACCESSSOR(ITER)])dnl
INSTALL_INDEX_FUNCTIONS[]dnl

    proto_temp = func_temp->PrototypeTemplate();
foreach([ITER], ($4), [SET_METHOD(ITER)])dnl

    PROJ_PREFIX[$1][_temp = Persistent<FunctionTemplate>::New(func_temp);
}]dnl
dnl
UNVARFRAME[]dnl
dnl
])

dnl
dnl FUNCTION(func_name, real_func, arguments, arguement_count,
dnl          return_type, options)
dnl
define([FUNCTION], [dnl
dnl
VARFRAME[]dnl
ifelse($6, [], [], [dnl
foreach([ITER], $6, [EXPAND([fdefine]ITER)])dnl
])dnl
dnl
/* **************************************************
 * [FUNCTION]: $1
 * Generated by gen_v8_binding.m4
 */
static Handle<Value>
PROJ_PREFIX[]$1(const Arguments &args) {
    HandleScope scope;
    int argc = args.Length();
    int i;
    const char *_err = NULL;
foreach([ITER], ($3), [START_METHOD_ARG_VAR[]ITER[]STOP_METHOD_ARG])dnl
START_METHOD_RET_VAL[]$5[]STOP_METHOD_ARG[]dnl
    Handle<Value> _ret_val;
    
    if(argc != $4)
        THROW("Invalid number of arguments (!=$][4)");
    i = 0;
    if(0]dnl
[foreach([ITER], ($3), [START_METHOD_ARG_TYPE_CHK[]ITER[]STOP_METHOD_ARG]))
        THROW("Invalid argument type");

    i = 0;
foreach([ITER], ($3), [START_METHOD_ARG_ASSIGN[]ITER[]STOP_METHOD_ARG])dnl

define([SEP], [])dnl
    START_METHOD_RET_ASSIGN[]$5[]STOP_METHOD_ARG[]$2(foreach([ITER], ($3), [START_METHOD_ARG_PASS[]SEP[]ITER[]STOP_METHOD_ARG[]define([SEP], [, ])]));[]undefine([SEP])
    if(_err)
        THROW(_err);
START_METHOD_RET[]$][5[]STOP_METHOD_ARG[]dnl
ifelse($][5, [], [
    return Null();
], [dnl
dnl
dnl Modify returned object
dnl
ifdef([MOD], [
    MOD[](self, _ret_val);
])dnl
    scope.Close(_ret_val);
    return _ret_val;
])dnl
}
static Persistent<FunctionTemplate> PROJ_PREFIX[]$1[]_temp;

static void
PROJ_PREFIX[]$1[]_init(void) {
    Handle<FunctionTemplate> func_temp;

    func_temp = FunctionTemplate::New(PROJ_PREFIX[]$1);
    PROJ_PREFIX[]$1[]_temp = Persistent<FunctionTemplate>::New(func_temp);
}
dnl
UNVARFRAME[]dnl
dnl
])

divert([])dnl
