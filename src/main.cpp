/*
 * Adapted from: https://github.com/nodejs/node-addon-api/blob/master/doc/object_wrap.md
 */
#include "main.h"

Napi::FunctionReference YubiHsm::constructor;

Napi::Object YubiHsm::Init(Napi::Env env, Napi::Object exports) {
    // This method is used to hook the accessor and method callbacks
    Napi::Function func = DefineClass(env, "YubiHsm", {
        InstanceMethod("close", &YubiHsm::close),
        InstanceMethod("connectDev", &YubiHsm::connect_dev),
        InstanceMethod("openSession", &YubiHsm::open_session),
        InstanceMethod("getPublicKey", &YubiHsm::get_public_key),
        InstanceMethod("createAuthkey", &YubiHsm::create_authkey),
        InstanceMethod("changeAuthkeyPwd", &YubiHsm::change_authkey_pwd),
        InstanceMethod("deleteObject", &YubiHsm::delete_object),
        InstanceMethod("genKey", &YubiHsm::gen_key),
        InstanceMethod("signEcdsa", &YubiHsm::sign_ecdsa),
        InstanceMethod("signEddsa", &YubiHsm::sign_eddsa),
        InstanceMethod("getOpaque", &YubiHsm::get_opaque),
        InstanceMethod("importOpaque", &YubiHsm::import_opaque)
    });
    
    // Create a peristent reference to the class constructor. This will allow
    // a function called on a class prototype and a function
    // called on instance of a class to be distinguished from each other.
    constructor = Napi::Persistent(func);
    // Call the SuppressDestruct() method on the static data prevent the calling
    // to this destructor to reset the reference when the environment is no longer
    // available.
    constructor.SuppressDestruct();
    exports.Set("YubiHsm", func);
    return exports;
}


/*
 * Constructor that will be called from Javascript with 'new YubiHsm()'
 */
YubiHsm::YubiHsm(const Napi::CallbackInfo &info) : Napi::ObjectWrap<YubiHsm>(info) {
    const auto env = info.Env();
    yh_rc yrc{YHR_GENERIC_ERROR};

    auto config = info[0].As<Napi::Object>();
    _url = config.Get("url").As<Napi::String>().Utf8Value();
    // authkey 的密码每次openSession时传入，内存中不保留
    // _password = config.Get("password").As<Napi::String>().Utf8Value();
    _authkey = static_cast<uint16_t>(config.Get("authkey").As<Napi::Number>().Uint32Value());
    
    std::string domain = "all";
    if(config.Has("domain")) {
      domain = config.Get("domain").As<Napi::String>().Utf8Value();
    } 
    yrc = yh_string_to_domains(domain.c_str(), &_domain);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_string_to_domains failed: {}", yh_strerror(yrc)); 
    }
}

YubiHsm::~YubiHsm(){
  close_connect();
}

void YubiHsm::close_session() {
  if(_session) {
    yh_util_close_session(_session);
    yh_destroy_session(&_session);
    _session = nullptr;
  }
}

void YubiHsm::close_connect() {
  close_session();
  if(_connector) {
    yh_disconnect(_connector);
    _connector = nullptr;
  }
  yh_exit();
}

// js: close()->bool
Napi::Value YubiHsm::close(const Napi::CallbackInfo &info) {
    close_connect();
    const auto env = info.Env();
    Napi::Boolean result = Napi::Boolean::New(env, true);
    return result;
}

// js: connect_dev()->bool
Napi::Value YubiHsm::connect_dev(const Napi::CallbackInfo &info) {
    close_connect();
    const auto env = info.Env();
    yh_rc yrc{YHR_GENERIC_ERROR};

    yrc = yh_init();
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_init failed: %s", yh_strerror(yrc));
    }

    // printf("Trying to connect to: \"%s\"\n", _url.c_str());
    yrc = yh_init_connector(_url.c_str(), &_connector);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_init_connector failed: %s", yh_strerror(yrc));
    }

    yrc = yh_connect(_connector, 0);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_connect failed: %s", yh_strerror(yrc));
    }
    Napi::Boolean result = Napi::Boolean::New(env, true);
    return result;
}

