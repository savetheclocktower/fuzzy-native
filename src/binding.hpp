#include <napi.h>
#include "MatcherBase.h"

#define CHECK(cond, msg, env)                                  \
if (!(cond)) {                                                 \
  Napi::TypeError::New(env, msg).ThrowAsJavaScriptException(); \
  return env.Null();                                           \
}

#define CHECK_VOID(cond, msg, env)                             \
if (!(cond)) {                                                 \
  Napi::TypeError::New(env, msg).ThrowAsJavaScriptException(); \
  return;                                                      \
}

class Matcher : public Napi::ObjectWrap<Matcher> {
public:
  static void Init(Napi::Env, Napi::Object exports);
  Matcher(const Napi::CallbackInfo& info);

private:
  Napi::Value Match(const Napi::CallbackInfo& info);
  void AddCandidates(const Napi::CallbackInfo& info);
  void RemoveCandidates(const Napi::CallbackInfo& info);
  void SetCandidates(const Napi::CallbackInfo& info);

  MatcherBase _impl;
};
