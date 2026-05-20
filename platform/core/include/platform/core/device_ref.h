#ifndef PLATFORM_CORE_DEVICE_REF_H_
#define PLATFORM_CORE_DEVICE_REF_H_

struct device;

namespace platform {

class DeviceRef final {
   public:
    constexpr DeviceRef() = default;
    explicit constexpr DeviceRef(const device* device) : device_(device) {}

    [[nodiscard]] constexpr const device* native_handle() const {
        return device_;
    }
    [[nodiscard]] constexpr bool has_value() const {
        return device_ != nullptr;
    }
    [[nodiscard]] bool is_ready() const;

    [[nodiscard]] explicit constexpr operator bool() const {
        return has_value();
    }

   private:
    const device* device_{nullptr};
};

}  // namespace platform

#endif  // PLATFORM_CORE_DEVICE_REF_H_