// js: open_session(password)->number[session_id]
Napi::Value YubiHsm::open_session(const Napi::CallbackInfo &info) {
    if(!_connector) {
      connect_dev(info);
    }
    close_session();
    const auto env = info.Env();
    yh_rc yrc{YHR_GENERIC_ERROR};
    
    if(info.Length() < 1){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsString()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    std::string password = info[0].As<Napi::String>().Utf8Value();

    yrc = yh_create_session_derived(_connector, _authkey, (const uint8_t *)password.c_str(),
                                    password.size(), false, &_session);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_create_session_derived failed: %s", yh_strerror(yrc));
    }

    yrc = yh_authenticate_session(_session);
    if(yrc != YHR_SUCCESS) {
      printf("yh_authenticate_session failed\n");
      THROW(env, "yh_authenticate_session: %s", yh_strerror(yrc));
    }
    
    uint8_t session_id;
    yrc = yh_get_session_id(_session, &session_id);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_get_session_id: %s", yh_strerror(yrc));
    }
    Napi::Number result = Napi::Number::New(env, session_id);
    return result;
}

std::string YubiHsm::get_public_key(uint16_t key_id) {
    yh_rc yrc = YHR_GENERIC_ERROR;
    if(!_session) {
      THROW_ASYNC("need open one session");
    }
    uint8_t key[65];
    size_t public_key_len = sizeof(key);
    yrc = yh_util_get_public_key(_session, key_id, key, &public_key_len, NULL);
    
    if(yrc != YHR_SUCCESS) {
      THROW_ASYNC("Couldn't get public key");
    }

    std::string pk_string(std::begin(key), std::begin(key)+public_key_len);
    return string_to_hex(pk_string);
}

// js: get_public_key(key_id)->string[pk]
Napi::Value YubiHsm::get_public_key(const Napi::CallbackInfo& info) {
  const auto env = info.Env();
  if(!_session) {
    THROW(env, "need open one session");
  }
  yh_rc yrc = YHR_GENERIC_ERROR;

  if (info.Length() < 1) {
    THROW(env, "Wrong number of arguments");
  }
  if (!info[0].IsNumber()) {
    THROW(env, "Wrong arguments");
  } 
  const auto key_id = static_cast<uint16_t>(info[0].As<Napi::Number>().Uint32Value());
  
  std::string pk_string= get_public_key(key_id);
  Napi::String reslut = Napi::String::New(env,pk_string);
  return reslut;
}

// js: create_authkey(key_label, password)->string[pk]
Napi::Value YubiHsm::create_authkey(const Napi::CallbackInfo& info) {
    const auto env = info.Env();
    if(!_session) {
      THROW(env, "need open one session");
    }
    yh_rc yrc{YHR_GENERIC_ERROR};
    
    if(info.Length() < 2){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsString()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    if(!info[1].IsString()){
        THROW(env, "Wrong arguments type at %d", 1);
    }
    const auto key_label = info[0].As<Napi::String>().Utf8Value();
    const auto password = info[1].As<Napi::String>().Utf8Value();

    yh_capabilities capabilities = {{0}};
    // delete-authentication-key 删除默认authkey
    // change-authentication-key 可修改自己的密码
    // sign-ecdsa , sign-eddsa 两种曲线签名算法都支持
    // export-wrapped,import-wrapped 需要拥有导入导出的能力
    // generate-asymmetric-key 可生成椭圆曲线keypair
    // put-opaque,put-opaque,delete-opaque opaque对象的操作权限，用于自定义开发
    yrc = yh_string_to_capabilities("put-opaque,put-opaque,delete-opaque,delete-authentication-key,change-authentication-key,sign-ecdsa,sign-eddsa,export-wrapped,import-wrapped,generate-asymmetric-key", &capabilities);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_string_to_capabilities failed: %s", yh_strerror(yrc));
    }
    uint16_t key_id = 0;
    yrc = yh_util_import_authentication_key_derived(
      _session, 
      &key_id, 
      key_label.c_str(), 
      _domain,
      &capabilities,
      &capabilities, 
      (const uint8_t *)password.c_str(), 
      password.size()
    );
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_util_import_authentication_key_derived failed: %s", yh_strerror(yrc));
    }
    Napi::Number result = Napi::Number::New(env, key_id);
    return result;
}

