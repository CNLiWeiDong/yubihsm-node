/*
 * Adapted from: https://github.com/nodejs/node-addon-api/blob/master/doc/object_wrap.md
 */
#include "main.hpp"
#include "async.hpp"

Napi::Object Session::Init(Napi::Env env, Napi::Object exports) {
    // This method is used to hook the accessor and method callbacks
    Napi::Function func = DefineClass(env, "Session", {
        InstanceMethod("open", &Session::open),
        InstanceMethod("close", &Session::close),
        InstanceMethod("getPublicKey", &Session::getPublicKey),
        InstanceMethod("getKeys", &Session::getKeys),
        InstanceMethod("genKey", &Session::genKey),
        InstanceMethod("addAuthKey", &Session::addAuthKey),
        InstanceMethod("ecdh", &Session::ecdh),

        InstanceMethod("test", &Session::test)
    });

    // Create a peristent reference to the class constructor. This will allow
    // a function called on a class prototype and a function
    // called on instance of a class to be distinguished from each other.
    constructor = Napi::Persistent(func);
    // Call the SuppressDestruct() method on the static data prevent the calling
    // to this destructor to reset the reference when the environment is no longer
    // available.
    constructor.SuppressDestruct();
    exports.Set("Session", func);
    return exports;
}

/*
 * Constructor that will be called from Javascript with 'new Session()'
 */
