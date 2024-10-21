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

Matcher::Matcher(const Napi::CallbackInfo& info): Napi::ObjectWrap<Matcher>(info) {
  auto env = info.Env();
  CHECK_VOID(
    info.IsConstructCall(),
    "Use 'new' to construct Matcher",
    env
  );
  AddCandidates(info);
}

void Matcher::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Matcher", {
    InstanceMethod<&Matcher::Match>("match", napi_default_method),
    InstanceMethod<&Matcher::AddCandidates>("addCandidates", napi_default_method),
    InstanceMethod<&Matcher::RemoveCandidates>("removeCandidates", napi_default_method),
    InstanceMethod<&Matcher::SetCandidates>("setCandidates", napi_default_method)
  });

  exports.Set("Matcher", func);
}

Napi::Value Matcher::Match(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  CHECK(info.Length() > 0, "Wrong number of arguments", env);
  CHECK(info[0].IsString(), "First argument should be a query string", env);

  Napi::String query(info[0].As<Napi::String>());
  std::string queryStr(query);

  MatcherOptions options;

  if (info.Length() > 1) {
    CHECK(info[1].IsObject(), "Second argument should be an options object", env);
    auto obj = info[1].As<Napi::Object>();
    if (obj.Get("caseSensitive").IsBoolean()) {
      options.case_sensitive = obj.Get("caseSensitive").As<Napi::Boolean>();
    }
    if (obj.Get("smartCase").IsBoolean()) {
      options.smart_case = obj.Get("smartCase").As<Napi::Boolean>();
    }
    if (obj.Get("recordMatchIndexes").IsBoolean()) {
      options.record_match_indexes = obj.Get("recordMatchIndexes").As<Napi::Boolean>();
    }
    if (obj.Get("numThreads").IsNumber()) {
      options.num_threads = obj.Get("numThreads").As<Napi::Number>().Int32Value();
    }
    if (obj.Get("maxResults").IsNumber()) {
      options.max_results = obj.Get("maxResults").As<Napi::Number>().Int32Value();
    }
    if (obj.Get("maxGap").IsNumber()) {
      options.max_gap = obj.Get("maxGap").As<Napi::Number>().Int32Value();
    }
    if (obj.Get("algorithm").IsString()) {
      std::string algorithm(obj.Get("algorithm").As<Napi::String>());
      if (algorithm == "fuzzaldrin") {
        options.fuzzaldrin = true;
      }
    }
    if (obj.Get("rootPath").IsString()) {
      options.root_path = obj.Get("rootPath").As<Napi::String>();
    }
  }

  auto idKey = Napi::String::New(env, "id");
  auto valueKey = Napi::String::New(env, "value");
  auto scoreKey = Napi::String::New(env, "score");
  auto matchIndexesKey = Napi::String::New(env, "matchIndexes");

  std::vector<MatchResult> matches = _impl.findMatches(query, options);

  auto result = Napi::Array::New(env);
  size_t result_count = 0;
  for (const auto &match : matches) {
    auto obj = Napi::Object::New(env);
    obj.Set(idKey, Napi::Number::New(env, match.id));
    obj.Set(scoreKey, Napi::Number::New(env, match.score));
    obj.Set(valueKey, Napi::String::New(env, *match.value));

    if (match.matchIndexes != nullptr) {
      auto array = Napi::Array::New(env, match.matchIndexes->size());
      for (size_t i = 0; i < array.Length(); i++) {
        array.Set(i, Napi::Number::New(env, match.matchIndexes->at(i)));
      }
      obj.Set(matchIndexesKey, array);
    }
    result.Set(result_count++, obj);
  }

  return result;
}

void Matcher::AddCandidates(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  if (info.Length() > 0) {
    CHECK_VOID(
      info[0].IsArray(),
      "Expected an array of unsigned 32-bit integer IDs as the first argument",
      env
    );
    CHECK_VOID(
      info[1].IsArray(),
      "Expected an array of strings as the second argument",
      env
    );

    auto ids = info[0].As<Napi::Array>();
    auto values = info[1].As<Napi::Array>();

    CHECK_VOID(
      ids.Length() == values.Length(),
      "Expected IDs array and values array to have the same length",
      env
    );

    // Create a random permutation so that candidates are shuffled.
    std::vector<size_t> indexes(ids.Length());
    for (size_t i = 0; i < indexes.size(); i++) {
      indexes[i] = i;
      if (i > 0) {
        std::swap(indexes[rand() % i], indexes[i]);
      }
    }
    _impl.reserve(_impl.size() + ids.Length());

    for (auto i : indexes) {
      auto id_value = ids.Get(i);
      CHECK_VOID(
        id_value.IsNumber(),
        "Expected first array to contain only unsigned 32-bit integer ids",
        env
      );
      auto str_value = values.Get(i);
      CHECK_VOID(
        str_value.IsString(),
        "Expected first array to contain only strings",
        env
      );
      auto id = id_value.As<Napi::Number>();
      auto value = (std::string) str_value.As<Napi::String>();
      _impl.addCandidate(id, value);
    }
  }
}

void Matcher::RemoveCandidates(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  if (info.Length() > 0) {
    CHECK_VOID(
      info[0].IsArray(),
      "Expected an array of unsigned 32-bit integer IDs as the first argument",
      env
    );

    auto ids = info[0].As<Napi::Array>();
    for (size_t i = 0; i < ids.Length(); i++) {
      auto id_value = ids.Get(i);
      CHECK_VOID(
        id_value.IsNumber(),
        "Expected first array to contain only unsigned 32-bit integer ids",
        env
      );
      auto id = id_value.As<Napi::Number>();
      _impl.removeCandidate(id);
    }
  }
}

void Matcher::SetCandidates(const Napi::CallbackInfo& info) {
  _impl.clear();
  AddCandidates(info);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Matcher::Init(env, exports);

  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
