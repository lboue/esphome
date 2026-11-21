#pragma once
#ifdef USE_ZEPHYR

#include "esphome/components/canbus/canbus.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/version.h>

namespace esphome::zephyr_can {

class ZephyrCan : public canbus::Canbus {
 public:
  ZephyrCan(const struct device *can_dev, struct k_msgq *rx_queue, uint32_t bitrate, can_mode_t mode)
      : can_dev_(can_dev), rx_queue_(rx_queue), bitrate_(bitrate), mode_(mode) {}

#ifdef USE_ZEPHYR_CAN_TX_QUEUE
  void set_tx_queue(struct k_msgq *tx_queue) { this->tx_queue_ = tx_queue; }
#endif

  void dump_config() override;
  canbus::CanStatus get_status() override;

 protected:
  bool setup_internal() override;
  canbus::Error send_message(struct canbus::CanFrame *frame) override;
  canbus::Error read_message(struct canbus::CanFrame *frame) override;
  canbus::CanEventFlags get_events() override;

  /// Add one accept-all receive filter feeding rx_queue_. Returns false on failure.
  bool add_rx_filter_(bool extended_id);
  /// Report frames that failed to send since the last report.
  void log_tx_errors_();
#ifdef USE_ZEPHYR_CAN_TX_QUEUE
  /// Start sending queued frames unless a frame is already with the controller.
  void kick_tx_();
  /// Hand the next queued frame to the controller. Only called by the owner of tx_busy_.
  void send_next_();
  static void tx_work_handler_(struct k_work *work);
#endif
  static void tx_done_(const struct device *dev, int error, void *user_data);
  /// Turn a controller state change into canbus events for get_events(). ISR context.
  void latch_state_(enum can_state state);
#if KERNEL_VERSION_NUMBER >= 0x040500
  static void state_changed_(const struct device *dev, struct can_state_change_callback *callback, enum can_state state,
                             struct can_bus_err_cnt err_cnt);
#else
  static void state_changed_(const struct device *dev, enum can_state state, struct can_bus_err_cnt err_cnt,
                             void *user_data);
#endif

  const struct device *can_dev_;
  struct k_msgq *rx_queue_;
#ifdef USE_ZEPHYR_CAN_TX_QUEUE
  struct k_msgq *tx_queue_{nullptr};
  /// Runs send_next_() in thread context: the driver's send path can't be used from the TX done ISR.
  /// Wrapped so CONTAINER_OF works on a standard-layout type (ZephyrCan has virtual methods).
  struct TxWork {
    struct k_work work;
    ZephyrCan *parent;
  } tx_work_{};
  /// Set while a frame is with the controller or a refill is pending.
  atomic_t tx_busy_{ATOMIC_INIT(0)};
#endif
  /// Frames dropped after send_message() returned (can_send or transmission failed), reported by log_tx_errors_().
  atomic_t tx_errors_{ATOMIC_INIT(0)};
  atomic_t tx_last_error_{ATOMIC_INIT(0)};
  atomic_val_t reported_tx_errors_{0};
  uint32_t bitrate_;
  can_mode_t mode_;
  /// canbus::CanEventFlags bits set by state_changed_() and taken by get_events().
  atomic_t pending_events_{ATOMIC_INIT(0)};
  /// Previous controller state, only used by latch_state_().
  can_state isr_last_state_{CAN_STATE_ERROR_ACTIVE};
#if KERNEL_VERSION_NUMBER >= 0x040500
  /// Wrapped so CONTAINER_OF works on a standard-layout type, as with TxWork.
  struct StateCallback {
    struct can_state_change_callback callback;
    ZephyrCan *parent;
  } state_callback_{};
#endif
};

}  // namespace esphome::zephyr_can

#endif  // USE_ZEPHYR
