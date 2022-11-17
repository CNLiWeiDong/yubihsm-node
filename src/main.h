
#include <iostream>
#include <sstream>
#include <napi.h>
#include <yubihsm.h>



#define log_max_length 512


#define THROW(env, fmt, ...)                                                 \
do {                                                                         \
    char str[log_max_length];                                                \
    sprintf(str, "%s" fmt, "", ##__VA_ARGS__);                               \
    std::cerr<<str<<std::endl;                                               \
    Napi::Error::New(env, str).ThrowAsJavaScriptException();                 \
    throw std::runtime_error(str);                                           \
} while (0)


#define THROW_ASYNC(fmt, ...)                                                \
do {                                                                         \
    char str[log_max_length];                                                \
    sprintf(str, "%s" fmt, "", ##__VA_ARGS__);                               \
    throw std::runtime_error(str);                                           \
} while (0)


std::string string_to_hex(const std::string& data)
{
    const std::string hex = "0123456789abcdef";
    std::stringstream ss;

    for (std::string::size_type i = 0; i < data.size(); ++i)
        ss << hex[(unsigned char)data[i] >> 4] << hex[(unsigned char)data[i] & 0xf];
    std::cout << ss.str() << std::endl;
    return ss.str();
}

std::string hex_to_str(const std::string& str)
{
    std::string result;
    for (size_t i = 0; i < str.length(); i += 2)
    {
        std::string byte = str.substr(i, 2);
        char chr = (char)(int)strtol(byte.c_str(), NULL, 16);
        result.push_back(chr);
    }
    return result;
}


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
    Napi::Value get_opaque(const Napi::CallbackInfo& info);
    Napi::Value import_opaque(const Napi::CallbackInfo& info);
};
