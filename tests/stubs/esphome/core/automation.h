#pragma once
#include <functional>
// Just enough of esphome's Action and TEMPLATABLE_VALUE for the two actions.
namespace esphome {
template<typename T, typename... X> class TemplatableValue {
 public:
  TemplatableValue() = default;
  TemplatableValue(T value) : value_(value) {}
  TemplatableValue(std::function<T(X...)> f) : f_(f), is_lambda_(true) {}
  T value(X... x) const { return this->is_lambda_ ? this->f_(x...) : this->value_; }

 protected:
  T value_{};
  std::function<T(X...)> f_;
  bool is_lambda_{false};
};
template<typename... Ts> class Action {
 public:
  virtual ~Action() = default;
  virtual void play(const Ts &...x) = 0;
};
}  // namespace esphome
#define TEMPLATABLE_VALUE(type, name) \
 protected: \
  TemplatableValue<type, Ts...> name##_{}; \
\
 public: \
  template<typename V> void set_##name(V name) { this->name##_ = name; }
