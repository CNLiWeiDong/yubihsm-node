#include <napi.h>
#include <yubihsm.h>



#define log_max_length 512


#define THROW(env, fmt, ...)                                                 \
do {                                                                         \
    char str[log_max_length];                                                \
    sprintf(str, "%s" fmt, "", ##__VA_ARGS__);                               \
    Napi::Error::New(env, str).ThrowAsJavaScriptException();                 \
} while (0)


#define THROW_ASYNC(fmt, ...)                                                \
do {                                                                         \
    char str[log_max_length];                                                \
    sprintf(str, "%s" fmt, "", ##__VA_ARGS__);                               \
    throw std::runtime_error(str);                                           \
} while (0)



class YubiHsm : public Napi::ObjectWrap<YubiHsm> {
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    YubiHsm(const Napi::CallbackInfo &info);
    ~YubiHsm();
  private:
    static Napi::FunctionReference constructor;
    yh_session *_session = nullptr;
    yh_connector *_connector = nullptr;
    std::string _url;
    uint16_t _authkey;
    uint16_t _domain;

    // internal functions
    void close_session();
    void close_connect();
    std::string get_public_key(uint16_t key_id);

    // JS-exposed functions
    Napi::Value close(const Napi::CallbackInfo &info);
    Napi::Value connect_dev(const Napi::CallbackInfo &info);
    Napi::Value open_session(const Napi::CallbackInfo &info);
    Napi::Value get_public_key(const Napi::CallbackInfo& info);
    Napi::Value create_authkey(const Napi::CallbackInfo& info);
    Napi::Value change_authkey_pwd(const Napi::CallbackInfo& info);
    Napi::Value delete_object(const Napi::CallbackInfo& info);
    Napi::Value gen_key(const Napi::CallbackInfo& info);
    Napi::Value sign_ecdsa(const Napi::CallbackInfo& info);
    Napi::Value sign_eddsa(const Napi::CallbackInfo& info);
};
