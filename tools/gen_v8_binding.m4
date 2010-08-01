dnl
dnl Developers should provide SET, WRAP, UNWRAP, and THROW C macros.
dnl
changequote(`[', `]')dnl
include([foreach.m4])dnl
divert([-1])dnl

define([UNQUOTE], [$*])

define([QUOTE], [[[$*]]])

define([COUNT],[ifelse([$*],[],0,[$#])])

define([IMPORT],[define([$1],[$2$1(]$[]@[)])])

define([EXPAND], [$1])

define([PROJ_PREFIX], [xnjsmb_])

define([START_ACCESSOR], [dnl
divert([-1])dnl
  define([INT], [
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
  define([NUMBER], [
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
  define([OBJ], [
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
  define([STR], [
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
divert([])dnl
])

define([STOP_ACCESSOR], [dnl
divert([-1])dnl
undefine([INT])
undefine([NUMBER])
undefine([OBJ])
undefine([STR])
divert([])dnl
])

define([SET_ACCESSSOR], [dnl
define([INT], [$][1])dnl
define([NUMBER], [$][1])dnl
define([OBJ], [$][1])dnl
define([STR], [$][1])dnl
    inst_temp->SetAccessor(String::New("$1"),
			   PROJ_PREFIX[]STRUCT_NAME[]_get_[]$1,
			   PROJ_PREFIX[]STRUCT_NAME[]_set_[]$1);
undefine([INT])dnl
undefine([NUMBER])dnl
undefine([OBJ])dnl
undefine([STR])dnl
])

define([START_METHOD_ARG_VAR], [dnl
define([INT], [dnl
    int arg_$][1;
])dnl
define([NUMBER], [dnl
    double arg_$][1;
])dnl
define([OBJ], [dnl
    $][2 *arg_$][1;
])dnl
define([STR], [dnl
    char *arg_$][1;
])dnl
define([FUNC], [dnl
    Handle<Function> arg_$][1;
])dnl
define([SELF], [])dnl
define([ERR], [])dnl
])

define([START_METHOD_ARG_TYPE_CHK], [dnl
define([INT], [ ||
       !args[[i++]]->IsInt32()])dnl
define([NUMBER], [ ||
       !args[[i++]]->IsNumber()])dnl
define([OBJ], [ ||
       !args[[i++]]->IsObject()])dnl
define([STR], [ ||
       !args[[i++]]->IsString()])dnl
define([FUNC], [ ||
       !args[[i++]]->IsFunction()])dnl
define([SELF], [])dnl
define([ERR], [])dnl
])

define([START_TYPE_CHK], [dnl
define([INT], [$1->IsInt32()])dnl
define([NUMBER], [$1->IsNumber()])dnl
define([OBJ], [$1->IsObject()])dnl
define([STR], [$1->IsString()])dnl
define([FUNC], [$1->IsFunction()])dnl
])

define([START_METHOD_ARG_ASSIGN], [dnl
define([INT], [dnl
    arg_$][1 = args[[i++]]->Int32Value();
])dnl
define([NUMBER], [dnl
    arg_$][1 = args[[i++]]->NumberValue();
])dnl
define([OBJ], [dnl
    arg_$][1 = ($][2 *)UNWRAP(args[[i++]]->ToObject());
])dnl
define([STR], [dnl
    arg_$][1 = strdup(*String::Utf8Value(args[[i++]]->ToString()));
])dnl
define([FUNC], [dnl
    arg_$][1 = args[[i++]].As<Function>();
])dnl
define([SELF], [])dnl
define([ERR], [])dnl
])

define([START_VALUE_ASSIGN], [dnl
define([INT], [dnl
    $1 = $2->Int32Value();
])dnl
define([NUMBER], [dnl
    $1 = $2->NumberValue();
])dnl
define([OBJ], [dnl
    $1 = ($][2 *)UNWRAP($2->ToObject());
])dnl
define([STR], [dnl
    $1 = strdup(*String::Utf8Value($2->ToString()));
])dnl
define([FUNC], [dnl
    $1 = $2.As<Function>();
])dnl
])

define([START_METHOD_ARG_PASS], [dnl
define([INT], [arg_$][1])dnl
define([NUMBER], [arg_$][1])dnl
define([OBJ], [arg_$][1])dnl
define([STR], [arg_$][1])dnl
define([FUNC], [arg_$][1])dnl
define([SELF], [self])dnl
define([ERR], [&_err])dnl
])

define([START_METHOD_RET_VAL], [dnl
define([INT], [dnl
    int _ret;
])dnl
define([NUMBER], [dnl
    double _ret;
])dnl
define([OBJ], [dnl
    $][2 *_ret;
])dnl
define([STR], [dnl
    char *_ret;
])dnl
define([FUNC], [dnl
    Handle<Function> _ret;
])dnl
])

define([START_VAR], [dnl
define([INT], [dnl
    int $1;
])dnl
define([NUMBER], [dnl
    double $1;
])dnl
define([OBJ], [dnl
    $][2 *$1;
])dnl
define([STR], [dnl
    char *$1;
])dnl
define([FUNC], [dnl
    Handle<Function> $1;
])dnl
])

define([START_METHOD_RET_ASSIGN], [dnl
define([INT], [_ret = (int)])dnl
define([NUMBER], [_ret = (double)])dnl
define([OBJ], [_ret = ($][2 *)])dnl
define([STR], [_ret = (char *)])dnl
define([FUNC], [_ret = ])dnl
])

define([START_METHOD_RET], [dnl
define([INT], [
    return Integer::New(_ret);
])dnl
define([NUMBER], [
    return Number::New(_ret);
])dnl
define([OBJ], [
    return PROJ_PREFIX[]$][1[]_new(_ret);
])dnl
define([STR], [
    return String::New(_ret);
])dnl
define([FUNC], [
    return _ret;
])dnl
])

define([STOP_METHOD_ARG], [dnl
undefine([INT])dnl
undefine([NUMBER])dnl
undefine([OBJ])dnl
undefine([STR])dnl
undefine([FUNC])dnl
undefine([SELF])dnl
undefine([ERR])dnl
])

define([START_METHOD], [dnl
define([METHOD], [
static Handle<Value>
PROJ_PREFIX[]STRUCT_NAME[]_$][1(const Arguments &args) {
    int i;
    int argc = args.Length();
    Handle<Object> self = args.This();
    STRUCT_TYPE *_self = (STRUCT_TYPE *)UNWRAP(self);
    const char *_err = NULL;
foreach([ITER], $][3, [START_METHOD_ARG_VAR[]ITER[]STOP_METHOD_ARG])dnl
START_METHOD_RET_VAL[]$][5[]STOP_METHOD_ARG

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
])dnl
}
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

    _ret = $1(obj, self, index, &_err);
    if(_err)
        THROW(_err);
START_METHOD_RET[]$2[]STOP_METHOD_ARG[]dnl
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

    if(START_TYPE_CHK(value)[]![]$2[]STOP_METHOD_ARG)
        THROW("Invalid value type");

START_VALUE_ASSIGN(in_value, value)[]$2[]STOP_METHOD_ARG[]dnl
    _ret = $1(obj, self, index, in_value, &_err);
    if(_err) THROW(_err);
START_METHOD_RET[]$2[]STOP_METHOD_ARG[]dnl
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

dnl
dnl STRUCT(struct_name, struct_type, member_vars, methods)
dnl
define([STRUCT], [dnl
define([STRUCT_NAME], [$1])dnl
define([STRUCT_TYPE], [$2])dnl
dnl
ifelse([$5], [], [], [foreach([ITER], $5, [EXPAND([define]ITER)])])dnl
dnl
[
/* **************************************************
 * STRUCT: $1
 * Generated by gen_v8_binding.m4
 */
static Handle<Value>
]PROJ_PREFIX[$1(const Arguments &args) {
}

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
ifelse([$5], [], [], [foreach([ITER], $5, [dnl
define([DUMMY], [undefine]ITER)dnl
DUMMY[]dnl
undefine([DUMMY])])dnl
])dnl
dnl
])

dnl
dnl FUNCTION(func_name, real_func, arguments, arguement_count, return_type)
dnl
define([FUNCTION], [dnl
/* **************************************************
 * [FUNCTION]: $1
 * Generated by gen_v8_binding.m4
 */
static Handle<Value>
PROJ_PREFIX[]$1(const Arguments &args) {
    int argc = args.Length();
    int i;
    const char *_err = NULL;
foreach([ITER], ($3), [START_METHOD_ARG_VAR[]ITER[]STOP_METHOD_ARG])dnl
START_METHOD_RET_VAL[]$5[]STOP_METHOD_ARG[]dnl
    
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
])dnl
}
static Persistent<FunctionTemplate> PROJ_PREFIX[]$1[]_temp;

static void
PROJ_PREFIX[]$1[]_init(void) {
    Handle<FunctionTemplate> func_temp;

    func_temp = FunctionTemplate::New(PROJ_PREFIX[]$1);
    PROJ_PREFIX[]$1[]_temp = Persistent<FunctionTemplate>::New(func_temp);
}
])

divert([])dnl
