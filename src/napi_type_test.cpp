#include <napi.h>

Napi::Value getNumber(const CallbackInfo& info) {
  const auto env = info.Env();
  Napi::Number result = Number::New(env, 1);
  return result;
}

Napi::Value getString(const CallbackInfo& info){
  const auto env = info.Env();
  // 检测参数数量
  if(info.Length()<1){
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  // 检测参数类型
  if(!info[0].IsString()){
    Napi::TypeError::New(env, "Wrong arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string str = info[0].As<Napi::String>();
  if(str == "hello"){
    str = "hello world!";
  }else{
    str = "Please say hello!";
  }
  Napi::String reslut = Napi::String::New(env,str);
  return reslut;
}

Napi::Value getObj( const CallbackInfo& info ){
  const auto env = info.Env();
  // 准备返回给js，c++对象
  Napi::Object result = Napi::Object::New(env);
  result.Set(Napi::String::New(env,"name"),Napi::String::New(env,"lwd"));
  result.Set(Napi::String::New(env,"age"),Napi::Number::New(env,36));
  result.Set(Napi::String::New(env,"height"),Napi::Number::New(env,175));
  result.Set(Napi::String::New(env,"weight"),Napi::Number::New(env,66.5));
  result.Set(Napi::String::New(env,"old"),Napi::Boolean::New(env, true));
  return result;
}

Napi::Value getArr( const CallbackInfo& info ){
  const auto env = info.Env();
  // 准备返回给js，c++数组
  Napi::Array result = Napi::Array::New(env);
  // 准备一个对象
  Napi::Object obj = Napi::Object::New(env);
  obj.Set(Napi::String::New(env,"name"),Napi::String::New(env,"zhangsan"));
  // 给数组填充值
  result.Set(Napi::Number::New(env,0),Napi::String::New(env,"balabala"));
  result.Set(Napi::Number::New(env,1),Napi::Number::New(env,36));
  result.Set(Napi::Number::New(env,2),Napi::Number::New(env,66.5));
  result.Set(Napi::Number::New(env,3),obj);
  return result;
}