// js: change_authkey_pwd(key_id, new_password)->bool[true]
Napi::Value YubiHsm::change_authkey_pwd(const Napi::CallbackInfo& info) {
    const auto env = info.Env();
    if(!_session) {
      THROW(env, "need open one session");
    }
    yh_rc yrc{YHR_GENERIC_ERROR};
    
    if(info.Length() < 2){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsNumber()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    if(!info[1].IsString()){
        THROW(env, "Wrong arguments type at %d", 1);
    }
    uint16_t key_id = static_cast<uint16_t>(info[0].As<Napi::Number>().Uint32Value());
    const auto password = info[1].As<Napi::String>().Utf8Value();

    yrc = yh_util_change_authentication_key_derived(
      _session, 
      &key_id, 
      (const uint8_t *)password.c_str(), 
      password.size()
    );

    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_util_change_authentication_key_derived failed: %s", yh_strerror(yrc));
    }
    Napi::Boolean result = Napi::Boolean::New(env, true);
    return result;
}

// js: delet_authkey(key_id, object_type)->bool[pk]
Napi::Value YubiHsm::delete_object(const Napi::CallbackInfo& info) {
    const auto env = info.Env();
    if(!_session) {
      THROW(env, "need open one session");
    }
    yh_rc yrc{YHR_GENERIC_ERROR};
    
    if(info.Length() < 2){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsNumber()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    if(!info[1].IsString()){
        THROW(env, "Wrong arguments type at %d", 1);
    }
    uint16_t key_id = static_cast<uint16_t>(info[0].As<Napi::Number>().Uint32Value());
    const auto object_type = info[1].As<Napi::String>().Utf8Value();

    yh_object_type type;
    yh_string_to_type(object_type.c_str(), &type);
    yrc = yh_util_delete_object(_session, key_id, type);

    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_util_delete_object failed: %s", yh_strerror(yrc));
    }
    Napi::Boolean result = Napi::Boolean::New(env, true);
    return result;
}

Napi::Value YubiHsm::import_opaque(const Napi::CallbackInfo& info) {
    const auto env = info.Env();
    if(!_session) {
      THROW(env, "need open one session");
    }
    yh_rc yrc{YHR_GENERIC_ERROR};

    if(info.Length() < 3){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsString()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    if(!info[1].IsString()){
        THROW(env, "Wrong arguments type at %d", 1);
    }
    if(!info[2].IsString()){
        THROW(env, "Wrong arguments type at %d", 2);
    }

    const auto key_label = info[0].As<Napi::String>().Utf8Value();
    const auto key_type = info[1].As<Napi::String>().Utf8Value();
    std::string in_data = info[2].As<Napi::String>().Utf8Value();
    in_data = hex_to_str(in_data);

    yh_algorithm algorithm;
    yrc = yh_string_to_algo(key_type.c_str(), &algorithm);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_string_to_algo failed: %s", yh_strerror(yrc));
    }
    uint16_t key_id = 0;
    yh_capabilities capabilities = {{0}};
    // put-opaque,put-opaque,delete-opaque opaque对象的操作权限，用于自定义开发
    yrc = yh_string_to_capabilities("put-opaque,put-opaque,delete-opaque", &capabilities);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_string_to_capabilities failed: %s", yh_strerror(yrc));
    }

    yrc = yh_util_import_opaque(_session, &key_id, key_label.c_str(), _domain, &capabilities, algorithm, (const uint8_t *)in_data.c_str(), (size_t)in_data.size());
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_util_import_opaque failed: %s", yh_strerror(yrc));
    }
    Napi::Number result = Napi::Number::New(env, key_id);
    return result;
}

