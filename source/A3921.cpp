#include "A3921.h"

#include "include/Error.h"

namespace spirit {

A3921::A3921(InterfaceDigitalOut& sr, InterfacePwmOut& pwmh, InterfacePwmOut& pwml, InterfacePwmOut& phase,
             InterfaceDigitalOut& reset)
    : _sr(sr), _pwmh(pwmh), _pwml(pwml), _phase(phase), _reset(reset)
{
    sleep(false);
    _pwmh.period(Motor::Default::pulse_period);
    _pwml.period(Motor::Default::pulse_period);
    _phase.period(Motor::Default::pulse_period);
    run();
}

void A3921::sleep(const bool enabled)
{
    if (enabled) {
        _reset.write(0);
    } else {
        _reset.write(1);
    }
}

void A3921::reset(std::function<void(void)>& sleep)
{
    _reset.write(0);
    sleep();
    _reset.write(1);
}

void A3921::duty_cycle(const float value)
{
    _duty_cycle = value;
}

void A3921::state(const Motor::State type)
{
    switch (type) {
        case Motor::State::Coast:
        case Motor::State::CW:
        case Motor::State::CCW:
        case Motor::State::Brake:
            break;
        default:
            Error&         error            = Error::get_instance();
            constexpr char message_format[] = "Unknown motor state (%d)";
            char           message[sizeof(message_format) + Error::max_uint32_t_length];
            snprintf(message, sizeof(message), message_format, static_cast<uint32_t>(type));
            error.error(Error::Type::UnknownValue, 0, message, __FILE__, __func__, __LINE__);
            return;
    }

    _state = type;
}

void A3921::decay(const Motor::Decay type)
{
    Error& error = Error::get_instance();
    switch (type) {
        case Motor::Decay::Slow:
        case Motor::Decay::Fast:
            break;
        case Motor::Decay::Mixed:
            error.error(Error::Type::InvalidValue, 0, "Invalid motor decay (Motor::Decay::Mixed)", __FILE__, __func__,
                        __LINE__);
            return;
        default:
            constexpr char message_format[] = "Unknown motor decay (%d)";
            char           message[sizeof(message_format) + Error::max_uint32_t_length];
            snprintf(message, sizeof(message), message_format, static_cast<uint32_t>(type));
            error.error(Error::Type::UnknownValue, 0, message, __FILE__, __func__, __LINE__);
            return;
    }

    _decay = type;
}

void A3921::pwm_side(const Motor::PwmSide type)
{
    switch (type) {
        case Motor::PwmSide::Low:
        case Motor::PwmSide::High:
            break;
        default:
            Error&         error            = Error::get_instance();
            constexpr char message_format[] = "Unknown motor pwm side (%d)";
            char           message[sizeof(message_format) + Error::max_uint32_t_length];
            snprintf(message, sizeof(message), message_format, static_cast<uint32_t>(type));
            error.error(Error::Type::UnknownValue, 0, message, __FILE__, __func__, __LINE__);
            return;
    }

    _pwm_side = type;
}

void A3921::run()
{
    Error& error = Error::get_instance();
    switch (_decay) {
        case Motor::Decay::Slow:
            run_slow_decay();
            break;

        case Motor::Decay::Fast:
            run_fast_decay();
            break;

        case Motor::Decay::Mixed:
            error.error(Error::Type::InvalidValue, 0, "Invalid motor decay (Motor::Decay::Mixed)", __FILE__, __func__,
                        __LINE__);
            return;
        default:
            constexpr char message_format[] = "Unknown motor decay (%d)";
            char           message[sizeof(message_format) + Error::max_uint32_t_length];
            snprintf(message, sizeof(message), message_format, static_cast<uint32_t>(_decay));
            error.error(Error::Type::UnknownValue, 0, message, __FILE__, __func__, __LINE__);
            return;
    }
}

void A3921::run_slow_decay()
{
    float pwm_low_side;
    float pwm_high_side;

    switch (_pwm_side) {
        case Motor::PwmSide::Low:
            pwm_low_side  = _duty_cycle;
            pwm_high_side = 1.00F;
            break;
        case Motor::PwmSide::High:
            pwm_low_side  = 1.00F;
            pwm_high_side = _duty_cycle;
            break;
        default:
            Error&         error            = Error::get_instance();
            constexpr char message_format[] = "Unknown motor pwm side (%d)";
            char           message[sizeof(message_format) + Error::max_uint32_t_length];
            snprintf(message, sizeof(message), message_format, static_cast<uint32_t>(_pwm_side));
            error.error(Error::Type::UnknownValue, 0, message, __FILE__, __func__, __LINE__);
            return;
    }

    switch (_state) {
        case Motor::State::Coast:
            _sr.write(0);
            _pwmh.write(0.00F);
            _pwml.write(0.00F);
            _phase.write(0.00F);
            break;

        case Motor::State::CW:
            // a to b
            _sr.write(1);
            _pwmh.write(pwm_high_side);
            _pwml.write(pwm_low_side);
            _phase.write(1.00F);
            break;

        case Motor::State::CCW:
            // b to a
            _sr.write(1);
            _pwmh.write(pwm_high_side);
            _pwml.write(pwm_low_side);
            _phase.write(0.00F);
            break;

        case Motor::State::Brake:
            _sr.write(1);
            _pwmh.write(0.00F);
            _pwml.write(1.00F);
            _phase.write(0.00F);
            break;

        default:
            Error&         error            = Error::get_instance();
            constexpr char message_format[] = "Unknown motor state (%d)";
            char           message[sizeof(message_format) + Error::max_uint32_t_length];
            snprintf(message, sizeof(message), message_format, static_cast<uint32_t>(_state));
            error.error(Error::Type::UnknownValue, 0, message, __FILE__, __func__, __LINE__);
            return;
    }
}

void A3921::run_fast_decay()
{
    switch (_state) {
        case Motor::State::Coast:
            _sr.write(0);
            _pwmh.write(0.00F);
            _pwml.write(0.00F);
            _phase.write(0.00F);
            break;

        case Motor::State::CW:
            // a to b
            _sr.write(0);
            _pwmh.write(1.00F);
            _pwml.write(1.00F);
            _phase.write(0.50F + (_duty_cycle / 2.0F));
            break;

        case Motor::State::CCW:
            // b to a
            _sr.write(0);
            _pwmh.write(1.00F);
            _pwml.write(1.00F);
            _phase.write(0.50F - (_duty_cycle / 2.0F));
            break;

        case Motor::State::Brake:
            _sr.write(0);
            _pwmh.write(0.00F);
            _pwml.write(1.00F);
            _phase.write(0.00F);
            break;

        default:
            Error&         error            = Error::get_instance();
            constexpr char message_format[] = "Unknown motor state (%d)";
            char           message[sizeof(message_format) + Error::max_uint32_t_length];
            snprintf(message, sizeof(message), message_format, static_cast<uint32_t>(_state));
            error.error(Error::Type::UnknownValue, 0, message, __FILE__, __func__, __LINE__);
            return;
    }
}

void A3921::pulse_period(const float seconds)
{
    if ((Motor::min_pulse_period <= seconds) && (seconds <= Motor::max_pulse_period)) {
        _pwmh.period(seconds);
        _pwml.period(seconds);
        _phase.period(seconds);
    } else {
        Error&         error            = Error::get_instance();
        constexpr char message_format[] = "Invalid motor pulse period (%1.4e)";
        char           message[sizeof(message_format) + Error::max_float_1_4_e_length];
        snprintf(message, sizeof(message), message_format, seconds);
        error.error(Error::Type::InvalidValue, 0, message, __FILE__, __func__, __LINE__);
    }
}

}  // namespace spirit