Session::Session(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Session>(info) {
    const auto env = info.Env();
    yh_rc yrc{YHR_GENERIC_ERROR};

    // ...
    auto config = info[0].As<Napi::Object>();
    this->url = config.Get("url").As<Napi::String>().Utf8Value();
    this->password = config.Get("password").As<Napi::String>().Utf8Value();
    this->authkey = static_cast<uint16_t>(config.Get("authkey").As<Napi::Number>().Uint32Value());
    
    std::string domain = "1";
    if(config.Has("domain")) {
      domain = config.Get("domain").As<Napi::String>().Utf8Value();
    } 
    yrc = yh_string_to_domains(domain.c_str(), &this->domain);
    if(yrc != YHR_SUCCESS) {
      THROW(env, "yh_string_to_domains failed: {}", yh_strerror(yrc)); 
    }
    
    
    this->open(info);
}

Session::~Session(){
  this->close_session();
}

Napi::FunctionReference Session::constructor;

void Session::close_session() {
  if(this->session) {
    yh_util_close_session(this->session);
    yh_destroy_session(&this->session);
    session = nullptr;
  }
    
  if(this->connector) {
    yh_disconnect(this->connector);
    this->connector = nullptr;
  }
  yh_exit();
}
void Session::close(const Napi::CallbackInfo &info) {
  this->close_session();
}

void Session::open(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  this->close_session();
  yh_rc yrc{YHR_GENERIC_ERROR};
    
  yrc = yh_init();
  if(yrc != YHR_SUCCESS) {
    THROW(env, "yh_init failed: {}", yh_strerror(yrc));
  }

  // printf("Trying to connect to: \"%s\"\n", this->url.c_str());
  yrc = yh_init_connector(this->url.c_str(), &this->connector);
  if(yrc != YHR_SUCCESS) {
    THROW(env, "yh_init_connector failed: {}", yh_strerror(yrc));
  }

  yrc = yh_connect(connector, 0);
  if(yrc != YHR_SUCCESS) {
    THROW(env, "yh_connect failed: {}", yh_strerror(yrc));
  }

  yrc = yh_create_session_derived(connector, this->authkey, (const uint8_t *)this->password.c_str(),
                                  this->password.size(), false, &this->session);
  if(yrc != YHR_SUCCESS) {
    THROW(env, "yh_create_session_derived failed: {}", yh_strerror(yrc));
  }

  yrc = yh_authenticate_session(this->session);
  if(yrc != YHR_SUCCESS) {
    printf("yh_authenticate_session failed\n");
    THROW(env, "yh_authenticate_session: {}", yh_strerror(yrc));
  }
  
  uint8_t session_id;
  yrc = yh_get_session_id(this->session, &session_id);
  if(yrc != YHR_SUCCESS) {
    THROW(env, "yh_get_session_id: {}", yh_strerror(yrc));
  }

  // printf("Successfully established session %02d\n", session_id);
  
}

Napi::Value Session::getPublicKey(const Napi::CallbackInfo& info) {
  const auto env = info.Env();
  if (info.Length() < 1) {
    NAPI_THROW(Napi::TypeError::New(env, "Wrong number of arguments"));
  }
  if (!info[0].IsNumber()) {
    NAPI_THROW(Napi::TypeError::New(env, "Wrong arguments"));
  } 
  const auto key_id = static_cast<uint16_t>(info[0].As<Napi::Number>().Uint32Value());
  return dispatch_async(env, [=]() -> std::string {
    const auto pk_string = get_public_key(this->session, key_id);
    return pk_string;
  });
}

Napi::Value Session::getKeys(const Napi::CallbackInfo& info) {
  const auto env = info.Env();
  return dispatch_async(env, [=]() -> MyMap {
    MyMap m{};
    yh_rc yrc{YHR_GENERIC_ERROR};
    
    // std::array<yh_object_descriptor, 64> objects;
    size_t n_objects{64};
    yh_object_descriptor objects[n_objects];
    yh_capabilities capabilities;
    yh_string_to_capabilities("derive-ecdh", &capabilities);
    yrc = yh_util_list_objects(
      this->session, 
      0, // uint16_t id
      YH_ASYMMETRIC_KEY, // yh_object_type type 
      this->domain, // uint16_t domains
      &capabilities, // const yh_capabilities *capabilities
      YH_ALGO_EC_K256, //yh_algorithm algorithm 
      nullptr, // const char *label
      objects, 
      &n_objects
    );
    if(yrc != YHR_SUCCESS) {
      THROW_ASYNC("yh_util_list_objects failed: {}", yh_strerror(yrc));
    }
    for(size_t i = 0; i < n_objects; ++i) {
      const auto key_id = objects[i].id;
      const auto pkey = get_public_key(this->session, key_id);
      m.insert({pkey, key_id});
    }
    return m;
  });
}

Napi::Value Session::test(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  auto in = info[0].As<Napi::String>();
  auto s = in.Utf8Value();
  printf("Received s: %s\n", s.c_str());
  
  auto deferred = Napi::Promise::Deferred::New(env);
  // auto fun = Function::New(env, [&]() {
  //   printf("My Lambda function was called\n");
  //   deferred.Resolve(Napi::String::New(info.Env(), s));
  //   // printf("Resolve was called\n");
  //   // return Napi::String::New(info.Env(), "OK");
  // });
  EchoWorker* wk = new EchoWorker(deferred, [&]() -> MyMap {
    // HandleScope scope(Env());
    printf("Ohai Lambda 1\n");
    // auto obj = Napi::Object::New(Env());
    printf("Ohai Lambda 2\n");
    // obj.Set("rofl", "lol");
    printf("Ohai Lambda 3\n");
    MyMap my_car {
      {"make", "Ford"},
      {"model", "Mustang"},
      {"year", 1969}
    };
    return my_car;
  });
  wk->Queue();
  return deferred.Promise();
}

Napi::Value Session::addAuthKey(const Napi::CallbackInfo& info) {
  const auto env = info.Env();
  const auto key_label = info[0].As<Napi::String>().Utf8Value();
  const auto password = info[1].As<Napi::String>().Utf8Value();
  return dispatch_async(env, [=]() -> MyMap {
    MyMap m{};
    yh_rc yrc = YHR_GENERIC_ERROR;
    yh_capabilities capabilities = {{0}};
    yrc = yh_string_to_capabilities("derive-ecdh", &capabilities);
    if(yrc != YHR_SUCCESS) {
      THROW_ASYNC("yh_string_to_capabilities failed: {}", yh_strerror(yrc));
    }
    uint16_t key_id = 0;
    yrc = yh_util_import_authentication_key_derived(
      this->session, 
      &key_id, 
      key_label.c_str(), 
      this->domain,
      &capabilities,
      &capabilities, 
      (const uint8_t *)password.c_str(), 
      password.size()
    );
    if(yrc != YHR_SUCCESS) {
      THROW_ASYNC("yh_util_import_authentication_key_derived failed: {}", yh_strerror(yrc));
    }
    m.insert({"key_id", key_id});
    return m;
  });  
}

Napi::Value Session::genKey(const Napi::CallbackInfo& info) {
  const auto env = info.Env();
  // ...
  const auto key_label = info[0].As<Napi::String>().Utf8Value();
  
  return dispatch_async(env, [=]() -> MyMap {
    MyMap m{};
    yh_rc yrc = YHR_GENERIC_ERROR;
    yh_capabilities capabilities = {{0}};
    yrc = yh_string_to_capabilities("derive-ecdh", &capabilities);
    if(yrc != YHR_SUCCESS) {
      THROW_ASYNC("yh_string_to_capabilities failed: {}", yh_strerror(yrc));
    }

    uint16_t key_id = 0;
    yrc = yh_util_generate_ec_key(this->session, &key_id, key_label.c_str(), this->domain, &capabilities, YH_ALGO_EC_K256);
    if(yrc != YHR_SUCCESS) {
      THROW_ASYNC("yh_util_generate_ec_key failed: {}", yh_strerror(yrc));
    }
    
    const auto pkey = get_public_key(this->session, key_id);
    m.insert({"key_id", key_id});
    m.insert({"public_key", pkey});
    return m;
  });
  
}

Napi::Value Session::ecdh(const Napi::CallbackInfo& info) {
  const auto env = info.Env();
  const auto key_id = static_cast<uint16_t>(info[0].As<Napi::Number>().Uint32Value());
  const auto pubkey_str = info[1].As<Napi::String>().Utf8Value();

  return dispatch_async(env, [=]() -> bytes {
    bytes buffer{};
    yh_rc yrc{YHR_GENERIC_ERROR};    
    const auto compressed_pk = abieos::string_to_public_key(pubkey_str);
    uint8_t pubkey[65];
    
    /* this will only fill the first 64 bytes of pubkey */
    uECC_decompress(compressed_pk.data.data(), pubkey, uECC_secp256k1());
    int valid = uECC_valid_public_key(pubkey, uECC_secp256k1());
    if(!valid) {
      THROW_ASYNC("This is not a valid secp256k1 key");
    }
    
    /* 
     * uECC public keys are missing the 0x04 prefix that 
     * yh_util_derive_ecdh expects, so let's add it manually 
     */
     
    /* move the first 64 bytes one byte to the right (end of public key  
     * now aligns with end of buffer) */
    memmove(pubkey + 1, pubkey, sizeof(pubkey) - 1);
    /* The first byte of an uncompressed EC key should be 0x04 */
    pubkey[0] = 0x04;
      
    /* The shared secret for curve secp256k1 is always 32 bytes */
    size_t secret_len{32};
    buffer.resize(secret_len);
    
    yrc = yh_util_derive_ecdh(
      this->session, 
      key_id, 
      pubkey, 
      sizeof(pubkey),
      buffer.data(),
      &secret_len
    );
    if(yrc != YHR_SUCCESS) {
      THROW_ASYNC("yh_util_derive_ecdh failed: {}", yh_strerror(yrc));
    }
    if(secret_len != buffer.size()) {
      THROW_ASYNC("yh_util_derive_ecdh: buffer size mismatch, this should not be possible");
    }
    return buffer;
  });
}

// Initialize native add-on
Napi::Object Init (Napi::Env env, Napi::Object exports) {
    Session::Init(env, exports);
    return exports;
}

// Regisgter and initialize native add-on
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)