Napi::Value YubiHsm::get_opaque(const Napi::CallbackInfo& info) {
    const auto env = info.Env();
    if(!_session) {
      THROW(env, "need open one session");
    }
    yh_rc yrc{YHR_GENERIC_ERROR};

    if(info.Length() < 1){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsNumber()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    uint16_t key_id = static_cast<uint16_t>(info[0].As<Napi::Number>().Uint32Value());
    uint8_t out_data[100];
    size_t out_data_len = sizeof(out_data);

    yrc = yh_util_get_opaque(_session, key_id,  (uint8_t *)out_data, &out_data_len);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_util_get_opaque failed: %s", yh_strerror(yrc));
    }
    std::string key(reinterpret_cast<char const*>(out_data), out_data_len);
    Napi::String result = Napi::String::New(env, string_to_hex(key));
    return result;

}
// js: gen_key(key_label, key_type)->->object[key_id,public_key]
// yh_string_to_algo()
Napi::Value YubiHsm::gen_key(const Napi::CallbackInfo& info) {
    const auto env = info.Env();
    if(!_session) {
      THROW(env, "need open one session");
    }
    yh_rc yrc{YHR_GENERIC_ERROR};

    if(info.Length() < 2){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsString()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    if(!info[1].IsString()){
        THROW(env, "Wrong arguments type at %d", 1);
    }
    const auto key_label = info[0].As<Napi::String>().Utf8Value();
    const auto key_type = info[1].As<Napi::String>().Utf8Value();


    yh_capabilities capabilities = {{0}};
    // sign-ecdsa , sign-eddsa 两种曲线签名算法都支持
    yrc = yh_string_to_capabilities("sign-ecdsa,sign-eddsa", &capabilities);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_string_to_capabilities failed: %s", yh_strerror(yrc));
    }

    yh_algorithm algorithm;
    yrc = yh_string_to_algo(key_type.c_str(), &algorithm);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_string_to_algo failed: %s", yh_strerror(yrc));
    }


    uint16_t key_id = 0;
    yrc = yh_util_generate_ec_key(_session, &key_id, key_label.c_str(), _domain, &capabilities, algorithm);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_util_generate_ec_key failed: %s", yh_strerror(yrc));
    }
    
    std::string pk_string= get_public_key(key_id);
    Napi::Object result = Napi::Object::New(env);
    result.Set(Napi::String::New(env,"public_key"),Napi::String::New(env, pk_string));
    result.Set(Napi::String::New(env,"key_id"),Napi::Number::New(env,key_id));
    return result;
}

// js: sign_ecdsa(key_id, in_data)->->string[signature]
Napi::Value YubiHsm::sign_ecdsa(const Napi::CallbackInfo& info) {
    const auto env = info.Env();
    if(!_session) {
      THROW(env, "need open one session");
    }
    yh_rc yrc{YHR_GENERIC_ERROR};

    if(info.Length() < 2){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsNumber()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    if(!info[1].IsString()){
        THROW(env, "Wrong arguments type at %d", 1);
    }
    uint16_t key_id = static_cast<uint16_t>(info[0].As<Napi::Number>().Uint32Value());
    std::string in_data = info[1].As<Napi::String>().Utf8Value();
    in_data = hex_to_str(in_data);
    uint8_t out_data[100];
    size_t out_data_len = sizeof(out_data);
    yrc = yh_util_sign_ecdsa(_session, key_id, (const uint8_t *)in_data.c_str(), (size_t)in_data.size(), (uint8_t *)out_data, &out_data_len);

    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_util_sign_ecdsa failed: %s", yh_strerror(yrc));
    }
    std::string signature(reinterpret_cast<char const*>(out_data), out_data_len);
    Napi::String result = Napi::String::New(env, string_to_hex(signature));
    return result;
}


// js: sign_eddsa(key_id, in_data)->->string[signature]
Napi::Value YubiHsm::sign_eddsa(const Napi::CallbackInfo& info) {
    const auto env = info.Env();
    if(!_session) {
      THROW(env, "need open one session");
    }
    yh_rc yrc{YHR_GENERIC_ERROR};

    if(info.Length() < 2){
        THROW(env, "Wrong number of arguments %d", info.Length());
    }
    if(!info[0].IsString()){
        THROW(env, "Wrong arguments type at %d", 0);
    }
    if(!info[1].IsString()){
        THROW(env, "Wrong arguments type at %d", 1);
    }
    uint16_t key_id = static_cast<uint16_t>(info[0].As<Napi::Number>().Uint32Value());
    std::string in_data = info[1].As<Napi::String>().Utf8Value();
    in_data = hex_to_str(in_data);
    
    uint8_t out_data[100];
    size_t out_data_len = sizeof(out_data);

    yrc = yh_util_sign_eddsa(_session, key_id, (const uint8_t *)in_data.c_str(), (size_t)in_data.size(), (uint8_t *)out_data, &out_data_len);

    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_util_sign_eddsa failed: %s", yh_strerror(yrc));
    }
    std::string signature(reinterpret_cast<char const*>(out_data), out_data_len);
    Napi::String result = Napi::String::New(env, string_to_hex(signature));
    return result;
}

// Initialize native add-on
Napi::Object Init (Napi::Env env, Napi::Object exports) {
    YubiHsm::Init(env, exports);
    return exports;
}

// Regisgter and initialize native add-on
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)